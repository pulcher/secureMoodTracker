#include <signal.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <stdarg.h>

#include "applibs_versions.h"
#include <applibs/gpio.h>
#include <applibs/log.h>
#include <applibs/networking.h>
#include <applibs/storage.h>

#include <hw/sample_hardware.h>

#include "build_options.h"

// Polling helpers
#include "epoll_timerfd_utilities.h"

// I2C connected sensors/modules
#include "i2c.h";
#include "mcp23x17.h";
#include "oled.h"
#include "lsm6dso_reg.h"
#include "lps22hh_reg.h"

// Azure IoT SDK
#include <iothub_client_core_common.h>
#include <iothub_device_client_ll.h>
#include <iothub_client_options.h>
#include <iothubtransportmqtt.h>
#include <iothub.h>
#include <azure_sphere_provisioning.h>

static volatile sig_atomic_t terminationRequired = false;

// Azure IoT Hub/Central stuff
#define SCOPEID_LENGTH 20
static char scopeId[SCOPEID_LENGTH]; // ScopeId for the Azure IoT Central application, set in
									 // app_manifest.json, CmdArgs
static IOTHUB_DEVICE_CLIENT_LL_HANDLE iothubClientHandle = NULL;
static bool iothubAuthenticated = false;
static const int keepalivePeriodSeconds = 20;

static void SendTelemetry(const unsigned char* key, const unsigned char* value);
static void SendMessageCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* context);
static const char* GetReasonString(IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason);
static const char* getAzureSphereProvisioningResultString(AZURE_SPHERE_PROV_RETURN_VALUE provisioningResult);

// Initialization/Cleanup
static int InitPeripheralsAndHandlers(void);
static void ClosePeripheralsAndHandlers(void);

// File descriptors - initialized to invalid value
// Buttons
static int sendMessageButtonGpioFd = -1;
static int sendOrientationButtonGpioFd = -1;

// Timer / polling
static int buttonPollTimerFd = -1;
static int azureTimerFd = -1;
static int epollFd = -1;

// Azure IoT poll periods
static const int AzureIoTDefaultPollPeriodSeconds = 5;
static const int AzureIoTMinReconnectPeriodSeconds = 5;
static const int AzureIoTMaxReconnectPeriodSeconds = 1 * 60;

static int azureIoTPollPeriodSeconds = -1;

// Button state variables
#define IDX_GREEN_BTN 0
#define IDX_YELLOW_BTN 1
#define IDX_RED_BTN 2
#define IDX_PROXIMITY 3

#define GREEN_VOTE 3
#define YELLOW_VOTE 2
#define RED_VOTE 1

#define NUM_INPUT_TRACKING 4

#define GREEN_ELEMENT_NAME "HappyButton"
#define YELLOW_ELEMENT_NAME "MehButton"
#define RED_ELEMENT_NAME "MadButton"
#define PROXIMITY_ELEMENT_NAME "ProximityAlert"

struct InputState {
	char ElementName[20];
	int Vote;
	int State;
};

struct InputState previousInputState[NUM_INPUT_TRACKING] = {
	{GREEN_ELEMENT_NAME, 3, 0},
	{YELLOW_ELEMENT_NAME, 2, 0},
	{RED_ELEMENT_NAME, 1, 0},
	{PROXIMITY_ELEMENT_NAME, 0, 0}
};

struct InputState currentInputState[NUM_INPUT_TRACKING] = {
	{GREEN_ELEMENT_NAME, 3, 0},
	{YELLOW_ELEMENT_NAME, 2, 0},
	{RED_ELEMENT_NAME, 1, 0},
	{PROXIMITY_ELEMENT_NAME, 0, 0}
};

static GPIO_Value_Type sendMessageButtonState = GPIO_Value_High;

