#pragma once
#include <Arduino.h>

const char Glo[] PROGMEM                            = "gloflat";
const char MTN[] PROGMEM                            = "web.gprs.mtnnigeria.net";
const char Airtel[] PROGMEM                         = "internet.ng.airtel.com";
const char NineMobile[] PROGMEM                     = "9mobile";


const char * const APN[]                    =  
{   
  MTN,
  Glo,
  Airtel,
  NineMobile,
};
