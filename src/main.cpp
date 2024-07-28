#include <Arduino.h>
#define USE_MALLOC_ 0
//User libraries
//#include "U8glib.h"
#include <DS3231.h>
#include <Wire.h>
// #include <Adafruit_GFX.h>
// #include <Adafruit_SSD1306.h>
#include "ArduinoEEPROM.h"
#include "FloatingInteger.h"
#include "IOX0_Email.h"

#define MAX_STORAGE 4095

// Define hardware
#define SENSOR_PIN A1

//Object definitions
// #define SCREEN_WIDTH 128 // OLED display width, in pixels
// #define SCREEN_HEIGHT 32 // OLED display height, in pixels
// #define OLED_RESET     -1
// #define SCREEN_ADDRESS 0x3C
//Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

DS3231 myRTC;
ArduinoEEPROM mem(0x57);
FloatingInteger ft;

struct datetime_data
{
  uint8_t year;
  uint8_t month;
  uint8_t date;
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
  datetime_data() : year(0), month(0), date(0), hour(0), minute(0), second(0) {}
  datetime_data(uint8_t yr, uint8_t mn, uint8_t dt, uint8_t hr, uint8_t mi, uint8_t se) 
  : year(yr), month(mn), date(dt), hour(hr), minute(mi), second(se) {}
};

struct UV_data
{
  uint16_t count;
  datetime_data dt;
  number uv_index;        // 2-byte decimal point
  number uv_irradiance;   //mW/cm^2
  UV_data() : count(0), dt(), uv_index(), uv_irradiance() {}
};

// Gobal variables
uint16_t saveIndex = 0, counter = 1;
number min_index, max_index;

// Function definitions
int convertIntToString(int value, char* res, int len, int sf = 0)
{
  memset(res, 0, len);
  char format[12] = "";
  if (sf > 0)
    sprintf(format, "%s%d%c", "%0", sf, 'd');//%02d
  else 
    strcpy(format, "%d");

  return snprintf(res, len, format, value);
}

// void draw(const UV_data& uvd) 
// {
//   auto it = uvd.dt;
//   // char dateandtime[6] = "";
//   // char temp[32] = "";
//   // auto it = uvd.dt;
//   // memset(temp, 0, sizeof(temp));
//   // int num = convertIntToString(it.hour, temp, sizeof(temp));
//   // strcat(temp, ":");
//   // ++num;
//   // convertIntToString(it.minute, &temp[num], (num+2));
//   // strcpy(dateandtime, temp);


//   // char u[4] = "", umax[4] = "", umin[4] = "", uirrad[4] = "";
//   // num = convertIntToString(round(ft.NumberToFloat(uvd.uv_index)), u, sizeof(u), 2);

//   // num = convertIntToString(round(ft.NumberToFloat(max_index)), umax, sizeof(umax), 2);

//   // num = convertIntToString(round(ft.NumberToFloat(min_index)), umin, sizeof(umin), 2);

//   // num = convertIntToString(round(ft.NumberToFloat(uvd.uv_irradiance)), uirrad, sizeof(uirrad), 2);

//   // Here we print converted values
//   display.clearDisplay();
//   display.setTextSize(1);      // Normal 1:1 pixel scale
//   display.setTextColor(SSD1306_WHITE); // Draw white text
//   display.setCursor(48, 10);
//   display.cp437(true);
//   display.write(it.hour);
//   display.write(':');
//   display.write(it.minute);
//   display.display();
//   //delay(2000);

//   // u8g.setFont(u8g_font_6x12);
//   // u8g.drawStr(48, 10, dateandtime);
//   // u8g.drawStr(105, 27, umax); // Max_index
//   // u8g.drawStr(105, 56, umin); // Min_index
//   // u8g.setFont(u8g_font_5x7);
//   // u8g.drawStr(105, 17, "MAX");
//   // u8g.drawStr(105, 46, "MIN");
//   // u8g.drawStr(63, 58, "mW/cm2");
//   // u8g.drawStr(7, 20, "UV Index");
//   // u8g.setFont(u8g_font_fub35n);
//   // u8g.drawStr(2, 60, u);
//   // u8g.setFont(u8g_font_fub14);
//   // u8g.drawStr(65, 50, uirrad);
// }


/**
* @brief Reads an analog value from a specified pin.
* This function reads an analog value from the specified pin and returns the average of the samples taken.
* It takes a single parameter, PIN, which represents the pin number to read from.
* The function reads the analog value samples times and calculates the average by summing all the values and dividing by the number of samples.
* The analog value is obtained using the analogRead function provided by the Arduino framework.
* @param[in] PIN The pin number from which to read the analog value.
* @return The average of the analog values read from the specified pin.
* @see analogRead
*/
uint16_t ReadAnalogueData(uint8_t PIN)
{
  const uint8_t samples = 100;
  uint32_t values; 

  for(int i = 0; i < samples; ++i)
  {
    values += analogRead(PIN);
    delayMicroseconds(500);
  }
  return (uint16_t)values/samples;
}

