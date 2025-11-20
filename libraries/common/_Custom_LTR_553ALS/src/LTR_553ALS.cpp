/*
*****************************************************************************
@file         Deneyap_MesafeOlcerIsikAlgilayici.cpp
@mainpage     Deneyap Proximity&Light Sensor LTR553 Arduino library source file
@maintainer   RFtek Electronics <techsupport@rftek.com.tr>
@version      v1.0.0
@date         June 24, 2022
@brief        Includes functions to control Deneyap Proximity&Light Sensor LTR553
              Arduino library
@license      MIT License
@modified     Feb 27, 2025 by takkaO
Library includes:
--> Configuration functions
--> Data manipulation functions
--> I2C communication functions
*****************************************************************************
*/

#include "LTR_553ALS.h"

#define PS_ACTIVE_MODE 0x03
#define PS_STANDBY_MODE 0x00

/*===========================================================================*/

#define PS_LED_PULSE_FREQUENCY_30KHZ 0x00
#define PS_LED_PULSE_FREQUENCY_40KHZ 0x20
#define PS_LED_PULSE_FREQUENCY_50KHZ 0x40
#define PS_LED_PULSE_FREQUENCY_60KHZ 0x60
#define PS_LED_PULSE_FREQUENCY_70KHZ 0x80
#define PS_LED_PULSE_FREQUENCY_80KHZ 0xA0
#define PS_LED_PULSE_FREQUENCY_90KHZ 0xC0
#define PS_LED_PULSE_FREQUENCY_100KHZ 0xE0

#define PS_LED_DUTY_CYCLE_25 0x00
#define PS_LED_DUTY_CYCLE_50 0x08
#define PS_LED_DUTY_CYCLE_75 0x10
#define PS_LED_DUTY_CYCLE_100 0x18

#define PS_LED_PEAK_CURRENT_5MA 0x00
#define PS_LED_PEAK_CURRENT_10MA 0x01
#define PS_LED_PEAK_CURRENT_20MA 0x02
#define PS_LED_PEAK_CURRENT_50MA 0x03
#define PS_LED_PEAK_CURRENT_100MA 0x04

#define PS_LED_PULSES_1 0x01
#define PS_LED_PULSES_2 0x02
#define PS_LED_PULSES_3 0x03
#define PS_LED_PULSES_4 0x04
#define PS_LED_PULSES_5 0x05
#define PS_LED_PULSES_6 0x06
#define PS_LED_PULSES_7 0x07
#define PS_LED_PULSES_8 0x08
#define PS_LED_PULSES_9 0x09
#define PS_LED_PULSES_10 0x0A
#define PS_LED_PULSES_11 0x0B
#define PS_LED_PULSES_12 0x0C
#define PS_LED_PULSES_13 0x0D
#define PS_LED_PULSES_14 0x0E
#define PS_LED_PULSES_15 0x0F

#define PS_MEASUREMENT_REPEAT_RATE_10MS 0x08
#define PS_MEASUREMENT_REPEAT_RATE_50MS 0x00
#define PS_MEASUREMENT_REPEAT_RATE_70MS 0x01
#define PS_MEASUREMENT_REPEAT_RATE_100MS 0x02
#define PS_MEASUREMENT_REPEAT_RATE_200MS 0x03
#define PS_MEASUREMENT_REPEAT_RATE_500MS 0x04
#define PS_MEASUREMENT_REPEAT_RATE_1000MS 0x05
#define PS_MEASUREMENT_REPEAT_RATE_2000MS 0x06

#define ALS_ACTIVE_MODE 0x01
#define ALS_STANDBY_MODE 0x00

#define ALS_GAIN_1X 0x00
#define ALS_GAIN_2X 0x04
#define ALS_GAIN_4X 0x08
#define ALS_GAIN_8X 0x0C
#define ALS_GAIN_48X 0x18
#define ALS_GAIN_96X 0x1C

