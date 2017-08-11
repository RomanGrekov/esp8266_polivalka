#ifndef EEPROM_FUNCS_h
#define EEPROM_FUNCS_h

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "eeprom_funcs.h"
#include <ESP8266WiFi.h>

const int EEPROM_SIZE = 512;
const int FIRST_RUN_SIZE = 1;
const int FIRST_RUN_ADDR = 0;
const int SSID_SIZE = 20;
const int SSID_ADDR = FIRST_RUN_ADDR + FIRST_RUN_SIZE;
const int PW_SIZE = 20;
const int PW_ADDR = SSID_ADDR + SSID_SIZE;

const int HOUR_SIZE = 1;
const int HOUR_ADDR = PW_ADDR + PW_SIZE;
const int MINUTE_SIZE = 1;
const int MINUTE_ADDR = HOUR_ADDR + HOUR_SIZE;
const int DAYS_SIZE = 1;
const int DAYS_ADDR = MINUTE_ADDR + MINUTE_SIZE;

void init_eeprom(int size);
char is_wifi_configured(void);
void eeprom_write_byte(int address, uint8_t val);
void set_wifi_configured(void);
void reset_wifi_configured(void);
char save_ssid(String ssid);
char save_pw(String pw);
void get_ssid(char *dest);
void get_pw(char *dest);
void clean_eeprom(void);
void read_scheduler(struct Scheduler *scheduler);
void save_scheduler(struct Scheduler *scheduler);

typedef union
{
  struct bitFields {
    byte monday:1;
    byte tuesday:1;
    byte wednesday:1;
    byte thursday:1;
    byte friday:1;
    byte saturday:1;
    byte sunday:1;
    byte empty:1;
  }days;

  byte all;
}S_Week;

typedef struct S_Time{
    unsigned char hour;
    unsigned char minute;
 };

typedef struct Scheduler {
   S_Time _time;
   S_Week _week;
};

#endif
