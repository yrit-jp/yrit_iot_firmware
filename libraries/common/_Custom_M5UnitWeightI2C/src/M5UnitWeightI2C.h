#ifndef _M5UnitWeightI2C_H_
#define _M5UnitWeightI2C_H_

#include <Arduino.h>
#include <Wire.h>

// I2C
#define DEVICE_DEFAULT_ADDR 0x26

// SCALES REGISTER
#define WEIGHT_I2C_RAW_ADC_REG         0x00
#define WEIGHT_I2C_CAL_DATA_REG        0x10
#define WEIGHT_I2C_SET_GAP_REG         0x40
#define WEIGHT_I2C_SET_OFFESET_REG     0x50
#define WEIGHT_I2C_CAL_DATA_INT_REG    0x60
#define WEIGHT_I2C_CAL_DATA_STRING_REG 0x70
#define WEIGHT_I2C_FILTER_REG          0x80
#define JUMP_TO_BOOTLOADER_REG         0xFD
#define FIRMWARE_VERSION_REG           0xFE
#define I2C_ADDRESS_REG                0xFF

class M5UnitWeightI2C {
   private:
    uint8_t _addr;
    TwoWire *_wire;
    uint8_t _sda;
    uint8_t _scl;
    uint32_t _speed;
    bool writeBytes(uint8_t addr, uint8_t reg, uint8_t *buffer, uint8_t length);
    bool readBytes(uint8_t addr, uint8_t reg, uint8_t *buffer, uint8_t length);

   public:
    bool begin(TwoWire *wire = &Wire, uint8_t sda = 21, uint8_t scl = 22,
               uint8_t addr = DEVICE_DEFAULT_ADDR, uint32_t speed = 100000U);
    // SCALES
    int32_t getRawADC();
    float getWeight();
    int32_t getWeightInt();
    String getWeightString();
    float getGapValue();
    void setGapValue(float offset);
    void setOffset(void);
    // RGB LED CTL
    // uint8_t status();
    bool setLEDColor(uint32_t color);
    uint32_t getLEDColor();
    // BUTTON
    uint8_t getBtnStatus();
    uint8_t getFirmwareVersion(void);
    void jumpBootloader(void);
    uint8_t setI2CAddress(uint8_t addr);
    uint8_t getI2CAddress(void);
    bool setLPFilter(uint8_t en);
    uint8_t getLPFilter(void);
    bool setAvgFilter(uint8_t avg);
    uint8_t getAvgFilter(void);
    bool setEmaFilter(uint8_t ema);
    uint8_t getEmaFilter(void);
};

#endif