static void ButtonPollTimerEventHandler(EventData* eventData);
static bool IsButtonPressed(int fd, GPIO_Value_Type* oldState);
static void SendMessageButtonHandler(void);
static void AzureTimerEventHandler(EventData* eventData);
static void RetainPreviousState();
static void UpdateCurrentState();
static void HandleInput(int index);

// event handler data structures. Only the event handler field needs to be populated.
static EventData buttonPollEventData = { .eventHandler = &ButtonPollTimerEventHandler };
static EventData azureEventData = { .eventHandler = &AzureTimerEventHandler };

/// <summary>
///     Signal handler for termination requests. This handler must be async-signal-safe.
/// </summary>
static void TerminationHandler(int signalNumber)
{
	// Don't use Log_Debug here, as it is not guaranteed to be async-signal-safe.
	terminationRequired = true;
}

int main(int argc, char* argv[])
{
	Log_Debug("IoT Hub/Central Application starting.\n");

	if (argc == 2) {
		Log_Debug("Setting Azure Scope ID %s\n", argv[1]);
		strncpy(scopeId, argv[1], SCOPEID_LENGTH);
	}
	else {
		Log_Debug("ScopeId needs to be set in the app_manifest CmdArgs\n");
		return -1;
	}

	// Fire up the timer and the Event Handlers
	if (InitPeripheralsAndHandlers() != 0) {
		//terminationRequired = true;
	}

	// Main loop
	while (!terminationRequired) {
		if (WaitForEventAndCallHandler(epollFd) != 0) {
			terminationRequired = true;
		}
	}

	ClosePeripheralsAndHandlers();

	Log_Debug("Application exiting.\n");

	return 0;
}

