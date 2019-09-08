/***************************************************************************************************
   Name: mcp23x17.c
   Sphere OS: 19.06
****************************************************************************************************/

#include "mcp23x17.h"

uint8_t mcp23x17_buffer[BUFFER_SIZE];
uint8_t mcp23x17_addr = mcp23x17_DEFAULT_ADDR;

///**
//  * @brief  Send command to mcp23x17.
//  * @param  addr: address of device
//  * @param  cmd: commandto send
//  * @retval retval: negative if was unsuccefully, positive if was succefully
//  */
//int32_t mcp23x17_send_command(uint8_t addr, uint8_t cmd)
//{
//	int32_t retval;
//	uint8_t data_to_send[2];
//
//	// Byte to tell mcp23x17 to process byte as command
//	data_to_send[0] = 0x00;
//	// Commando to send
//	data_to_send[1] = cmd;
//
//	// Send the data by I2C bus
//	retval = I2CMaster_Write(i2cFd, addr, data_to_send, 2);
//	return retval;
//}

/**
//  * @brief  Send data to mcp23x17 RAM.
//  * @param  addr: address of device
//  * @param  data: pointer to data
//  * @retval retval: negative if was unsuccefully, positive if was succefully
//  */
//int32_t mcp23x17_write_data(uint8_t addr, uint8_t *data)
//{
//	int32_t retval;
//	uint16_t i;
//	uint8_t data_to_send[1025];
//
//	// Byte to tell mcp23x17 to process byte as data
//	data_to_send[0] = 0x40;
//
//	// Copy data to buffer
//	for (i = 0; i < 1024; i++)
//	{
//		data_to_send[i + 1] = data[i];
//	}
//
//	// Send the data by I2C bus
//	retval = I2CMaster_Write(i2cFd, addr, data_to_send, 1025);
//	return retval;
//}

///**
// * Register address, port dependent, for a given PIN
// */
//uint8_t mcp23x17_regForPin(uint8_t pin, uint8_t portAaddr, uint8_t portBaddr) {
//	return(pin < 8) ? portAaddr : portBaddr;
//}

///**
// * Reads a given register
// */
//uint8_t mcp23x17_readRegister(uint8_t addr) {
//	// read the current GPINTEN
//	Wire.beginTransmission(mcp23x17_DEFAULT_ADDR | mcp23x17_addr);
//	wiresend(addr);
//	Wire.endTransmission();
//	Wire.requestFrom(mcp23x17_DEFAULT_ADDR | mcp23x17_addr, 1);
//	return wirerecv();
//}

/**
  * @brief  Initialize mcp23x17.
  * @param  None.
  * @retval Positive if was unsuccefully, zero if was succefully.
  */
//uint8_t mcp23x17_init(uint8_t addr)
//{
//	//// OLED turn off and check if OLED is connected
//	//if (mcp23x17_send_command(mcp23x17_ADDR, 0xae) < 0)
//	//{
//	//	return 1;
//	//}
//	//// Set display oscillator freqeuncy and divide ratio
//	//mcp23x17_send_command(mcp23x17_ADDR, 0xd5);
//	//mcp23x17_send_command(mcp23x17_ADDR, 0x50);
//
//	//// Set multiplex ratio
//	//mcp23x17_send_command(mcp23x17_ADDR, 0xa8);
//	//mcp23x17_send_command(mcp23x17_ADDR, 0x3f);
//	//// Set display start line
//	//mcp23x17_send_command(mcp23x17_ADDR, 0xd3);
//	//mcp23x17_send_command(mcp23x17_ADDR, 0x00);
//	//// Set the lower comulmn address
//	//mcp23x17_send_command(mcp23x17_ADDR, 0x00);
//	//// Set the higher comulmn address
//	//mcp23x17_send_command(mcp23x17_ADDR, 0x10);
//
//	//// Set page address
//	//mcp23x17_send_command(mcp23x17_ADDR, 0xb0);
//
//	//// Charge pump
//	//mcp23x17_send_command(mcp23x17_ADDR, 0x8d);
//	//mcp23x17_send_command(mcp23x17_ADDR, 0x14);
//
//	//// Memory mode
//	//mcp23x17_send_command(mcp23x17_ADDR, 0x20);
//	//mcp23x17_send_command(mcp23x17_ADDR, 0x00);
//
//	//// Set segment from left to right
//	//mcp23x17_send_command(mcp23x17_ADDR, 0xa0 | 0x01);
//	//// Set OLED upside up
//	//mcp23x17_send_command(mcp23x17_ADDR, 0xc8);
//	//// Set common signal pad configuration
//	//mcp23x17_send_command(mcp23x17_ADDR, 0xda);
//	//mcp23x17_send_command(mcp23x17_ADDR, 0x12);
//
//	//// Set Contrast
//	//mcp23x17_send_command(mcp23x17_ADDR, 0x81);
//	//// Contrast data
//	//mcp23x17_send_command(mcp23x17_ADDR, 0x00);
//
//	//// Set discharge precharge periods
//	//mcp23x17_send_command(mcp23x17_ADDR, 0xd9);
//	//mcp23x17_send_command(mcp23x17_ADDR, 0xf1);
//
//	//// Set common mode pad output voltage
//	//mcp23x17_send_command(mcp23x17_ADDR, 0xdb);
//	//mcp23x17_send_command(mcp23x17_ADDR, 0x40);
//
//	//// Set Enire display
//	//mcp23x17_send_command(mcp23x17_ADDR, 0xa4);
//
//	//// Set Normal display
//	//mcp23x17_send_command(mcp23x17_ADDR, 0xa6);
//	//// Stop scroll
//	//mcp23x17_send_command(mcp23x17_ADDR, 0x2e);
//
//	//// OLED turn on
//	//mcp23x17_send_command(mcp23x17_ADDR, 0xaf);
//
//	//// Set column address
//	//mcp23x17_send_command(mcp23x17_ADDR, 0x21);
//	//// Start Column
//	//mcp23x17_send_command(mcp23x17_ADDR, 0x00);
//	//// Last column
//	//mcp23x17_send_command(mcp23x17_ADDR, 127);
//
//	//// Set page address
//	//mcp23x17_send_command(mcp23x17_ADDR, 0x22);
//	//// Start Page
//	//mcp23x17_send_command(mcp23x17_ADDR, 0x00);
//	//// Last Page
//	//mcp23x17_send_command(mcp23x17_ADDR, 0x07);
//
//	return 0;
//}

