#pragma once
#include "Flash_Data.h"
#include <SoftwareSerial.h>

// Pin Configuration
#define RXD_PIN 4
#define TXD_PIN 3


#define MAX_ATTACHMENT_SIZE         1136
#define MAXBUF                      64
SoftwareSerial GSM(RXD_PIN, TXD_PIN);

bool                                GSMExchange(const char* comd, const char *resp, uint32_t waitTime, size_t len = 0);
const char                          Email_Body[] PROGMEM          =         "Find attached (*.csv) UV data for the day in the email\r\n";

struct Email_User
{
  char name[16];
  char email[64];
  Email_User(const char* _name, const char* _email)
  {
    strncpy(name, _name, sizeof(name));
    strncpy(email, _email, sizeof(email));
  }
};

struct Sender
{
  Email_User _sender;
  char username[64];
  char password[16];
  Sender(const char* _name, const char* _email, const char* _username, const char* _password)
  : _sender(Email_User(_name, _email))
  {
    strncpy(username, _username, sizeof(username));
    strncpy(password, _password, sizeof(password));
  }
  Sender(const char* _name, const char* _email, const char* _password)
  : Sender(_name, _email, _email, _password){}
};


uint8_t GSM_data(char *resp, uint32_t waitTime)
{
  uint8_t len  = 0;
  uint32_t timr = millis() + waitTime + 5;
  while (GSM.available() == 0 && millis() < timr);
  len = GSM.readBytes(resp, MAXBUF);

#ifdef debug
  for (int p = 0; p < len; p++)
    Serial.print(resp[p]);
  Serial.println();
#endif

  return len;
}

bool GSMExchange(const char* comd, const char *resp, uint32_t waitTime, size_t len)
{
  char buffer[MAXBUF] = "";
  uint16_t _len = 0;
  if (len) _len = len;
  else _len = strlen(comd);

  memset(buffer, 0, sizeof(buffer));
  GSM.write(comd, _len);
  GSM.write("\r\n", 2);
  GSM.flush();
  u32 ttlay = millis() + waitTime;
  while (millis() < ttlay)
  {
    GSM_data(buffer, waitTime);
    if (strstr(buffer, resp) != NULL)
      return true;
  }

  return false;
}

bool isGSMPresent()
{
  return GSMExchange("AT", "OK", 500);
}

void reboot_network()
{
  GSMExchange("AT+CFUN= 1,1", "+CPIN: READY", 20000);
}

int get_simcard_config()
{
  int net = -1;
  char buffer[MAXBUF] = "";
  uint32_t gmstimer_temp = millis() + 10000;
  while (millis() < gmstimer_temp)//do
  {
#ifdef debug
    Serial.println(F("Check Network"));
#endif
    if (GSMExchange("AT+COPS?", "+COPS: 0,0,", 500))
    {
      if (strstr(buffer, "MTN") != NULL)            net = 0;
      else if (strstr(buffer, "Glo") != NULL)       net = 1;
      else if (strstr(buffer, "Airtel") != NULL || strstr(buffer, "Econet") != NULL)    net = 2;
      else if (strstr(buffer, "9Mobile") != NULL || strstr(buffer, "etisalat") != NULL)    net = 3;
      else                                          net = -1;
      GSMExchange("AT+GSMBUSY=1", "OK", 250);           //disable phone calls
      break;
    }
  }
  return net;
}

bool bringup_gprs_context()
{
  if(isGSMPresent())
  {
    // check sim config
    int net = get_simcard_config();
    if (net < 0 || net >= 4)
      return false;
    // set up gprs context
    char temp_buf[32] = "";
    strcpy_P(temp_buf, (char*)APN[net]);
#ifdef debug
    Serial.print(F("APN: "));
    Serial.println(temp_buf);
#endif
    char buf[64] = "";
    int amt = snprintf(buf, sizeof(buf), "AT+SAPBR=3,1,\"APN\", \"%s%s", temp_buf, "\"");

    GSMExchange(buf, "OK", 200, amt);
    GSMExchange("AT+SAPBR=1,1", "OK", 2000);

    // try bring up gprs connection
    if (GSMExchange("AT+SAPBR=2,1", "+SAPBR: 1,1", 5000))
      return true;

    //! reboot network if needed
    //reboot_network();
  }

  return false;
}
//uvdata44@gmail.com
//Teleport123*
void close_network()
{
  GSMExchange("AT+SAPBR=0,1", "OK", 2000);                                                                                  // Close GPRS Context
}
//Sender
bool init_emails_addr(const Sender& sender, const Email_User& recipient, const Email_User cc[] = {}, uint16_t cc_len = 0)
{
  bool status = true;
  status &= GSMExchange("AT+EMAILSSL=1", "OK", 200);                                                                          // Enable SSL
  status &= GSMExchange("AT+EMAILCID=1", "OK", 200);                                                                          // Email Parameter
  status &= GSMExchange("AT+EMAILTO=30", "OK", 200);                                                                          // Email response timeout
  char buf[MAXBUF] = "";
  status &= GSMExchange("AT+SMTPSRV=\"smtp.gmail.com\",465", "OK", 200);                                                      // set SMPT server address and port
  int amt = snprintf(buf, sizeof(buf), "AT+SMTPAUTH=1, \"%s\", \"%s\"", sender.username, sender.password);
  status &= GSMExchange(buf, "OK", 200, amt);                                                                                 // set username and password
  amt = snprintf(buf, sizeof(buf), "AT+SMTPFROM=\"%s\", \"%s\"", sender._sender.email, sender._sender.name);
  status &= GSMExchange(buf, "OK", 200, amt);                                                                                 // set sender address and name
  amt = snprintf(buf, sizeof(buf), "AT+SMTPRCPT=0, 0, \"%s\", \"%s\"", recipient.email, recipient.name);
  status &= GSMExchange(buf, "OK", 200, amt);                                                                                 // set the recipient address
  for(size_t i = 0; i < cc_len; ++i)
  {
    amt = snprintf(buf, sizeof(buf), "AT+SMTPRCPT=1, 0, \"%s\", \"%s\"", cc[i].email, cc[i].name);
    status &= GSMExchange(buf, "OK", 200, amt);                                                                               // set the cc recipient
  }

  return status;
}