/// <summary>
///     Set up SIGTERM termination handler, initialize peripherals, and set up event handlers.
/// </summary>
/// <returns>0 on success, or -1 on failure</returns>
static int InitPeripheralsAndHandlers(void)
{
	struct sigaction action;
	memset(&action, 0, sizeof(struct sigaction));
	action.sa_handler = TerminationHandler;
	sigaction(SIGTERM, &action, NULL);

	epollFd = CreateEpollFd();
	if (epollFd < 0) {
		return -1;
	}

	// Open button A GPIO as input
	Log_Debug("Opening SAMPLE_BUTTON_1 as input\n");
	sendMessageButtonGpioFd = GPIO_OpenAsInput(SAMPLE_BUTTON_1);
	if (sendMessageButtonGpioFd < 0) {
		Log_Debug("ERROR: Could not open button A: %s (%d).\n", strerror(errno), errno);
		return -1;
	}

	// Open button B GPIO as input
	Log_Debug("Opening SAMPLE_BUTTON_2 as input\n");
	sendOrientationButtonGpioFd = GPIO_OpenAsInput(SAMPLE_BUTTON_2);
	if (sendOrientationButtonGpioFd < 0) {
		Log_Debug("ERROR: Could not open button B: %s (%d).\n", strerror(errno), errno);
		return -1;
	}

	// Set up a timer to poll for button events.
	struct timespec buttonPressCheckPeriod = { 0, 1000 * 1000 };
	buttonPollTimerFd =
		CreateTimerFdAndAddToEpoll(epollFd, &buttonPressCheckPeriod, &buttonPollEventData, EPOLLIN);
	if (buttonPollTimerFd < 0) {
		return -1;
	}

	// initialize the I2C master
	// - need pull ups?
	// - need status indicators for guard code
	initI2c();
	if (i2cFd < 0) {
		Log_Debug("ERROR: Could not open I2C master: %s (%d).\n", strerror(errno), errno);
		return -1;
	}

	// initialize the OLED screen
		// Start OLED
	if (oled_init())
	{
		Log_Debug("OLED not found!\n");
	}
	else
	{
		Log_Debug("OLED found!\n");
	}

	// Draw AVNET logo
	oled_draw_logo();
	oled_i2c_bus_status(0);

	// initialize the MCP23017
	// - Port A input from buttons and proximity
	// - Port B output to button LEDS
	//
	// - No need for interrupts as we are polling at every 0.001 seconds.
	// - set port a with all pull-up resistors
	// - output can be high, as we are going through transistors to power the lights.
	mcp23x17_init(&i2cFd, 0);

	bool mcp23x17Detected = false;
	int failCount = 10;
	int32_t resultCheck = -1;

	while (!mcp23x17Detected) {
		// Check if mcp23x17Detected is connected
		resultCheck = mcp23x17_device_id_get(&mcp23x17_ctx, &whoami);

		if ((resultCheck < 0) || (whoami != MCP23X17_DEFAULT_HIGH)) {
			Log_Debug("resultCheck: %d\n", resultCheck);
			Log_Debug("whoami: %0x\n", whoami);
		}
		if (resultCheck < 0) {
			Log_Debug("MCP23x17 not found!\n");

			mcp23x17_status = 1;
		}
		else {
				mcp23x17Detected = true;
				Log_Debug("MCP23X17 Found!\n");

				mcp23x17_status = 0;

				// add setup code here
				uint8_t setOuputBank = 0x00U;
				mcp23x17_write_reg(&mcp23x17_ctx, MCP23017_IODIRB, &setOuputBank, 1);

				uint8_t testRead = 0xffU;

				uint8_t test = mcp23x17_read_reg(&mcp23x17_ctx, MCP23017_IODIRB, &testRead, 1);

				Log_Debug("test: %d\n", test);
				Log_Debug("testRead: %0x\n", testRead);

				// Setup default lights
				uint8_t testOutput = 0x07U;

				ssize_t writeRet = mcp23x17_write_reg(&mcp23x17_ctx, MCP23017_GPIOB, &testOutput, 1);

				uint8_t readRet = mcp23x17_read_reg(&mcp23x17_ctx, MCP23017_GPIOB, &testRead, 1);
				Log_Debug("readRet: %d\n", readRet);
				Log_Debug("testRead: %0x\n", testRead);

				// Setup the input buttons
				uint8_t baseInputState = 0xffU;  // this is the default, but I am making sure
				writeRet = mcp23x17_write_reg(&mcp23x17_ctx, MCP23017_IODIRA, &baseInputState, 1);
				writeRet = mcp23x17_write_reg(&mcp23x17_ctx, MCP23017_GPPUA, &baseInputState, 1);

				readRet = mcp23x17_read_reg(&mcp23x17_ctx, MCP23017_GPIOA, &testRead, 1);

				Log_Debug("readRet(portA): %d\n", readRet);
				Log_Debug("testRead(portA): %0x\n", testRead);
		}

		// If we failed to detect the mcp23x17Detected device, then pause before trying again.
		if (!mcp23x17Detected) {
			HAL_Delay(100);
		}

		if (failCount-- == 0) {
			Log_Debug("Failed to read mcp23x17 device ID.\n");
			return -1;
		}
	}

	// initialize the temp and humidity

	// lps22hh specific init

	bool lps22hhDetected = false;
	failCount = 10;

	while (!lps22hhDetected) {

		// Enable pull up on master I2C interface.
		lsm6dso_sh_pin_mode_set(&dev_ctx, LSM6DSO_INTERNAL_PULL_UP);

		// Check if LPS22HH is connected to Sensor Hub
		lps22hh_device_id_get(&pressure_ctx, &whoami);
		if (whoami != LPS22HH_ID) {
			Log_Debug("LPS22HH not found!\n");
		}
		else {
			lps22hhDetected = true;
			Log_Debug("LPS22HH Found!\n");
		}

		// Restore the default configuration
		lps22hh_reset_set(&pressure_ctx, PROPERTY_ENABLE);
		do {
			lps22hh_reset_get(&pressure_ctx, &rst);
		} while (rst);

		// Enable Block Data Update
		lps22hh_block_data_update_set(&pressure_ctx, PROPERTY_ENABLE);

		//Set Output Data Rate
		lps22hh_data_rate_set(&pressure_ctx, LPS22HH_10_Hz_LOW_NOISE);

		// If we failed to detect the lps22hh device, then pause before trying again.
		if (!lps22hhDetected) {
			HAL_Delay(100);
		}

		if (failCount-- == 0) {
			Log_Debug("Failed to read LSM22HH device ID, exiting\n");
			return -1;
		}
	}

	// may need to pump the i2c stuff through a single reader/writer
	// maybe make a symaphore for the longer periodic and write stuff
	// to limit the fast polling

	azureIoTPollPeriodSeconds = AzureIoTDefaultPollPeriodSeconds;
	struct timespec azureTelemetryPeriod = { azureIoTPollPeriodSeconds, 0 };
	azureTimerFd =
		CreateTimerFdAndAddToEpoll(epollFd, &azureTelemetryPeriod, &azureEventData, EPOLLIN);
	if (buttonPollTimerFd < 0) {
		return -1;
	}

	return 0;
}

