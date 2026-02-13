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
	I2C_9_DATA_DEVICE = 9,
	I2C_10_DATA_DEVICE = 10,
	I2C_11_DATA_DEVICE = 11,
	I2C_12_DATA_DEVICE = 12,
	I2C_13_DATA_DEVICE = 13,
	I2C_14_DATA_DEVICE = 14,
	I2C_15_DATA_DEVICE = 15,
	I2C_16_DATA_DEVICE = 16,
	I2C_17_DATA_DEVICE = 17,
	I2C_18_DATA_DEVICE = 18,
	I2C_19_DATA_DEVICE = 19,
	I2C_20_DATA_DEVICE = 20,
	I2C_21_DATA_DEVICE = 21,
	I2C_22_DATA_DEVICE = 22,
	I2C_23_DATA_DEVICE = 23,
	I2C_24_DATA_DEVICE = 24,
	I2C_25_DATA_DEVICE = 25,
	I2C_26_DATA_DEVICE = 26,
	I2C_27_DATA_DEVICE = 27,
	I2C_28_DATA_DEVICE = 28,
	I2C_29_DATA_DEVICE = 29,
	I2C_30_DATA_DEVICE = 30,
};

#endif