/**
* @brief Converts an analog value to a UV index using a polynomial equation.
* This function takes an analog value (x) as input and returns the corresponding UV index.
* The conversion is performed using a polynomial equation that maps the analog value to the UV index.
* @param[in] x The analog value obtained from the UV sensor.
* @return The UV index corresponding to the given analog value.
*/
float SampleToUVIndex(int x) // x = analog value
{
  // Implementation details are left as an exercise for the reader
  return x < 50 ? 0.00 : 
          (8.18 * pow(10, -12) * pow(x, 4))   +
          (-2.37 * pow(10, -8) * pow(x, 3))   +
          (2.4 * pow(10, -5) * pow(x, 2))     + 
          (7.44* pow(10, -4) * x)             + 
          (-0.0224);
}

/**
* @brief Updates the datetime_data object with the current date-time from the RTC.
        This function reads the current date-time from the RTC (Real Time Clock) and updates the datetime_data object with the obtained values.
* @param[out] dateTime A reference to a datetime_data object that will be updated with the current date-time.
* @return void.
* @note The function does not perform any validation on the obtained date-time values. It is assumed that the RTC is providing accurate and valid date-time data.
* @see datetime_data
* @see myRTC
*/
void UpdateDateTime(datetime_data& dateTime)
{
  bool century = false;
  bool h12Flag;
  bool pmFlag;
  dateTime = datetime_data(myRTC.getYear(), myRTC.getMonth(century), 
                            myRTC.getDate(), myRTC.getHour(h12Flag, pmFlag), 
                              myRTC.getMinute(), myRTC.getSecond());
}
/**
 * @brief Updates the UV data packet with the current UV index and date-time.
 *
 * This function reads the UV sensor's analog value, converts it to UV index, and updates the UV data packet.
 * It also calls the UpdateDateTime function to get the current date-time.
 *
 * @param data The UV data packet to be updated.
 *
 * @return void.
 *
 * @note The function does not calculate the UV irradiance values. It is a placeholder for future implementation.
 *
 * @see UpdateDateTime
 * @see SampleToUVIndex
 * @see ReadAnalogueData
 * @see FloatingInteger::FloatToNumber
 */
void UpdateUVDataPacket(UV_data& data)
{
  UpdateDateTime(data.dt);
  // int x = ReadAnalogueData(SENSOR_PIN);
  // Serial.print("analog (x) = ");
  // Serial.println(x);
  if(counter >= 65535)
    counter = 0;
  data.count = counter;
  auto value = SampleToUVIndex(ReadAnalogueData(SENSOR_PIN));
  data.uv_index = ft.FloatToNumber(value, 3);
  // ! ToDo: find irradiance values
}

int GetDateTimeBytes(const datetime_data& dt, uint8_t* data)
{
  int pos = 0;
  // yr,mn,dd,hr,mm,ss
  data[pos++] = dt.year; 
  data[pos++] = dt.month; 
  data[pos++] = dt.date; 
  data[pos++] = dt.hour; 
  data[pos++] = dt.minute; 
  data[pos++] = dt.second; 
  return pos;
}

datetime_data GetDateTimeFromBytes(const uint8_t* data)
{
  datetime_data dt;
  dt.year   = data[0];
  dt.month  = data[1];
  dt.date   = data[2];
  dt.hour   = data[3];
  dt.minute = data[4];
  dt.second = data[5];
  return dt;
}
int ConvertUVDataToByte(const UV_data& data, uint8_t* buf)
{
  /*
  * LLB, MMB
  * count         - 2 bytes
  * dt            - 6 bytes
  * uv_index      - 2 bytes
  * uv_irradiance - 2 bytes
  */
 int pos = 0;
 // count
 buf[pos++] = data.count & 0xFF; // get low byte
 buf[pos++] = data.count >> 8; // get high byte
 // Date and Time
 pos += GetDateTimeBytes(data.dt, &buf[pos]);
 // UV Index
 buf[pos++] = data.uv_index.value;
 buf[pos++] = data.uv_index.power; 
 // UV Irradiance
 buf[pos++] = data.uv_irradiance.value;
 buf[pos++] = data.uv_irradiance.power;
 
 return pos;
}

