#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "applibs_versions.h"		// defines the struct version for the applibs

#include <hw\sample_hardware.h>

#include <applibs/log.h>
#include <applibs/i2c.h>

#include "i2c.h"
#include "lsm6dso_reg.h"
#include "lps22hh_reg.h"

// extern vars
int i2cFd = -1;		// i2c file handle
extern int epollFd;
extern volatile sig_atomic_t terminationRequired;
const uint8_t lsm6dsOAddress = LSM6DSO_ADDRESS;     // Addr = 0x6A

/// <summary>
///		Initialize the I2C interface.  Sets i2cFd
/// <summary>
/// <returns>0 on success, -1 on failure</returns>
int initI2c(void) {

	// Begin MT3620 I2C init

	i2cFd = I2CMaster_Open(SAMPLE_LSM6DS3_I2C);
	if (i2cFd < 0) {
		Log_Debug("ERROR: I2CMaster_Open: errno=%d (%s)\n", errno, strerror(errno));
		return -1;
	}

	int result = I2CMaster_SetBusSpeed(i2cFd, I2C_BUS_SPEED_STANDARD);
	if (result != 0) {
		Log_Debug("ERROR: I2CMaster_SetBusSpeed: errno=%d (%s)\n", errno, strerror(errno));
		return -1;
	}

	result = I2CMaster_SetTimeout(i2cFd, 100);
	if (result != 0) {
		Log_Debug("ERROR: I2CMaster_SetTimeout: errno=%d (%s)\n", errno, strerror(errno));
		return -1;
	}

	whoami = -1;

	// Initialize lsm6dso mems driver interface
	dev_ctx.write_reg = platform_write;
	dev_ctx.read_reg = platform_read;
	dev_ctx.handle = &i2cFd;

	// Initialize lps22hh mems driver interface
	pressure_ctx.read_reg = lsm6dso_read_lps22hh_cx;
	pressure_ctx.write_reg = lsm6dso_write_lps22hh_cx;
	pressure_ctx.handle = &i2cFd;

	return 0;
}

/*
 * @brief  Write lsm2mdl device register (used by configuration functions)
 *
 * @param  handle    customizable argument. In this examples is used in
 *                   order to select the correct sensor bus handler.
 * @param  reg       register to write
 * @param  bufp      pointer to data to write in register reg
 * @param  len       number of consecutive register to write
 *
 */
static int32_t lsm6dso_write_lps22hh_cx(void* ctx, uint8_t reg, uint8_t* data,
	uint16_t len)
{
	axis3bit16_t data_raw_acceleration;
	int32_t ret;
	uint8_t drdy;
	lsm6dso_status_master_t master_status;
	lsm6dso_sh_cfg_write_t sh_cfg_write;

	// Configure Sensor Hub to write to the LPS22HH, and send the write data
	sh_cfg_write.slv0_add = (LPS22HH_I2C_ADD_L & 0xFEU) >> 1; // 7bit I2C address
	sh_cfg_write.slv0_subadd = reg,
		sh_cfg_write.slv0_data = *data,
		ret = lsm6dso_sh_cfg_write(&dev_ctx, &sh_cfg_write);

	/* Disable accelerometer. */
	lsm6dso_xl_data_rate_set(&dev_ctx, LSM6DSO_XL_ODR_OFF);

	/* Enable I2C Master. */
	lsm6dso_sh_master_set(&dev_ctx, PROPERTY_ENABLE);

	/* Enable accelerometer to trigger Sensor Hub operation. */
	lsm6dso_xl_data_rate_set(&dev_ctx, LSM6DSO_XL_ODR_104Hz);

	/* Wait Sensor Hub operation flag set. */
	lsm6dso_acceleration_raw_get(&dev_ctx, data_raw_acceleration.u8bit);
	do
	{
		HAL_Delay(20);
		lsm6dso_xl_flag_data_ready_get(&dev_ctx, &drdy);
	} while (!drdy);

	do
	{
		HAL_Delay(20);
		lsm6dso_sh_status_get(&dev_ctx, &master_status);
	} while (!master_status.sens_hub_endop);

	/* Disable I2C master and XL (trigger). */
	lsm6dso_sh_master_set(&dev_ctx, PROPERTY_DISABLE);
	lsm6dso_xl_data_rate_set(&dev_ctx, LSM6DSO_XL_ODR_OFF);

	return ret;
}

/*
 * @brief  Read lsm2mdl device register (used by configuration functions)
 *
 * @param  handle    customizable argument. In this examples is used in
 *                   order to select the correct sensor bus handler.
 * @param  reg       register to read
 * @param  bufp      pointer to buffer that store the data read
 * @param  len       number of consecutive register to read
 *
 */
