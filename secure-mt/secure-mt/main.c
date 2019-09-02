﻿#include <signal.h>
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

// Timer / polling
static int buttonPollTimerFd = -1;
static int azureTimerFd = -1;
static int epollFd = -1;


int main(void)
{
	// Main loop
	while (!terminationRequired) {
		if (WaitForEventAndCallHandler(epollFd) != 0) {
			terminationRequired = true;
		}
	}
}