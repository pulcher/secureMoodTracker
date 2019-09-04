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

#include "i2c.h";

// extern vars
int i2cFd = -1;		// i2c file handle
extern int epollFd;
extern volatile sig_atomic_t terminationRequired;

// sleep for a bit when needed.
void HAL_Delay(int delayTime) {
	struct timespec ts;
	ts.tv_sec = 0;
	ts.tv_nsec = delayTime * 10000;
	nanosleep(&ts, NULL);
}

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

	return 0;
}

/// <summary>
///     Closes the I2C interface File Descriptors.
/// </summary>
void closeI2c(void) {
	CloseFdAndPrintError(i2cFd, "i2c");
}