/**
  * @brief  DeviceWhoamI[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  buff     buffer that stores data read
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t mcp23x17_device_id_get(mcp23x17_ctx_t* ctx, uint8_t* buff)
{
	int32_t ret;
	ret = mcp23x17_read_reg(ctx, MCP23X17_WHO_AM_I, buff, 1);
	return ret;
}

/**
  * @brief  Read generic device register
  *
  * @param  ctx   read / write interface definitions(ptr)
  * @param  reg   register to read
  * @param  data  pointer to buffer that store the data read(ptr)
  * @param  len   number of consecutive register to read
  * @retval       interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t mcp23x17_read_reg(mcp23x17_ctx_t* ctx, uint8_t reg, uint8_t* data,
	uint16_t len)
{
	int32_t ret;
	ret = ctx->read_reg(ctx, reg, data, len);
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
static int32_t mcp23x17_read_ctx(mcp23x17_ctx_t* ctx, uint8_t reg, uint8_t* data, uint8_t len)
{
	ssize_t ret;
	int test = *((int*)ctx->handle);
	const uint8_t command[] = { reg, data };

	// Send the data by I2C bus
	// tried todo this the applibs way instead of the posix way.  I am sure I missed a memo....
	int targetSet = I2CMaster_SetDefaultTargetAddress(*((int*)ctx->handle), mcp23x17_DEFAULT_ADDR);
	int targetRegisterWrite = write(*((int*)ctx->handle), &reg, len);
	ssize_t readRet = read(*((int*)ctx->handle), data, len);
	ret = readRet;

#ifdef ENABLE_READ_WRITE_DEBUG
	Log_Debug("Read %d bytes: ", len);
	for (int i = 0; i < len; i++) {
		Log_Debug("[%0x] ", data[i]);
	}
	Log_Debug("\n", len);
#endif

	return ret;
}

/**
  * @brief  Write generic device register
  *
  * @param  ctx   read / write interface definitions(ptr)
  * @param  reg   register to write
  * @param  data  pointer to data to write in register reg(ptr)
  * @param  len   number of consecutive register to write
  * @retval       interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t mcp23x17_write_reg(mcp23x17_ctx_t* ctx, uint8_t reg, uint8_t* data,
	uint16_t len)
{
	int32_t ret;
	ret = ctx->write_reg(ctx, reg, data, len);
	return ret;
}

/*
 * @brief  Write mcp23x17 device register
 *
 * @param  handle    customizable argument. In this examples is used in
 *                   order to select the correct sensor bus handler.
 * @param  reg       register to write
 * @param  bufp      pointer to data to write in register reg
 * @param  len       number of consecutive register to write
 *
 */
static uint8_t mcp23x17_write_ctx(mcp23x17_ctx_t* ctx, uint8_t reg, uint8_t* data, uint8_t len)
{
	ssize_t ret;
	const uint8_t command[] = { reg, *data };

	ret = I2CMaster_Write(*((int*)ctx->handle), mcp23x17_DEFAULT_ADDR, command, sizeof(command));

	return ret;
}

uint8_t mcp23x17_init(int* i2cFd, uint8_t addr)
{
	mcp23x17_status = 1;

	mcp23x17_ctx.handle = i2cFd;
	mcp23x17_ctx.read_reg = mcp23x17_read_ctx;
	mcp23x17_ctx.write_reg = mcp23x17_write_ctx;

	if (addr != 0) {			// if the address is not the default.
		mcp23x17_addr = addr;
	}

	return 0;
}