static int32_t lsm6dso_read_lps22hh_cx(void* ctx, uint8_t reg, uint8_t* data, uint16_t len)
{
	lsm6dso_sh_cfg_read_t sh_cfg_read;
	axis3bit16_t data_raw_acceleration;
	int32_t ret;
	uint8_t drdy;
	lsm6dso_status_master_t master_status;

	/* Disable accelerometer. */
	lsm6dso_xl_data_rate_set(&dev_ctx, LSM6DSO_XL_ODR_OFF);

	// For each byte we need to read from the lps22hh
	for (int i = 0; i < len; i++) {

		/* Configure Sensor Hub to read LPS22HH. */
		sh_cfg_read.slv_add = (LPS22HH_I2C_ADD_L & 0xFEU) >> 1; /* 7bit I2C address */
		sh_cfg_read.slv_subadd = reg + i;
		sh_cfg_read.slv_len = 1;

		// Call the command to read the data from the sensor hub.
		// This data will be read from the device connected to the
		// sensor hub, and saved into a register for us to read.
		ret = lsm6dso_sh_slv0_cfg_read(&dev_ctx, &sh_cfg_read);
		lsm6dso_sh_slave_connected_set(&dev_ctx, LSM6DSO_SLV_0);

		/* Enable I2C Master and I2C master. */
		lsm6dso_sh_master_set(&dev_ctx, PROPERTY_ENABLE);

		/* Enable accelerometer to trigger Sensor Hub operation. */
		lsm6dso_xl_data_rate_set(&dev_ctx, LSM6DSO_XL_ODR_104Hz);

		/* Wait Sensor Hub operation flag set. */
		lsm6dso_acceleration_raw_get(&dev_ctx, data_raw_acceleration.u8bit);
		do {
			HAL_Delay(20);
			lsm6dso_xl_flag_data_ready_get(&dev_ctx, &drdy);
		} while (!drdy);

		do {
			HAL_Delay(20);
			lsm6dso_sh_status_get(&dev_ctx, &master_status);
		} while (!master_status.sens_hub_endop);

		/* Disable I2C master and XL(trigger). */
		lsm6dso_sh_master_set(&dev_ctx, PROPERTY_DISABLE);
		lsm6dso_xl_data_rate_set(&dev_ctx, LSM6DSO_XL_ODR_OFF);

		// Read the data from the device.  The call below reads
		// all 18 sensor hub data.  We just need the data from
		// sensor hub 1, so copy that into our data array.
		uint8_t buffer[18];
		lsm6dso_sh_read_data_raw_get(&dev_ctx, buffer);
		data[i] = buffer[1];

#ifdef ENABLE_READ_WRITE_DEBUG
		Log_Debug("Read %d bytes: ", len);
		for (int i = 0; i < len; i++) {
			Log_Debug("[%0x] ", data[i]);
		}
		Log_Debug("\n", len);
#endif
	}

	/* Re-enable accelerometer */
	lsm6dso_xl_data_rate_set(&dev_ctx, LSM6DSO_XL_ODR_104Hz);

	return ret;
}

/// <summary>
///     Writes data to the lsm6dso i2c device
/// </summary>
/// <returns>0</returns>

static int32_t platform_write(int* fD, uint8_t reg, uint8_t* bufp,
	uint16_t len)
{

#ifdef ENABLE_READ_WRITE_DEBUG
	Log_Debug("platform_write()\n");
	Log_Debug("reg: %0x\n", reg);
	Log_Debug("len: %0x\n", len);
	Log_Debug("bufp contents: ");
	for (int i = 0; i < len; i++) {

		Log_Debug("%0x: ", bufp[i]);
	}
	Log_Debug("\n");
#endif

	// Construct a new command buffer that contains the register to write to, then the data to write
	uint8_t cmdBuffer[len + 1];
	cmdBuffer[0] = reg;
	for (int i = 0; i < len; i++) {
		cmdBuffer[i + 1] = bufp[i];
	}

#ifdef ENABLE_READ_WRITE_DEBUG
	Log_Debug("cmdBuffer contents: ");
	for (int i = 0; i < len + 1; i++) {

		Log_Debug("%0x: ", cmdBuffer[i]);
	}
	Log_Debug("\n");
#endif

	// Write the data to the device
	int32_t retVal = I2CMaster_Write(*fD, lsm6dsOAddress, cmdBuffer, (size_t)len + 1);
	if (retVal < 0) {
		Log_Debug("ERROR: platform_write: errno=%d (%s)\n", errno, strerror(errno));
		return -1;
	}
#ifdef ENABLE_READ_WRITE_DEBUG
	Log_Debug("Wrote %d bytes to device.\n\n", retVal);
#endif
	return 0;
}

/// <summary>
///     Reads generic device register from the i2c interface
/// </summary>
/// <returns>0</returns>

/*
 * @brief  Read generic device register (platform dependent)
 *
 * @param  handle    customizable argument. In this examples is used in
 *                   order to select the correct sensor bus handler.
 * @param  reg       register to read
 * @param  bufp      pointer to buffer that store the data read
 * @param  len       number of consecutive register to read
 *
 */
static int32_t platform_read(int* fD, uint8_t reg, uint8_t* bufp,
	uint16_t len)
{

#ifdef ENABLE_READ_WRITE_DEBUG
	Log_Debug("platform_read()\n");
	Log_Debug("reg: %0x\n", reg);
	Log_Debug("len: %d\n", len);
	;
#endif

	// Set the register address to read
	int32_t retVal = I2CMaster_Write(i2cFd, lsm6dsOAddress, &reg, 1);
	if (retVal < 0) {
		Log_Debug("ERROR: platform_read(write step): errno=%d (%s)\n", errno, strerror(errno));
		return -1;
	}

	// Read the data into the provided buffer
	retVal = I2CMaster_Read(i2cFd, lsm6dsOAddress, bufp, len);
	if (retVal < 0) {
		Log_Debug("ERROR: platform_read(read step): errno=%d (%s)\n", errno, strerror(errno));
		return -1;
	}

#ifdef ENABLE_READ_WRITE_DEBUG
	Log_Debug("Read returned: ");
	for (int i = 0; i < len; i++) {
		Log_Debug("%0x: ", bufp[i]);
	}
	Log_Debug("\n\n");
#endif

	return 0;
}

/// <summary>
///     Closes the I2C interface File Descriptors.
/// </summary>
void closeI2c(void) {
	CloseFdAndPrintError(i2cFd, "i2c");
}

// sleep for a bit when needed.
void HAL_Delay(int delayTime) {
	struct timespec ts;
	ts.tv_sec = 0;
	ts.tv_nsec = delayTime * 10000;
	nanosleep(&ts, NULL);
}
