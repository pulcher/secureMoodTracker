#pragma once

#include <stdbool.h>
#include "epoll_timerfd_utilities.h"

uint8_t whoami;

// the FH to use in other files
extern int i2cFd;

// functions
int initI2c(void);
void closeI2c(void);

// sleep for a bit when needed.
void HAL_Delay(int delayTime);