/// <summary>
///     Sets the IoT Hub authentication state for the app
///     The SAS Token expires which will set the authentication state
/// </summary>
static void HubConnectionStatusCallback(IOTHUB_CLIENT_CONNECTION_STATUS result,
	IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason,
	void* userContextCallback)
{
	iothubAuthenticated = (result == IOTHUB_CLIENT_CONNECTION_AUTHENTICATED);
	Log_Debug("IoT Hub Authenticated: %s\n", GetReasonString(reason));
}

/// <summary>
///     Sets up the Azure IoT Hub connection (creates the iothubClientHandle)
///     When the SAS Token for a device expires the connection needs to be recreated
///     which is why this is not simply a one time call.

/// </summary>


static void SetupAzureClient(void)
{
	if (iothubClientHandle != NULL)
		IoTHubDeviceClient_LL_Destroy(iothubClientHandle);

	AZURE_SPHERE_PROV_RETURN_VALUE provResult =
		IoTHubDeviceClient_LL_CreateWithAzureSphereDeviceAuthProvisioning(scopeId, 10000,
			&iothubClientHandle);
	Log_Debug("IoTHubDeviceClient_LL_CreateWithAzureSphereDeviceAuthProvisioning returned '%s'.\n",
		getAzureSphereProvisioningResultString(provResult));

	if (provResult.result != AZURE_SPHERE_PROV_RESULT_OK) {

		// If we fail to connect, reduce the polling frequency, starting at
		// AzureIoTMinReconnectPeriodSeconds and with a backoff up to
		// AzureIoTMaxReconnectPeriodSeconds
		if (azureIoTPollPeriodSeconds == AzureIoTDefaultPollPeriodSeconds) {
			azureIoTPollPeriodSeconds = AzureIoTMinReconnectPeriodSeconds;
		}
		else {
			azureIoTPollPeriodSeconds *= 2;
			if (azureIoTPollPeriodSeconds > AzureIoTMaxReconnectPeriodSeconds) {
				azureIoTPollPeriodSeconds = AzureIoTMaxReconnectPeriodSeconds;
			}
		}

		struct timespec azureTelemetryPeriod = { azureIoTPollPeriodSeconds, 0 };
		SetTimerFdToPeriod(azureTimerFd, &azureTelemetryPeriod);

		Log_Debug("ERROR: failure to create IoTHub Handle - will retry in %i seconds.\n",
			azureIoTPollPeriodSeconds);
		return;
	}

	// Successfully connected, so make sure the polling frequency is back to the default
	azureIoTPollPeriodSeconds = AzureIoTDefaultPollPeriodSeconds;
	struct timespec azureTelemetryPeriod = { azureIoTPollPeriodSeconds, 0 };
	SetTimerFdToPeriod(azureTimerFd, &azureTelemetryPeriod);

	iothubAuthenticated = true;

	if (IoTHubDeviceClient_LL_SetOption(iothubClientHandle, OPTION_KEEP_ALIVE,
		&keepalivePeriodSeconds) != IOTHUB_CLIENT_OK) {
		Log_Debug("ERROR: failure setting option \"%s\"\n", OPTION_KEEP_ALIVE);
		return;
	}

	//IoTHubDeviceClient_LL_SetDeviceTwinCallback(iothubClientHandle, TwinCallback, NULL);
	IoTHubDeviceClient_LL_SetConnectionStatusCallback(iothubClientHandle,
		HubConnectionStatusCallback, NULL);
}