#define ALS_SOFTWARE_RESET_DISABLE 0x00
#define ALS_SOFTWARE_RESET_ENABLE 0x02

#define ALS_INTEGRATION_TIME_50MS 0x08
#define ALS_INTEGRATION_TIME_100MS 0x00
#define ALS_INTEGRATION_TIME_150MS 0x20
#define ALS_INTEGRATION_TIME_200MS 0x10
#define ALS_INTEGRATION_TIME_250MS 0x28
#define ALS_INTEGRATION_TIME_300MS 0x30
#define ALS_INTEGRATION_TIME_350MS 0x38
#define ALS_INTEGRATION_TIME_400MS 0x18

#define ALS_MEAS_REPEAT_RATE_50MS 0x00
#define ALS_MEAS_REPEAT_RATE_100MS 0x01
#define ALS_MEAS_REPEAT_RATE_200MS 0x02
#define ALS_MEAS_REPEAT_RATE_500MS 0x03
#define ALS_MEAS_REPEAT_RATE_1000MS 0x04
#define ALS_MEAS_REPEAT_RATE_2000MS 0x05

/*===========================================================================*/

#define ALS_MODE_MASK 0x01
#define ALS_SOFTWARE_RESET_MASK 0x02
#define ALS_GAIN_MASK 0x1C
#define ALS_INTEGRATION_TIME_MASK 0x38
#define ALS_MEAS_REPEAT_RATE_MASK 0x07

#define ALS_CH1_DATA_LOW_MASK 0xF0
#define ALS_CH1_DATA_MID_MASK 0xFF
#define ALS_CH1_DATA_HIGH_MASK 0xFF

#define ALS_CH2_DATA_LOW_MASK 0xF0
#define ALS_CH2_DATA_MID_MASK 0xFF
#define ALS_CH2_DATA_HIGH_MASK 0xFF

#define PS_MODE_MASK 0x03
#define PS_SATURATION_INDICATOR_MASK 0x20
#define PS_LED_DUTY_CYCLE_MASK 0x18
#define PS_LED_PEAK_CURRENT_MASK 0x07
#define PS_LED_PULSE_FREQUENCY_MASK 0xE0

#define PS_DATA_LOW_MASK 0xFF
#define PS_DATA_HIGH_MASK 0x07

#define PART_NUMBER_ID_MASK 0xF0
#define REVISION_ID_MASK 0x0F

/*===========================================================================*/

#define GAIN_1X 1
#define GAIN_2X 2
#define GAIN_4X 4
#define GAIN_8X 8
#define GAIN_48X 48
#define GAIN_96X 96

#define INTEGRATION_TIME_100 1
#define INTEGRATION_TIME_150 1.5
#define INTEGRATION_TIME_200 2
#define INTEGRATION_TIME_250 2.5
#define INTEGRATION_TIME_300 3
#define INTEGRATION_TIME_350 3.5
#define INTEGRATION_TIME_400 4

/*===========================================================================*/

