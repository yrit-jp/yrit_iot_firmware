#if defined(ARDUINO_M5STACK_UNIT_C6L)

// Wire.h
#ifndef Wire_h
#define Wire_h

#include <ESP32_SoftWire.h>

typedef SoftWire TwoWire;

extern TwoWire Wire;
extern TwoWire SoftwareWire;

#endif

#endif