/// <summary>
///     Converts the IoT Hub connection status reason to a string.
/// </summary>
static const char* GetReasonString(IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason)
{
	static char* reasonString = "unknown reason";
	switch (reason) {
	case IOTHUB_CLIENT_CONNECTION_EXPIRED_SAS_TOKEN:
		reasonString = "IOTHUB_CLIENT_CONNECTION_EXPIRED_SAS_TOKEN";
		break;
	case IOTHUB_CLIENT_CONNECTION_DEVICE_DISABLED:
		reasonString = "IOTHUB_CLIENT_CONNECTION_DEVICE_DISABLED";
		break;
	case IOTHUB_CLIENT_CONNECTION_BAD_CREDENTIAL:
		reasonString = "IOTHUB_CLIENT_CONNECTION_BAD_CREDENTIAL";
		break;
	case IOTHUB_CLIENT_CONNECTION_RETRY_EXPIRED:
		reasonString = "IOTHUB_CLIENT_CONNECTION_RETRY_EXPIRED";
		break;
	case IOTHUB_CLIENT_CONNECTION_NO_NETWORK:
		reasonString = "IOTHUB_CLIENT_CONNECTION_NO_NETWORK";
		break;
	case IOTHUB_CLIENT_CONNECTION_COMMUNICATION_ERROR:
		reasonString = "IOTHUB_CLIENT_CONNECTION_COMMUNICATION_ERROR";
		break;
	case IOTHUB_CLIENT_CONNECTION_OK:
		reasonString = "IOTHUB_CLIENT_CONNECTION_OK";
		break;
	}
	return reasonString;
}

/// <summary>
///     Converts AZURE_SPHERE_PROV_RETURN_VALUE to a string.
/// </summary>
static const char* getAzureSphereProvisioningResultString(
	AZURE_SPHERE_PROV_RETURN_VALUE provisioningResult)
{
	switch (provisioningResult.result) {
	case AZURE_SPHERE_PROV_RESULT_OK:
		return "AZURE_SPHERE_PROV_RESULT_OK";
	case AZURE_SPHERE_PROV_RESULT_INVALID_PARAM:
		return "AZURE_SPHERE_PROV_RESULT_INVALID_PARAM";
	case AZURE_SPHERE_PROV_RESULT_NETWORK_NOT_READY:
		return "AZURE_SPHERE_PROV_RESULT_NETWORK_NOT_READY";
	case AZURE_SPHERE_PROV_RESULT_DEVICEAUTH_NOT_READY:
		return "AZURE_SPHERE_PROV_RESULT_DEVICEAUTH_NOT_READY";
	case AZURE_SPHERE_PROV_RESULT_PROV_DEVICE_ERROR:
		return "AZURE_SPHERE_PROV_RESULT_PROV_DEVICE_ERROR";
	case AZURE_SPHERE_PROV_RESULT_GENERIC_ERROR:
		return "AZURE_SPHERE_PROV_RESULT_GENERIC_ERROR";
	default:
		return "UNKNOWN_RETURN_VALUE";
	}
}

