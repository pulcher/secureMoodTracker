#pragma once

#include <stdbool.h>
#include "epoll_timerfd_utilities.h"

int initI2c(void);
void closeI2c(void);

// the FH to use in other files
extern int i2cFd;
