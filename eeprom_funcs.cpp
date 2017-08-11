#include "eeprom_funcs.h"
#include <ESP8266WiFi.h>
#include "EEPROM.h"


void init_eeprom(int size){
  // Init eeprom
  EEPROM.begin(size);
}

void eeprom_write_byte(int address, uint8_t val){
  EEPROM.write(address, val);
  EEPROM.commit();
}

void clean_eeprom(void){
  for(int i=0; i < EEPROM_SIZE; i++) EEPROM.write(i, 0);
  EEPROM.commit();
}

char is_wifi_configured(void){
  return EEPROM.read(FIRST_RUN_ADDR);
}


void set_wifi_configured(void){
  eeprom_write_byte(FIRST_RUN_ADDR, 1);
}

void reset_wifi_configured(void){
  eeprom_write_byte(FIRST_RUN_ADDR, 0);
}


char save_ssid(String ssid){
  int addr = SSID_ADDR;
  if(sizeof(ssid) > SSID_SIZE) return 1;
  for(byte b=0; b<sizeof(ssid); b++){
    EEPROM.write(addr, ssid[b]);
    addr++;
  }
  EEPROM.commit();
  return 0;
}

char save_pw(String pw){
  int addr = PW_ADDR;
  if(sizeof(pw) > PW_SIZE) return 1;
  for(byte b=0; b<sizeof(pw); b++){
    EEPROM.write(addr, pw[b]);
    addr++;
  }
  EEPROM.commit();
  return 0;
}

void get_ssid(char *dest)
{
  int addr = 0;
  dest[addr] = EEPROM.read(addr + SSID_ADDR);
  while (dest[addr] != '\0' && addr < SSID_SIZE)
  {
    addr++;
    dest[addr] = (char)EEPROM.read(addr + SSID_ADDR);
  }
}

void get_pw(char *dest)
{
  int addr = 0;
  dest[addr] = EEPROM.read(addr + PW_ADDR);
  while (dest[addr] != '\0' && addr < PW_SIZE)
  {
    addr++;
    dest[addr] = (char)EEPROM.read(addr + PW_ADDR);
  }
}

void read_scheduler(struct Scheduler *scheduler){
  byte days = 0;
  days = EEPROM.read(DAYS_ADDR);
  scheduler->_week.days.monday = ((days >> 0) & 0x1);
  scheduler->_week.days.tuesday = ((days >> 1) & 0x1);
  scheduler->_week.days.wednesday = ((days >> 2) & 0x1);
  scheduler->_week.days.thursday = ((days >> 3) & 0x1);
  scheduler->_week.days.friday = ((days >> 4) & 0x1);
  scheduler->_week.days.saturday = ((days >> 5) & 0x1);
  scheduler->_week.days.sunday = ((days >> 6) & 0x1);

  scheduler->_time.hour = EEPROM.read(HOUR_ADDR);
  scheduler->_time.minute = EEPROM.read(MINUTE_ADDR);
}

void save_scheduler(struct Scheduler *scheduler){
  eeprom_write_byte(DAYS_ADDR, scheduler->_week.all);
  eeprom_write_byte(HOUR_ADDR, scheduler->_time.hour);
  eeprom_write_byte(MINUTE_ADDR, scheduler->_time.minute);
}

