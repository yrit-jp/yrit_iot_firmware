#ifdef ARDUINO_ESP32_DEV
#ifndef PIN_DEF_H
#define PIN_DEF_H

#include <Arduino.h>

/* PINIO define */
#define PIN_IO0 33
#define PIN_IO1 32
#define PIN_IO2 26
#define PIN_IO3 25
#define PIN_IO4 14
#define PIN_IO5 27
/* PIN I2C */
#define I2C_SCL 22
#define I2C_SDA 21
/* PIN Switch */
#define PSW1 34
#define PSW2 35
/* PIN LED */
#define LED_B 16
#define LED_G 17
#define LED_CHANNEL_GREEN 0 // PWM ch 0
#define LED_CHANNEL_BLUE 1  // PWM ch 1
/* PIN Power */
#define PIN_VOB 4           // for voltage monitoring
#define PIN_WAKE 13         // for battery sleep disturbance

#endif /* PIC_DEF_H */
#endif /* ARDUINO_ESP32_DEV */