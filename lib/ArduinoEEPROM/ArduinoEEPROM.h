#ifndef __ARDUINOEEPROM_H__
#define __ARDUINOEEPROM_H__
#include <Arduino.h>
#include <Wire.h>
#include "EEPROM_Chip.h"

class ArduinoEEPROM : public EEPROM_Chip
{
private:
    uint32_t i2c_clock;
    uint8_t i2c_address;
    // Arduino Hardware Implementation
    int16_t write_eeprom(uint16_t addr, uint8_t* data, uint16_t len) override;
    int16_t read_eeprom(uint16_t addr, uint8_t* data, uint16_t amt) override;
public:
    ArduinoEEPROM(uint8_t addr, uint16_t pg_size = 16, uint32_t clock = 100000);
    ~ArduinoEEPROM(){}
};

#endif /* __ARDUINOEEPROM_H__ */        