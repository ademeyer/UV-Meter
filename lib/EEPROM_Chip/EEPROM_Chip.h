#ifndef __EEPRROM_CHIP_H__
#define __EEPRROM_CHIP_H__
#include <stdio.h>
#include <Arduino.h>

class EEPROM_Chip
{
private:
    uint32_t page_size;
    uint32_t max_size;
public:
    EEPROM_Chip(uint32_t pg_size = 32, uint32_t max = 4096); // default clock set to 400kHz
    virtual ~EEPROM_Chip(){}
    int16_t ReadEEPROM(uint32_t addr, uint8_t* data, uint16_t amt);
    int16_t WriteEEPROM(uint32_t addr, uint8_t* data, uint16_t len);

    // Hardware specific functions
    virtual int16_t write_eeprom(uint32_t addr, uint8_t* data, uint16_t len) = 0;
    virtual int16_t read_eeprom(uint32_t addr, uint8_t* data, uint16_t amt) = 0;
};
#endif  // __EEPRROM_CHIP_H__
