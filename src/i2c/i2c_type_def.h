#ifndef I2C_TYPE_DEF_H
#define I2C_TYPE_DEF_H

#include "../hal/hal.h"

#define I2C_ADDRESS_MAXIMUM (120) // Exclude reserved
#define I2C_ADDRESS_MINIMUM (8)   // Exclude part of reserved
/*******************************************************
# I2C reserved addresses

0000 000 0 : general call address
0000 000 1 : Start byte
0000 001 X : CBUS address
0000 010 X : reserved for different bus format
0000 011 X : reserved for future purposes
0000 1XX X : Hs-mode controller code
1111 1XX 1 : device ID
*******************************************************/

struct I2CActiveDevice {
	uint8_t port;        // Wire bus index
	uint8_t address;     // Slave address
	uint8_t sub_address; // Slave address via multiple
	int8_t channel;		 // Multiplexer channel
};

enum I2C_DATA_RESULT {
	I2C_RES_FAILED = 0b00000000,
	I2C_RES_DOUBLE = 0b00000001,
	I2C_RES_INT    = 0b00000010,
	I2C_RES_STRING = 0b00000100
};

enum I2C_INIT_RESULT {
	I2C_IGNORE_DEVICE = -1,
	I2C_0_DATA_DEVICE = 0,
	I2C_1_DATA_DEVICE = 1,
	I2C_2_DATA_DEVICE = 2,
	I2C_3_DATA_DEVICE = 3,
	I2C_4_DATA_DEVICE = 4,
	I2C_5_DATA_DEVICE = 5,
	I2C_6_DATA_DEVICE = 6,
	I2C_7_DATA_DEVICE = 7,
	I2C_8_DATA_DEVICE = 8,
};

#endif