namespace LTR553_REG
{
    constexpr uint8_t LTR553_ALS_CONTR = 0x80;
    constexpr uint8_t LTR553_PS_CONTR = 0x81;
    constexpr uint8_t LTR553_PS_LED = 0x82;
    constexpr uint8_t LTR553_PS_N_PULSES = 0x83;
    constexpr uint8_t LTR553_PS_MEAS_RATE = 0x84;
    constexpr uint8_t LTR553_ALS_MEAS_PATE = 0x85;
    constexpr uint8_t LTR553_PART_ID = 0x86;
    constexpr uint8_t LTR553_MANUFAC_ID = 0x87;
    constexpr uint8_t LTR553_ALS_DATA_CH1_0 = 0x88;
    constexpr uint8_t LTR553_ALS_DATA_CH1_1 = 0x89;
    constexpr uint8_t LTR553_ALS_DATA_CH0_0 = 0x8A;
    constexpr uint8_t LTR553_ALS_DATA_CH0_1 = 0x8B;
    constexpr uint8_t LTR553_ALS_PS_STATUS = 0x8C;
    constexpr uint8_t LTR553_PS_DATA_0 = 0x8D;
    constexpr uint8_t LTR553_PS_DATA_1 = 0x8E;
    constexpr uint8_t LTR553_INTERRUPT = 0x8F;
    constexpr uint8_t LTR553_PS_THRES_UP_0 = 0x90;
    constexpr uint8_t LTR553_PS_THRES_UP_1 = 0x91;
    constexpr uint8_t LTR553_PS_THRES_LOW_0 = 0x92;
    constexpr uint8_t LTR553_PS_THRES_LOW_1 = 0x93;
    constexpr uint8_t LTR553_PS_OFFSET_1 = 0x94;
    constexpr uint8_t LTR553_PS_OFFSET_0 = 0x95;
    constexpr uint8_t LTR553_ALS_THRES_UP_0 = 0x97;
    constexpr uint8_t LTR553_ALS_THRES_UP_1 = 0x98;
    constexpr uint8_t LTR553_ALS_THRES_LOW_0 = 0x99;
    constexpr uint8_t LTR553_ALS_THRES_LOW_1 = 0x9A;
    constexpr uint8_t LTR553_INTERRUPT_PERSIST = 0x9E;
}

/*===========================================================================*/

/**
 * @brief  I2C initialization and read check
 * @param  adress: Device adress
 * @retval None
 **/
bool LTR_553ALS::begin(uint8_t address)
{

    _address = address;
    softwareReset();

    if (getPartNumberID() != 0x09)
    {
        Serial.printf("\nPN: 0x%02X\n", getPartNumberID());
        return false;
    }

    if (getRevisionID() != 0x02)
    {
        Serial.printf("\nRID: 0x%02X\n", getRevisionID());
        return false;
    }


    if (getManufacturerID() != 0x05)
    {
        Serial.printf("\nMID: 0x%02X\n", getManufacturerID());
        return false;
    }

    setPSledPulse(PS_LED_PULSES_15);
    setALSmode(ALS_ACTIVE_MODE);
    setPSmode(PS_ACTIVE_MODE);

    return true;
}

/**
 * @brief
 * @param
 * @retval
 **/
void LTR_553ALS::setALSmode(uint8_t mode)
{
    uint8_t readValue, writeValue;
    readRegs(LTR553_REG::LTR553_ALS_CONTR, &readValue, 1);
    readValue &= (~ALS_MODE_MASK);
    writeValue = readValue | mode;
    writeRegs(LTR553_REG::LTR553_ALS_CONTR, writeValue);
}

/**
 * @brief
 * @param
 * @retval
 **/
void LTR_553ALS::setPSmode(uint8_t mode)
{
    uint8_t readValue, writeValue;
    readRegs(LTR553_REG::LTR553_PS_CONTR, &readValue, 1);
    readValue &= (~PS_MODE_MASK);
    writeValue = readValue | mode;
    writeRegs(LTR553_REG::LTR553_PS_CONTR, writeValue);
}

/**
 * @brief
 * @param
 * @retval
 **/
void LTR_553ALS::softwareReset(void)
{
    uint8_t readValue, writeValue;
    readRegs(LTR553_REG::LTR553_ALS_CONTR, &readValue, 1);
    readValue &= (~ALS_SOFTWARE_RESET_MASK);
    writeValue = readValue | 0x02;
    writeRegs(LTR553_REG::LTR553_ALS_CONTR, writeValue);
}

/**
 * @brief
 * @param
 * @retval
 **/
void LTR_553ALS::setALSgain(uint8_t gain)
{
    uint8_t readValue, writeValue;
    readRegs(LTR553_REG::LTR553_ALS_CONTR, &readValue, 1);
    readValue &= (~ALS_GAIN_MASK);
    writeValue = readValue | gain;
    writeRegs(LTR553_REG::LTR553_ALS_CONTR, writeValue);
}

/**
 * @brief
 * @param
 * @retval
 **/
