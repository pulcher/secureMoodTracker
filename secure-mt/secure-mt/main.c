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

// Polling helpers
#include "epoll_timerfd_utilities.h"

// Azure IoT SDK
#include <iothub_client_core_common.h>
#include <iothub_device_client_ll.h>
#include <iothub_client_options.h>
#include <iothubtransportmqtt.h>
#include <iothub.h>
#include <azure_sphere_provisioning.h>

static volatile sig_atomic_t terminationRequired = false;

// Azure IoT Hub/Central variables

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

// Button state variables
static GPIO_Value_Type sendMessageButtonState = GPIO_Value_High;
static GPIO_Value_Type sendOrientationButtonState = GPIO_Value_High;

static void ButtonPollTimerEventHandler(EventData* eventData);
static bool IsButtonPressed(int fd, GPIO_Value_Type* oldState);
static void SendMessageButtonHandler(void);

// event handler data structures. Only the event handler field needs to be populated.
static EventData buttonPollEventData = { .eventHandler = &ButtonPollTimerEventHandler };

/// <summary>
///     Signal handler for termination requests. This handler must be async-signal-safe.
/// </summary>
static void TerminationHandler(int signalNumber)
{
	// Don't use Log_Debug here, as it is not guaranteed to be async-signal-safe.
	terminationRequired = true;
}

int main(void)
{
	Log_Debug("IoT Hub/Central Application starting.\n");

	// Fire up the timer and the Event Handlers
	if (InitPeripheralsAndHandlers() != 0) {
		terminationRequired = true;
	}

	// Main loop
	while (!terminationRequired) {
		if (WaitForEventAndCallHandler(epollFd) != 0) {
			terminationRequired = true;
		}
	}
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

	return 0;
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
		//SendTelemetry("ButtonPress", "True");
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

	SendMessageButtonHandler();
	//SendOrientationButtonHandler();
}
