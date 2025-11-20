/*
*****************************************************************************
@file         Deneyap_MesafeOlcerIsikAlgilayici.h
@mainpage     Deneyap Proximity&Light Sensor LTR553 Arduino library header file
@version      v1.0.0
@date         June 24, 2022
@brief        This file contains all function prototypes and macros
              for Deneyap Proximity&Light Sensor LTR553 Arduino library
@license      MIT License
@modified     Feb 27, 2025 by takkaO
*****************************************************************************
*/

#ifndef __LTR_553ALS_H__
#define __LTR_553ALS_H__

#include <Arduino.h>
#include <Wire.h>

class LTR_553ALS
{
public:
    static constexpr uint8_t DEFAULT_ADDRESS = 0x23;

    LTR_553ALS(TwoWire *theWire = &Wire)
    {
        _wire = theWire;
    }

    bool begin(uint8_t address = DEFAULT_ADDRESS);
    void startSensor(uint8_t psMode, uint8_t alsMode);
    void softwareReset(void);
    void setALSmode(uint8_t mode);
    void setALSgain(uint8_t gain);
    uint8_t getALSgain(void);
    void setPSmode(uint8_t mode);
    void setPSsaturationIndicator(uint8_t strIndicator);
    void setPSledPulseFreq(uint8_t ledPulseFreq);
    void setPSledDutyCycle(uint8_t ledDutyCycle);
    void setPSledPeakCurrent(uint8_t ledPeakCurrent);
    void setPSledPulse(uint8_t ledPulse);
    void setPSmeasurementRate(uint8_t measRate);
    void setALSintegrationTime(uint8_t intTime);
    uint8_t getALSintegrationTime(void);
    void setALSmeasurementRate(uint8_t measRate);
    uint8_t getPartNumberID(void);
    uint8_t getRevisionID(void);
    uint8_t getManufacturerID(void);
    uint16_t getPSvalue(void);
    uint16_t getALSCH0value(void);
    uint16_t getALSCH1value(void);
    float getLuxValue(void);

private:
    TwoWire *_wire;
    uint8_t _address;
    void readRegs(uint8_t start_reg_addr, uint8_t *pdata, uint8_t size);
    void writeRegs(int registerAddress, uint8_t dataValue);
};

#endif /* Deneyap_MesafeOlcerIsikAlgilayici_H */