uint8_t LTR_553ALS::getALSgain(void)
{
    uint8_t readValue, result;
    readRegs(LTR553_REG::LTR553_ALS_CONTR, &readValue, 1);
    readValue &= ALS_GAIN_MASK;
    return result = readValue;
}

/**
 * @brief
 * @param
 * @retval
 **/
void LTR_553ALS::setPSsaturationIndicator(uint8_t strIndicator)
{
    uint8_t readValue, writeValue;
    readRegs(LTR553_REG::LTR553_PS_CONTR, &readValue, 1);
    readValue &= (~PS_SATURATION_INDICATOR_MASK);
    writeValue = readValue | strIndicator;
    writeRegs(LTR553_REG::LTR553_PS_CONTR, writeValue);
}

/**
 * @brief
 * @param
 * @retval
 **/
void LTR_553ALS::setPSledPulseFreq(uint8_t ledPulseFreq)
{
    uint8_t readValue, writeValue;
    readRegs(LTR553_REG::LTR553_PS_LED, &readValue, 1);
    readValue &= (~PS_LED_PULSE_FREQUENCY_MASK);
    writeValue = readValue | ledPulseFreq;
    writeRegs(LTR553_REG::LTR553_PS_LED, writeValue);
}

/**
 * @brief
 * @param
 * @retval
 **/
void LTR_553ALS::setPSledDutyCycle(uint8_t ledDutyCycle)
{
    uint8_t readValue, writeValue;
    readRegs(LTR553_REG::LTR553_PS_LED, &readValue, 1);
    readValue &= (~PS_LED_DUTY_CYCLE_MASK);
    writeValue = readValue | ledDutyCycle;
    writeRegs(LTR553_REG::LTR553_PS_LED, writeValue);
}

/**
 * @brief
 * @param
 * @retval
 **/
void LTR_553ALS::setPSledPeakCurrent(uint8_t ledPeakCurrent)
{
    uint8_t readValue, writeValue;
    readRegs(LTR553_REG::LTR553_PS_LED, &readValue, 1);
    readValue &= (~PS_LED_PEAK_CURRENT_MASK);
    writeValue = readValue | ledPeakCurrent;
    writeRegs(LTR553_REG::LTR553_PS_LED, writeValue);
}

/**
 * @brief
 * @param
 * @retval
 **/
void LTR_553ALS::setPSledPulse(uint8_t ledPulse)
{
    uint8_t registerValue;
    registerValue = ledPulse;
    writeRegs(LTR553_REG::LTR553_PS_N_PULSES, registerValue);
}

/**
 * @brief
 * @param
 * @retval
 **/
void LTR_553ALS::setPSmeasurementRate(uint8_t measRate)
{
    uint8_t registerValue;
    registerValue = measRate;
    writeRegs(LTR553_REG::LTR553_PS_MEAS_RATE, registerValue);
}

/**
 * @brief
 * @param
 * @retval
 **/
void LTR_553ALS::setALSintegrationTime(uint8_t intTime)
{
    uint8_t readValue, writeValue;
    readRegs(LTR553_REG::LTR553_ALS_MEAS_PATE, &readValue, 1);
    readValue &= (~ALS_INTEGRATION_TIME_MASK);
    writeValue = readValue | intTime;
    writeRegs(LTR553_REG::LTR553_ALS_MEAS_PATE, writeValue);
}

/**
 * @brief
 * @param
 * @retval
 **/
uint8_t LTR_553ALS::getALSintegrationTime(void)
{
    uint8_t readValue, result;
    readRegs(LTR553_REG::LTR553_ALS_MEAS_PATE, &readValue, 1);
    readValue &= ALS_INTEGRATION_TIME_MASK;
    return result = readValue;
}

/**
 * @brief
 * @param
 * @retval
 **/
