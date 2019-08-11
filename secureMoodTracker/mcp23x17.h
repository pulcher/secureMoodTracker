#pragma once

#ifndef HEADER_sd1306_H
#define HEADER_sd1306_H


#include <stdint.h>
#include "i2c.h"
#include <applibs/i2c.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>


#define mcp23x17_ADDR 0x26U

#define BUFFER_SIZE 128/8

//#define _swap(a, b) (((a) ^= (b)), ((b) ^= (a)), ((a) ^= (b)))

/**
  * @brief  Initialize mcp23x17.
  * @param  None.
  * @retval None.
  */
extern uint8_t mcp23x17_init(void);

/**
  * @brief  Send buffer to mcp23x17
  * @retval None.
  */
extern void mcp23x17_refresh(void);

/**
  * @brief  Set all buffer's bytes to zero
  * @retval None.
  */
extern void clear_mcp23x17_buffer(void);

/**
  * @brief  Set all buffer's bytes to 0xff
  * @retval None.
  */
extern void fill_mcp23x17_buffer(void);

#endif