/// <summary>
///     Close peripherals and handlers.
/// </summary>
static void ClosePeripheralsAndHandlers(void)
{
	Log_Debug("Closing file descriptors\n");

	//// Leave the LEDs off
	//if (deviceTwinStatusLedGpioFd >= 0) {
	//	GPIO_SetValue(deviceTwinStatusLedGpioFd, GPIO_Value_High);
	//}

	CloseFdAndPrintError(buttonPollTimerFd, "ButtonTimer");
	CloseFdAndPrintError(azureTimerFd, "AzureTimer");
	CloseFdAndPrintError(sendMessageButtonGpioFd, "SendMessageButton");
	//CloseFdAndPrintError(sendOrientationButtonGpioFd, "SendOrientationButton");
	//CloseFdAndPrintError(deviceTwinStatusLedGpioFd, "StatusLed");
	CloseFdAndPrintError(epollFd, "Epoll");
	CloseFdAndPrintError(i2cFd, "I2C");
}

/*
 * Handlers
 */

 /// <summary>
 ///     Check whether a given button has just been pressed.
 /// </summary>
 /// <param name="fd">The button file descriptor</param>
 /// <param name="oldState">Old state of the button (pressed or released)</param>
 /// <returns>true if pressed, false otherwise</returns>
static bool IsButtonPressed(int fd, GPIO_Value_Type* oldState)
{
	bool isButtonPressed = false;
	GPIO_Value_Type newState;
	int result = GPIO_GetValue(fd, &newState);
	if (result != 0) {
		Log_Debug("ERROR: Could not read button GPIO: %s (%d).\n", strerror(errno), errno);
		terminationRequired = true;
	}
	else {
		// Button is pressed if it is low and different than last known state.
		isButtonPressed = (newState != *oldState) && (newState == GPIO_Value_Low);
		*oldState = newState;
	}

	return isButtonPressed;
}

/// <summary>
/// Pressing button A will:
///     Send a 'Button Pressed' event to Azure IoT Central
/// </summary>
static void SendMessageButtonHandler(void)
{
	if (IsButtonPressed(sendMessageButtonGpioFd, &sendMessageButtonState)) {
		Log_Debug("Button pressed\n");
		SendTelemetry("ButtonPress", "True");
	}
}

/// <summary>
/// IsInputStateChanged(index)
/// Compare the state of the input and report back whether the state has changed from the last time checked
/// </summary>
static bool IsInputStateChanged(int index)
{
	bool changed = currentInputState[index].State != previousInputState[index].State;
	return changed;
}

/// <summary>
/// UpdateButtonLED
/// Read the current state of the input and update the LEDS of the buttons.
/// </summary>
static void UpdateButtonLED()
{
	uint8_t buttonState = 0x00U;

	//for (size_t index = 2; index >= 0; index--)
	//{
	//	buttonState << 1;
	//}

	//uint8_t checkState = buttonState;
}

/// <summary>
/// Updates the current status of the input
/// - checks if the state is changed
/// - updates LED output to reflect the state
/// - updates the vote totals
/// - send message on low input (button press)
/// <summary>
static void HandleInput(int index)
{
	if (!IsInputStateChanged(index))
		return;

	UpdateButtonLED();	// will read the current state and update the lights.

	if (currentInputState[index].State) {
		//updateTotals(currentInputState[index].Vote);
		Log_Debug("Button Pressed: %s.\n", currentInputState[index].ElementName);
		SendTelemetry(currentInputState[index].ElementName, "True");
	}
	else {
		Log_Debug("Button Released: %s.\n", currentInputState[index].ElementName);
		SendTelemetry(currentInputState[index].ElementName, "False");
	}
}

/// <summary>
/// RetainPreviousState will copy the current button state to a previous for comparison
/// </summary>
static void RetainPreviousState()
{
	for (size_t i = 0; i < 4; i++)
	{
		previousInputState[i].State = currentInputState[i].State;
	}
}

