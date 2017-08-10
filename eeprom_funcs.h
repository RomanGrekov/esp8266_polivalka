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
const int MONDAY_SIZE = 1;
const int MONDAY_ADDR = MINUTE_ADDR + MINUTE_SIZE;
const int TUESDAY_SIZE = 1;
const int TUESDAY_ADDR = MONDAY_ADDR + MONDAY_SIZE;
const int WEDNESDAY_SIZE = 1;
const int WEDNESDAY_ADDR = TUESDAY_ADDR + TUESDAY_SIZE;
const int THURSDAY_SIZE = 1;
const int THURSDAY_ADDR = WEDNESDAY_ADDR + WEDNESDAY_SIZE;
const int FRIDAY_SIZE = 1;
const int FRIDAY_ADDR = THURSDAY_ADDR + THURSDAY_SIZE;
const int SATURDAY_SIZE = 1;
const int SATURDAY_ADDR = FRIDAY_ADDR + FRIDAY_SIZE;
const int SUNDAY_SIZE = 1;
const int SUNDAY_ADDR = SATURDAY_ADDR + SATURDAY_SIZE;


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


#endif