bool init_email_subject_body(const char* subject, const char* body, uint16_t body_len)
{
  bool status = true;
  char buf[MAXBUF + body_len] = "";
  int amt = snprintf(buf, sizeof(buf), "AT+SMTPSUB=\", \"%s%s", subject, "\"");
  status &= GSMExchange(buf, "OK", 200); 
  memset(buf, 0, sizeof(buf));
  amt = snprintf(buf, sizeof(buf), "AT+SMTPBODY=%d", body_len);
  if (GSMExchange(buf, "DOWNLOAD", 200, amt))
  {
    memset(buf, 0, sizeof(buf));
    GSM.write(body, body_len);
    GSM.write("\r\n", 2);
    GSM.write(0x1A);
    GSM.flush();
    memset(buf, 0, sizeof(buf));
    GSM_data(buf, 500);
    status &= strstr(buf, "OK") != NULL ? true : false;
  }
  return status;
}

bool init_attachment_data(const char *filename, uint16_t expected_attachment_len)
{
  char buf[MAXBUF] = "";
  int amt = snprintf(buf, sizeof(buf), "AT+SMTPFILE=1,\"%s\",0", filename);

  if (GSMExchange(buf, "OK", 1500, amt))
  {
    if (GSMExchange("AT+SMTPSEND", "+SMTPFT: 1", 30000))
    {
      memset(buf, 0, sizeof(buf));
      amt = snprintf(buf, sizeof(buf), "AT+SMTPFT=%d", expected_attachment_len);
      return GSMExchange(buf, "+SMTPFT: 2", 30000, amt);
    }
  }
  return false;
}

void fill_email_attachment(const char *attachment, uint16_t attachment_len)
{
  GSM.write(attachment, attachment_len);
}

void close_email_attachment()
{
  GSM.write("\r\n", 2);
  GSM.write(0x1A);
  GSM.flush();
}

bool send_email()
{
  return GSMExchange("AT+SMTPSEND", "+SMTPSEND: 1", 30000);
}

bool send_email_with_attachment()
{
  return GSMExchange("AT+SMTPFT=0", "+SMTPSEND: 1", 30000);
}

void setup_GSM()
{
  GSM.begin(9600);
  GSM.setTimeout(100);
}