void LTR_553ALS::setALSmeasurementRate(uint8_t measRate)
{
    uint8_t readValue, writeValue;
    readRegs(LTR553_REG::LTR553_ALS_MEAS_PATE, &readValue, 1);
    readValue &= (~ALS_MEAS_REPEAT_RATE_MASK);
    writeValue = readValue | measRate;
    writeRegs(LTR553_REG::LTR553_ALS_MEAS_PATE, writeValue);
}

/**
 * @brief
 * @param
 * @retval
 **/
uint8_t LTR_553ALS::getPartNumberID(void)
{
    uint8_t readValue, result;
    readRegs(LTR553_REG::LTR553_PART_ID, &readValue, 1);
    readValue &= PART_NUMBER_ID_MASK;
    return result = readValue >> 4;
}

/**
 * @brief
 * @param
 * @retval
 **/
uint8_t LTR_553ALS::getRevisionID(void)
{
    uint8_t readValue, result;
    readRegs(LTR553_REG::LTR553_PART_ID, &readValue, 1);
    readValue &= REVISION_ID_MASK;
    return result = readValue;
}

/**
 * @brief
 * @param
 * @retval
 **/
uint8_t LTR_553ALS::getManufacturerID(void)
{
    uint8_t readValue, result;
    readRegs(LTR553_REG::LTR553_MANUFAC_ID, &readValue, 1);
    return result = readValue;
}

/**
 * @brief
 * @param
 * @retval
 **/
uint16_t LTR_553ALS::getPSvalue(void)
{
    uint8_t buffer[2];
    uint16_t result;
    readRegs(LTR553_REG::LTR553_PS_DATA_0, &buffer[0], 2);
    buffer[0] &= PS_DATA_LOW_MASK;
    buffer[1] &= PS_DATA_HIGH_MASK;
    result = (buffer[1] << 8) | buffer[0];
    return result;
}

/**
 * @brief
 * @param
 * @retval
 **/
uint16_t LTR_553ALS::getALSCH0value(void)
{
    uint8_t buffer[2];
    uint32_t result;
    readRegs(LTR553_REG::LTR553_ALS_DATA_CH0_0, &buffer[0], 2);
    result = (buffer[1] << 8) | buffer[0];
    return result;
}

/**
 * @brief
 * @param
 * @retval
 **/
uint16_t LTR_553ALS::getALSCH1value(void)
{
    uint8_t buffer[2];
    uint32_t result;
    readRegs(LTR553_REG::LTR553_ALS_DATA_CH1_0, &buffer[0], 2);
    result = (buffer[1] << 8) | buffer[0];
    return result;
}

/**
 * @brief
 * @param
 * @retval
 **/
float LTR_553ALS::getLuxValue(void)
{
    uint16_t CH0, CH1;
    float ratio;
    CH1 = getALSCH1value();
    CH0 = getALSCH0value();

    ratio = ((float)(CH1 / ((float)CH1 + CH0)));

    if (ratio < 0.45)
        return ((float)1.7743 * CH0) + ((float)1.1059 * CH1);

    else if (0.45 <= ratio && ratio < 0.64)
        return ((float)4.2785 * CH0) - ((float)1.9548 * CH1);

    else if (0.64 <= ratio && ratio < 0.85)
        return ((float)0.5926 * CH0) + ((float)0.1185 * CH1);

    else
        return 0;
}

/**
 * @brief
 * @param
 * @retval
 **/
void LTR_553ALS::writeRegs(int registerAddress, uint8_t dataValue)
{
    _wire->beginTransmission(_address);
    _wire->write(registerAddress);
    _wire->write(dataValue);
    _wire->endTransmission();
}

/**
 * @brief
 * @param
 * @retval
 **/
void LTR_553ALS::readRegs(uint8_t start_reg_addr, uint8_t *pdata, uint8_t size)
{

    _wire->beginTransmission(_address);
    _wire->write(start_reg_addr);
    _wire->endTransmission();
    _wire->requestFrom(_address, size);

    for (uint8_t i = 0; i < size; i++)
    {
        *(pdata + i) = _wire->read();
    }
}
