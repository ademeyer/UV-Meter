#include "EEPROM_Chip.h"

EEPROM_Chip::EEPROM_Chip(uint16_t pg_size) : 
    page_size(pg_size) 
{}

int16_t EEPROM_Chip::ReadEEPROM(uint16_t addr, uint8_t* data, uint16_t amt)
{
    int16_t total = 0;
    while(amt > 0)
    {
        uint16_t ret = amt > page_size ? page_size : amt;
        auto resp = read_eeprom(addr, &data[total], ret);
        if (resp <= 0) return resp;
        total += resp;
        amt -= resp;
        addr += resp;
    }
    return total;
}

int16_t EEPROM_Chip::WriteEEPROM(uint16_t addr, uint8_t* data, uint16_t len)
{
    uint16_t pos = 0;
    while(len > 0)
    {
        uint16_t ret = len > page_size ? page_size : len;
        auto resp = write_eeprom(addr, &data[pos], ret);
        if (resp == -1) return resp;
        len -= ret;
        addr += ret;
        pos += ret;
    }
    return 0;
}