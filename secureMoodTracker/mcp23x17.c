/***************************************************************************************************
   Name: mcp23x17.c
   Sphere OS: 19.06
****************************************************************************************************/

#include "mcp23x17.h"

// pixel data of  screen
uint8_t mcp23x17_buffer[BUFFER_SIZE];

uint8_t mcp23x17_addr = mcp23x17_DEFAULT_ADDR;

// Lock up table to reverse byte's bits
static const uint8_t BitReverseTable256[] =
{
	0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0, 0x10, 0x90, 0x50, 0xD0, 0x30, 0xB0, 0x70, 0xF0,
	0x08, 0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8, 0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8, 0x78, 0xF8,
	0x04, 0x84, 0x44, 0xC4, 0x24, 0xA4, 0x64, 0xE4, 0x14, 0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4,
	0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC, 0x1C, 0x9C, 0x5C, 0xDC, 0x3C, 0xBC, 0x7C, 0xFC,
	0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2, 0x12, 0x92, 0x52, 0xD2, 0x32, 0xB2, 0x72, 0xF2,
	0x0A, 0x8A, 0x4A, 0xCA, 0x2A, 0xAA, 0x6A, 0xEA, 0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA,
	0x06, 0x86, 0x46, 0xC6, 0x26, 0xA6, 0x66, 0xE6, 0x16, 0x96, 0x56, 0xD6, 0x36, 0xB6, 0x76, 0xF6,
	0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE, 0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE,
	0x01, 0x81, 0x41, 0xC1, 0x21, 0xA1, 0x61, 0xE1, 0x11, 0x91, 0x51, 0xD1, 0x31, 0xB1, 0x71, 0xF1,
	0x09, 0x89, 0x49, 0xC9, 0x29, 0xA9, 0x69, 0xE9, 0x19, 0x99, 0x59, 0xD9, 0x39, 0xB9, 0x79, 0xF9,
	0x05, 0x85, 0x45, 0xC5, 0x25, 0xA5, 0x65, 0xE5, 0x15, 0x95, 0x55, 0xD5, 0x35, 0xB5, 0x75, 0xF5,
	0x0D, 0x8D, 0x4D, 0xCD, 0x2D, 0xAD, 0x6D, 0xED, 0x1D, 0x9D, 0x5D, 0xDD, 0x3D, 0xBD, 0x7D, 0xFD,
	0x03, 0x83, 0x43, 0xC3, 0x23, 0xA3, 0x63, 0xE3, 0x13, 0x93, 0x53, 0xD3, 0x33, 0xB3, 0x73, 0xF3,
	0x0B, 0x8B, 0x4B, 0xCB, 0x2B, 0xAB, 0x6B, 0xEB, 0x1B, 0x9B, 0x5B, 0xDB, 0x3B, 0xBB, 0x7B, 0xFB,
	0x07, 0x87, 0x47, 0xC7, 0x27, 0xA7, 0x67, 0xE7, 0x17, 0x97, 0x57, 0xD7, 0x37, 0xB7, 0x77, 0xF7,
	0x0F, 0x8F, 0x4F, 0xCF, 0x2F, 0xAF, 0x6F, 0xEF, 0x1F, 0x9F, 0x5F, 0xDF, 0x3F, 0xBF, 0x7F, 0xFF
};

/**
  * @brief  Send command to mcp23x17.
  * @param  addr: address of device
  * @param  cmd: commandto send
  * @retval retval: negative if was unsuccefully, positive if was succefully
  */
int32_t mcp23x17_send_command(uint8_t addr, uint8_t cmd)
{
	int32_t retval;
	uint8_t data_to_send[2];
	// Byte to tell mcp23x17 to process byte as command
	data_to_send[0] = 0x00;
	// Commando to send
	data_to_send[1] = cmd;
	// Send the data by I2C bus
	retval = I2CMaster_Write(i2cFd, addr, data_to_send, 2);
	return retval;
}

/**
  * @brief  Send data to mcp23x17 RAM.
  * @param  addr: address of device
  * @param  data: pointer to data
  * @retval retval: negative if was unsuccefully, positive if was succefully
  */
int32_t mcp23x17_write_data(uint8_t addr, uint8_t *data)
{
	int32_t retval;
	uint16_t i;
	uint8_t data_to_send[1025];
	// Byte to tell mcp23x17 to process byte as data
	data_to_send[0] = 0x40;

	// Copy data to buffer
	for (i = 0; i < 1024; i++)
	{
		data_to_send[i + 1] = data[i];
	}

	// Send the data by I2C bus
	retval = I2CMaster_Write(i2cFd, addr, data_to_send, 1025);
	return retval;
}

/**
 * Register address, port dependent, for a given PIN
 */
uint8_t mcp23x17_regForPin(uint8_t pin, uint8_t portAaddr, uint8_t portBaddr) {
	return(pin < 8) ? portAaddr : portBaddr;
}

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

uint8_t mcp23x17_init(uint8_t addr)
{
	mcp23x17_addr = addr;

	return 0;
}

/**
  * @brief  Set all buffer's bytes to zero
  * @retval None.
  */
void clear_mcp23x17_buffer()
{
	uint16_t i;

	for (i = 0; i < BUFFER_SIZE; i++)
	{
		mcp23x17_buffer[i] = 0;
	}
}


/**
  * @brief  Set all buffer's bytes to 0xff
  * @retval None.
  */

void fill_mcp23x17_buffer()
{
	uint16_t i;

	for (i = 0; i < BUFFER_SIZE; i++)
	{
		mcp23x17_buffer[i] = 0xff;
	}
}