/// <summary>
/// UpdateCurrentState will read the current input state then update the currentButtonState
/// to be used by the rest of the program
/// </summary>
static void UpdateCurrentState()
{
	uint8_t inputState = 0x00U;

	// grab the input byte from the MCP23017
	uint8_t readRet = mcp23x17_read_reg(&mcp23x17_ctx, MCP23017_GPIOA, &inputState, 1);

	uint8_t checkPosition = 0x01U;

	// Shift off the value of each of the bits into the correct position of the state
	for (size_t i = 0; i < 4; i++)
	{
		currentInputState[i].State = checkPosition & inputState ? 0 : 1;

		// move the checkPosition to what we care about
		checkPosition <<= 1;
	}
}

/// <summary>
/// Button timer event:  Check the status of buttons A and B
/// </summary>
static void ButtonPollTimerEventHandler(EventData* eventData)
{
	if (ConsumeTimerFdEvent(buttonPollTimerFd) != 0) {
		terminationRequired = true;
		return;
	}

	// test if the mcp is online and active
	// pull the port A from mcp23017
	if (!mcp23x17_status) {

		// break apart the bits
		RetainPreviousState();
		UpdateCurrentState();

		// Send in the array of structs that holds:
		// - state
		// - Message to send to Azure
		// - value to adjust the daily totals if any
		// - Element Name for Azure
		// -
		HandleInput(IDX_GREEN_BTN);
		HandleInput(IDX_YELLOW_BTN);
		HandleInput(IDX_RED_BTN);
		HandleInput(IDX_PROXIMITY);

		// for each in the array that is non-zero then send a message for each
		// proximity is just a funky button

		// update the screen with a thanks for each with a pause... probably should make that call async
		// maybe setup an array with the message and have the message pop up over the
	}

	SendMessageButtonHandler();
}

/// <summary>
/// Azure timer event:  Check connection status and send telemetry
/// </summary>
static void AzureTimerEventHandler(EventData* eventData)
{
	if (ConsumeTimerFdEvent(azureTimerFd) != 0) {
		terminationRequired = true;
		return;
	}

	bool isNetworkReady = false;
	if (Networking_IsNetworkingReady(&isNetworkReady) != -1) {
		if (isNetworkReady && !iothubAuthenticated) {
			SetupAzureClient();
		}
	}
	else {
		Log_Debug("Failed to get Network state\n");
	}

	if (iothubAuthenticated) {
		// this is where all the periodic status of things is sent.
		//SendSimulatedTemperature();
		IoTHubDeviceClient_LL_DoWork(iothubClientHandle);
	}
}

// Azure stuff

/// <summary>
///     Sends telemetry to IoT Hub
/// </summary>
/// <param name="key">The telemetry item to update</param>
/// <param name="value">new telemetry value</param>
static void SendTelemetry(const unsigned char* key, const unsigned char* value)
{
	static char eventBuffer[100] = { 0 };
	static const char* EventMsgTemplate = "{ \"%s\": \"%s\" }";
	int len = snprintf(eventBuffer, sizeof(eventBuffer), EventMsgTemplate, key, value);
	if (len < 0)
		return;

	Log_Debug("Sending IoT Hub Message: %s\n", eventBuffer);

	IOTHUB_MESSAGE_HANDLE messageHandle = IoTHubMessage_CreateFromString(eventBuffer);

	if (messageHandle == 0) {
		Log_Debug("WARNING: unable to create a new IoTHubMessage\n");
		return;
	}

	if (IoTHubDeviceClient_LL_SendEventAsync(iothubClientHandle, messageHandle, SendMessageCallback,
		/*&callback_param*/ 0) != IOTHUB_CLIENT_OK) {
		Log_Debug("WARNING: failed to hand over the message to IoTHubClient\n");
	}
	else {
		Log_Debug("INFO: IoTHubClient accepted the message for delivery\n");
	}

	IoTHubMessage_Destroy(messageHandle);
}

/// <summary>
///     Callback confirming message delivered to IoT Hub.
/// </summary>
/// <param name="result">Message delivery status</param>
/// <param name="context">User specified context</param>
static void SendMessageCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* context)
{
	Log_Debug("INFO: Message received by IoT Hub. Result is: %d\n", result);
}
