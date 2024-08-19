#include "EEPROM_Chip.h"

EEPROM_Chip::EEPROM_Chip(uint32_t pg_size, uint32_t max) : 
    page_size(pg_size), max_size(max) 
{}

int16_t EEPROM_Chip::ReadEEPROM(uint32_t _startAddr, uint8_t* data, uint16_t nBytes)
{
    uint16_t pg_size = page_size - 3;
    if((_startAddr + nBytes) >= max_size) return -1;
    int16_t tRead = 0;
    while(nBytes > 0)
    {
        int16_t nRead = nBytes < pg_size ? nBytes : pg_size;
        int16_t amtRead = 0;

        if((amtRead = read_eeprom(_startAddr, data, nRead)) <= 0) break;

        tRead += amtRead;
        nBytes -= amtRead;
        _startAddr += amtRead;
        data += amtRead;
    }
    return tRead;
}

int16_t EEPROM_Chip::WriteEEPROM(uint32_t _startAddr, uint8_t* data, uint16_t nBytes)
{
    if((_startAddr + nBytes) >= max_size) return -1;
    int16_t i2c_success = 0;
    uint16_t pg_size = page_size;
	while(nBytes > 0)
	{
        uint16_t nPage = pg_size - (_startAddr % pg_size); 
        int16_t nWrite = nPage == 0 ? (pg_size - 3) : nPage;  //! something maybe off here
        nWrite = nWrite >= nBytes ? nBytes : nWrite;

        if((i2c_success = write_eeprom(_startAddr, data, nWrite)) != 0)
            break;
            
        nBytes -= nWrite;
            _startAddr += nWrite;
        data += nWrite;
	}
    return i2c_success;
}   