UV_data ConvertBytesToUVData(uint8_t* buf, uint16_t size) 
{
/*
* LLB, MMB
* count         - 2bytes
* dt            - 6 bytes
* uv_index      - 2 bytes
* uv_irradiance - 2 bytes
*/
  UV_data result;
  if(size < sizeof(UV_data))
  {
    Serial.println(F("Invalid data size"));
    return result;
  } 
 int pos = 2;
 result.count = buf[1] << 8 | buf[0];
 result.dt = GetDateTimeFromBytes(&buf[pos]);
 pos += sizeof(datetime_data);
 result.uv_index.value = buf[pos++];
 result.uv_index.power = buf[pos++];
 result.uv_irradiance.value = buf[pos++];
 result.uv_irradiance.power = buf[pos++];

 return result;
}

int formatTimeToString(const datetime_data& dt, char* str)
{
  int pos = 0;
  pos += convertIntToString(dt.year, &str[pos], 3, 2);
  str[pos++] = '/';
  pos += convertIntToString(dt.month, &str[pos], 3, 2);
  str[pos++] = '/';
  pos += convertIntToString(dt.date, &str[pos], 3, 2);
  str[pos++] = ' ';
  pos += convertIntToString(dt.hour, &str[pos], 3, 2);
  str[pos++] = ':';
  pos += convertIntToString(dt.minute, &str[pos], 3, 2);
  str[pos++] = ':';
  pos += convertIntToString(dt.second, &str[pos], 3, 2);
  str[pos] = '\0';

  return pos;
}

int ConvertUVDataToString(const UV_data& data, char* buffer, int size)
{
  // 00001,01/12/24 12:45:55,00,00,00,00\r\n = 38
  int pos = 0;
  pos += convertIntToString(data.count, &buffer[pos], (size-pos), 5);
  buffer[pos++] = ',';
  pos += formatTimeToString(data.dt, &buffer[pos]); // return 17 bytes
  buffer[pos++] = ',';
  pos += convertIntToString(round(ft.NumberToFloat(data.uv_index)), &buffer[pos], (size-pos), 2);
  buffer[pos++] = ',';
  pos += convertIntToString(round(ft.NumberToFloat(data.uv_irradiance)), &buffer[pos], (size-pos), 2);
  buffer[pos++] = '\r';
  buffer[pos++] = '\n';
  buffer[pos] = '\0';
  return pos <= size ? pos : size;
}

void PrintDateTime(const datetime_data& dt)
{
  Serial.print(dt.year);
  Serial.print('/');
  Serial.print(dt.month);
  Serial.print('/');
  Serial.print(dt.date);
  Serial.print(' ');
  Serial.print(dt.hour);
  Serial.print(':');
  Serial.print(dt.minute);
  Serial.print(':');
  Serial.println(dt.second);
}

void PrintUVData(const UV_data& data)
{
  Serial.print(F("count: "));
  Serial.println(data.count);
  Serial.print(F("DateTime: "));
  PrintDateTime(data.dt);
  Serial.print(F("UV Index: "));
  Serial.println(ft.NumberToFloat(data.uv_index));
  Serial.print(F("Max UV Index: "));
  Serial.println(ft.NumberToFloat(max_index));
  Serial.print(F("Min UV Index: "));
  Serial.println(ft.NumberToFloat(min_index));
  Serial.println();
}

void SaveToMemory(const UV_data& data)
{
  // check whether there is enough memory to store
  if(saveIndex + sizeof(UV_data) > MAX_STORAGE)
  {
    Serial.println(F("No memory space to save"));
    return;
  }
  // saveIndex = 0;
  uint8_t buf[sizeof(UV_data)];
  int amt = ConvertUVDataToByte(data, buf);
  mem.WriteEEPROM(saveIndex, buf, amt);
  // increment index
  saveIndex += amt;
  Serial.print(F("Data saved at index "));
  Serial.println(saveIndex - sizeof(UV_data));
}

uint16_t ReadFromMemoryInStr(char* buf, size_t size, uint16_t& Index)
{
  /* 
  * To calculate bytes to read from memory:
  * we expected 32 char conversion for each size of UV_data
  */
  uint8_t data_size = sizeof(UV_data);
  uint8_t factor = size / 32; 
  uint16_t expected_len = factor * data_size;
  uint8_t temp[expected_len];
  uint16_t amt = mem.ReadEEPROM(Index, temp, sizeof(temp));

  if(amt > expected_len)
  {
    Serial.println(F("Error: Read data from memory exceeds expected length"));
    return 0;
  }
  uint16_t readBytes = 0;
  for(int i = 0; i < factor; ++i)
  {
    auto data = ConvertBytesToUVData(&temp[(i * data_size)], (amt - (i * data_size)));
    readBytes += ConvertUVDataToString(data, &buf[readBytes], (size - readBytes));
  }
  readBytes = readBytes > size ? size : readBytes;
  buf[readBytes] = '\0'; // null terminate
  Index += amt;
  return readBytes;
}

