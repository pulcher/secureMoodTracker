#pragma once

#ifndef HEADER_mcp23x17_H
#define HEADER_mcp23x17_H


#include <stdint.h>
#include "i2c.h"
#include <applibs/i2c.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

// registers
#define MCP23017_IODIRA 0x00
#define MCP23017_IPOLA 0x02
#define MCP23017_GPINTENA 0x04
#define MCP23017_DEFVALA 0x06
#define MCP23017_INTCONA 0x08
#define MCP23017_IOCONA 0x0A
#define MCP23017_GPPUA 0x0C
#define MCP23017_INTFA 0x0E
#define MCP23017_INTCAPA 0x10
#define MCP23017_GPIOA 0x12
#define MCP23017_OLATA 0x14


#define MCP23017_IODIRB 0x01
#define MCP23017_IPOLB 0x03
#define MCP23017_GPINTENB 0x05
#define MCP23017_DEFVALB 0x07
#define MCP23017_INTCONB 0x09
#define MCP23017_IOCONB 0x0B
#define MCP23017_GPPUB 0x0D
#define MCP23017_INTFB 0x0F
#define MCP23017_INTCAPB 0x11
#define MCP23017_GPIOB 0x13
#define MCP23017_OLATB 0x15

#define MCP23017_INT_ERR 255

#define mcp23x17_DEFAULT_ADDR 0x20U

#define BUFFER_SIZE 128/8

#define MCP23X17_WHO_AM_I 0x00U
#define MCP23X17_DEFAULT_HIGH 0xffU

typedef int32_t(*mcp23x17_write_ptr)(void*, uint8_t, uint8_t*, uint16_t);
typedef int32_t(*mcp23x17_read_ptr) (void*, uint8_t, uint8_t*, uint16_t);

typedef struct {
	mcp23x17_write_ptr  write_reg;
	mcp23x17_read_ptr   read_reg;
	void* handle;
} mcp23x17_ctx_t;

mcp23x17_ctx_t mcp23x17_ctx;
uint8_t mcp23x17_status;

int32_t mcp23x17_read_reg(mcp23x17_ctx_t* ctx, uint8_t reg, uint8_t* data, uint16_t len);
int32_t mcp23x17_write_reg(mcp23x17_ctx_t* ctx, uint8_t reg, uint8_t* data, uint16_t len);

int32_t mcp23x17_device_id_get(mcp23x17_ctx_t* ctx, uint8_t* buff);

/**
  * @brief  Initialize mcp23x17.
  * @param  None.
  * @retval None.
  */
extern uint8_t mcp23x17_init(int* i2cFd, uint8_t addr);


//	void pinMode(uint8_t p, uint8_t d);
//	void digitalWrite(uint8_t p, uint8_t d);
//	void pullUp(uint8_t p, uint8_t d);
//	uint8_t digitalRead(uint8_t p);
//
//	void writeGPIOAB(uint16_t);
//	uint16_t readGPIOAB();
//	uint8_t readGPIO(uint8_t b);
//
//	void setupInterrupts(uint8_t mirroring, uint8_t open, uint8_t polarity);
//	void setupInterruptPin(uint8_t p, uint8_t mode);
//	uint8_t getLastInterruptPin();
//	uint8_t getLastInterruptPinValue();
//
//private:
//	uint8_t bitForPin(uint8_t pin);
//	uint8_t regForPin(uint8_t pin, uint8_t portAaddr, uint8_t portBaddr);
//
//	uint8_t readRegister(uint8_t addr);
//	void writeRegister(uint8_t addr, uint8_t value);
//
//	/**
//	 * Utility private method to update a register associated with a pin (whether port A/B)
//	 * reads its value, updates the particular bit, and writes its value.
//	 */
//	void updateRegisterBit(uint8_t p, uint8_t pValue, uint8_t portAaddr, uint8_t portBaddr);
//
#endif
