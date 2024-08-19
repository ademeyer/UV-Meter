#include "ArduinoEEPROM.h"

ArduinoEEPROM::ArduinoEEPROM(uint8_t addr, uint32_t pg_size, uint32_t clock) 
: EEPROM_Chip(pg_size), i2c_address(addr), i2c_clock(clock)
{
    Wire.setClock(i2c_clock);
    Wire.begin();
}

int16_t ArduinoEEPROM::write_eeprom(uint32_t addr, uint8_t* data, uint16_t len)
{
    if(addr > 65535)
        i2c_address = i2c_address | B00000001;

    Wire.beginTransmission(i2c_address);
    Wire.write((int)(addr >> 8));
    Wire.write((int)(addr & 0xFF));
    Wire.write(data, len);  
    delay(5);  // wait for write to complete   
    return Wire.endTransmission(); 
}

int16_t ArduinoEEPROM::read_eeprom(uint32_t addr, uint8_t* data, uint16_t amt)
{
    int16_t _no = 0;
    
    if(addr > 65535)
        i2c_address = i2c_address | B00000001;

    Wire.beginTransmission(i2c_address);
    Wire.write((byte)(addr >> 8));
    Wire.write((byte)(addr & 0xFF));
    delay(1); 
    uint8_t i2c_status = Wire.endTransmission();
    if(i2c_status == 0) 
    {
        Wire.requestFrom(i2c_address, (int)amt); 
        while(Wire.available() > 0) 
        {
            uint8_t c = Wire.read() & 0xFF; 
            data[_no++] = c; 
            delayMicroseconds(100);
        }
    }
    else return -1;     
    return _no;
}