bool isTime(datetime_data nowTime, datetime_data chk)
{
  return nowTime.hour >= chk.hour && nowTime.minute >= chk.minute && nowTime.second >= chk.second;
}

void Handle_SendEmail(datetime_data& dt)
{
  static datetime_data gsm_timer = dt;
  // check timestamp
  if(saveIndex > 0 && isTime(dt, datetime_data(gsm_timer.year, gsm_timer.month, gsm_timer.date, (dt.hour + 0), (gsm_timer.minute + 1), gsm_timer.second)))
  {
    if(!bringup_gprs_context()) 
    {
      Serial.println(F("Failed to bring up GPRS context"));
      return;
    }

    {
      Sender sender("UV Robot", "uvdata44@gmail.com", "Teleport123*");
      Email_User main_receiver("server", "server@gmail.com");
      Email_User cc_receiver[] = { Email_User("Adebayo Timileyin", "adebayotimileyin@gmail.com") };
      if(!init_emails_addr(sender, main_receiver, cc_receiver, sizeof(cc_receiver) / sizeof(Email_User)))
      {
        Serial.println(F("Failed to initialize email addresses"));
        close_network();
        return;
      }
    }
#if USE_MALLOC_
    char* attach_ptr = NULL;
    attach_ptr = (char*) calloc(MAX_ATTACHMENT_SIZE, sizeof(char));
    if(!attach_ptr)
    {
      Serial.println(F("Failed to allocate memory for attachment"));
      return;
    }
#else
  char attach_ptr[224] = "";
#endif

    //Serial.println(attach_ptr);
    bool success = true;
    uint16_t readIndex = 0;
    while(readIndex < saveIndex && success)
    {
      Serial.println(F("Here 0"));
      success &= init_email_subject_body("UV DATA DUMP", Email_Body, strlen_P(Email_Body));
      int tlen = 0;
      memset(attach_ptr, 0, sizeof(attach_ptr));

      // Get name of attachment file
      char _fileName[24] = "UVLog_";
      sprintf(_fileName, "UVLog_%02d%02d_%d.csv", dt.hour, dt.minute, readIndex);
      Serial.print(F("_fileName = "));
      Serial.println(_fileName);
      success &= init_attachment_data(_fileName, MAX_ATTACHMENT_SIZE);
      // add attachments
      if(success)
      {
        uint16_t send_len = MAX_ATTACHMENT_SIZE;

        while(send_len > 0)
        {
#if USE_MALLOC_
          tlen = ReadFromMemoryInStr(attach_ptr, send_len, readIndex);
#else
          tlen = ReadFromMemoryInStr(attach_ptr, sizeof(attach_ptr), readIndex);
#endif
          if(tlen == 0)
          {
            Serial.println(F("Failed to read from memory"));
            break;
          } 
          fill_email_attachment(attach_ptr, tlen);
          send_len -= tlen;
        }
        // If we didn't send up to the length set for GSM module, we should inform it that we are done sending what we have
        if(send_len > 0)
          close_email_attachment();
      }
      // send email with attachment
      success &= send_email_with_attachment();
      Serial.println(F("Here 1"));
    }
    close_network();
    if(success)
    {
      saveIndex = 0;
    }
    Serial.print(F("saveIndex = "));
    Serial.println(saveIndex);

#if USE_MALLOC_
    if(attach_ptr)
      free(attach_ptr);
    attach_ptr = NULL;
#endif
    Serial.println(F("Here 2"));
    gsm_timer = dt;
    //return;
  }
}


void setup() 
{
  pinMode(SENSOR_PIN, INPUT);
  Serial.begin(9600);
  setup_GSM();
}

void loop() 
{
  static datetime_data timer_data;
  UV_data newData;
  UpdateUVDataPacket(newData);
  if(isTime(newData.dt, datetime_data(timer_data.year, timer_data.month, timer_data.date, timer_data.hour, (timer_data.minute + 0), timer_data.second + 10)))
  {
    // find max and min values
    if(ft.NumberToFloat(newData.uv_index) > ft.NumberToFloat(max_index))
      max_index = newData.uv_index;
    if(ft.NumberToFloat(newData.uv_index) <= ft.NumberToFloat(min_index))
      min_index = newData.uv_index;

    SaveToMemory(newData);
    timer_data = newData.dt;
    ++counter;
    PrintUVData(newData);
  }

  // send email if necessary
  Handle_SendEmail(newData.dt);
}
