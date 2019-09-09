#pragma once

#include <stdbool.h>
#include "epoll_timerfd_utilities.h"

uint8_t whoami;
uint8_t rst;

// the FH to use in other files
extern int i2cFd;

// functions
int initI2c(void);
void closeI2c(void);
static int32_t lsm6dso_write_lps22hh_cx(void* ctx, uint8_t reg, uint8_t* data, uint16_t len);
static int32_t lsm6dso_read_lps22hh_cx(void* ctx, uint8_t reg, uint8_t* data, uint16_t len);

// Routines to read/write to the LSM6DSO device
static int32_t platform_write(int* fD, uint8_t reg, uint8_t* bufp, uint16_t len);
static int32_t platform_read(int* fD, uint8_t reg, uint8_t* bufp, uint16_t len);

// sleep for a bit when needed.
void HAL_Delay(int delayTime);