/*
void Detect_GSM_Pressence()
{
  char buffer[MAXBUF] = "";
#ifdef debug
  Serial.println(F("Detect_GSM_Pressence"));
#endif
  if (GSMExchange("AT", "OK", 500))
  {
    gms_presence = true;
    GSMExchange("AT+CFUN= 1,1", "+CPIN: READY", 20000);
#ifdef debug
    Serial.println(F("Module Found"));
#endif
    uint32_t gmstimer_temp = millis() + 10000;
    while (millis() < gmstimer_temp)//do
    {
#ifdef debug
      Serial.println(F("Check Network"));
#endif
      if (GSMExchange("AT+COPS?", "+COPS: 0,0,", 500))
      {
        if (strstr(buffer, "MTN") != NULL)            net = 0;
        else if (strstr(buffer, "Glo") != NULL)       net = 1;
        else if (strstr(buffer, "Airtel") != NULL || strstr(buffer, "Econet") != NULL)    net = 2;
        else if (strstr(buffer, "9Mobile") != NULL || strstr(buffer, "etisalat") != NULL)    net = 3;
        else                                          net = -1;

//#ifdef debug
        Serial.print(F("Network Found: "));
        Serial.println(net);
//#endif
        GSMExchange("AT+GSMBUSY=1", "OK", 250);           //disable phone calls
        break;
      }
    }
  }
}

bool  send_Email_With_Attachment(const char *attachment_name, u16 an, char *attachment, u16 alen)
{
  bool sent = false;
  Detect_GSM_Pressence();
  delay(100);
  if (gms_presence)
  {
    if (net < 0 || net >= 4)
    {
#ifdef debug
      Serial.println(F("Detected Network Config not found!"));
#endif
      return sent;
    }

    char temp_buf[32] = "";
    strcpy_P(temp_buf, (char*)APN[net]);

#ifdef debug
    Serial.print(F("APN: "));
    Serial.println(temp_buf);
#endif

    char buf[64] = "";
    int amt = snprintf(buf, sizeof(buf), "AT+SAPBR=3,1,\"APN\", \"%s%s", temp_buf, "\"");

    GSMExchange(buf, "OK", 200, amt);
    GSMExchange("AT+SAPBR=1,1", "OK", 2000);

    if (GSMExchange("AT+SAPBR=2,1", "+SAPBR: 1,1", 5000))
    {
#ifdef debug
      Serial.println(F("GPRS is active!"));
#endif
      GSMExchange("AT+EMAILSSL=1", "OK", 200);                                                                          // Enable SSL
      GSMExchange("AT+EMAILCID=1", "OK", 200);                                                                          // Email Parameter
      GSMExchange("AT+EMAILTO=30", "OK", 200);                                                                          // Email response timeout
      //GSMExchange("AT+SMTPCS=\"UTF-8\"", "OK", 200);                                                                    // Set the Email Charset

      GSMExchange("AT+SMTPSRV=\"mail.timed.tech\",465", "OK", 200);                                                     // set SMPT server address and port
      GSMExchange("AT+SMTPAUTH=1, \"IOX@timed.tech\", \"ioxpa$$1\"", "OK", 200);                                        // set username and password
      GSMExchange("AT+SMTPFROM=\"IOX@timed.tech\", \"IOX Board\"", "OK", 200);                                          // set sender address and name
      //GSMExchange("AT+SMTPRCPT=0, 0, \"adebayotimileyin@gmail.com\", \"Adebayo Timileyin\"", "OK", 200);                // set the recipient address
      GSMExchange("AT+SMTPRCPT=0, 0, \"salisusegzy@gmail.com\", \"Salisu Segun\"", "OK", 200);                          // set the recipient address
      GSMExchange("AT+SMTPRCPT=1, 0, \"adebayotimileyin@gmail.com\", \"Adebayo Timileyin\"", "OK", 200);                // set the cc recipient address
      GSMExchange("AT+SMTPSUB=\"UV DATA DUMP\"", "OK", 200);                                                            // set the subject

      memset(buf, 0, sizeof(buf));
      int amt = snprintf(buf, sizeof(buf), "AT+SMTPBODY=%d", strlen(Email_Body));
      char buffer[MAXBUF] = "";
      if (GSMExchange(buf, "DOWNLOAD", 200, amt))
      {
        GSM.write(Email_Body, strlen(Email_Body));
        GSM.write("\r\n", 2);
        GSM.write(0x1A);
        GSM.flush();
        memset(buffer, 0, sizeof(buffer));
        GSM_data(buffer, 500);
        if (strstr(buffer, "OK") == NULL)
        {
#ifdef debug
          Serial.println(F("Failed to send Email: 1"));
#endif
          goto close_network;
        }

        amt = 0;
        memset(buf, 0, sizeof(buf));
        memset(temp_buf, 0, sizeof(temp_buf));
        strcat(buf, "AT+SMTPFILE=1,\"");
        strncat(buf, attachment_name, an);
        strcat(buf, "\",0");

        if (GSMExchange(buf, "OK", 1500))
        {
          if (GSMExchange("AT+SMTPSEND", "+SMTPFT: 1", 30000))
          {
            amt = 0;
            memset(buf, 0, sizeof(buf));
            memset(temp_buf, 0, sizeof(temp_buf));

            amt = snprintf(temp_buf, sizeof(temp_buf), "%d", alen);

            strcat(buf, "AT+SMTPFT=");
            strncat(buf, temp_buf, amt);

            if (GSMExchange(buf, "+SMTPFT: 2", 30000, strlen(buf)))
            {
              GSM.write(attachment, alen); //("Hello SIMCOM", 12);
              GSM.write("\r\n", 2);
              GSM.write(0x1A);
              GSM.flush();
              memset(buffer, 0, sizeof(buffer));
              GSM_data(buffer, 30000);
              if (strstr(buffer, "+SMTPFT: 1") == NULL)
              {
#ifdef debug
                Serial.println("Failed to send Email: 2");
#endif
                //GSMExchange("AT+SAPBR=0,1", "+SAPBR 1: DEACT", 500);
                //return;

                goto close_network;
              }

              if (GSMExchange("AT+SMTPFT=0", "+SMTPSEND: 1", 30000)) sent = true;
            }
          }
        }
      }
    }
    else
    {
#ifdef debug
      Serial.println(F("Couldn't bring up GPRS!"));
#endif
    }
close_network:
    GSMExchange("AT+SAPBR=0,1", "OK", 2500);
  }

  return sent;
}
*/
