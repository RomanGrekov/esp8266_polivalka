#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
//#include "html_server.h"
//#include "TimeClient.h"
#include "eeprom_funcs.h"
#include "TimeLib.h"
#include "TimeAlarms.h"
#include <WiFiUdp.h>         // for UDP NTP
#include "HashMap.h"


// Software SPI (slower updates, more flexible pin options):
// pin 13 - Serial clock out (SCLK)
// pin 12 - Serial data out (DIN)
// pin 14 - Data/Command select (D/C)
// pin 2 - LCD chip select (CS)
// pin 0 - LCD reset (RST)
Adafruit_PCD8544 display = Adafruit_PCD8544(13, 2, 12, 15, 14);

unsigned long previousMillis = 0;
const long interval = 1000;

WiFiUDP Udp;
unsigned int localPort = 8888;
 
// NTP Servers:
IPAddress timeServer(195, 186, 4, 101); // 195.186.4.101 (bwntp2.bluewin.ch)
const char* ntpServerName = "ch.pool.ntp.org";
const int timeZone = 3;     // Central European Time (summer time)
 

//SSID and Password for ESP8266
const char* def_ssid = "RESP8266";
const char* def_password = "visonic123";
IPAddress apIP(192, 168, 0, 1);
const char *myHostname = "polivalka";

AlarmId id;

ESP8266WebServer server(80);
const byte DNS_PORT = 53;
DNSServer dnsServer;

const int ledPin = D2; // an LED is connected to NodeMCU pin D1 (ESP8266 GPIO5) via a 1K Ohm resistor
bool ledState = false;

char interruption_happened = 0;

Scheduler my_scheduler;
AlarmId scheduler_ids[7];

char poliv_delay = 0;

void handleAPSettings(void);
void handleSubmitAP(void);
void handleInterrupt(void);
void handleRoot();
void handleNotFound();
void setup_scheduler(struct Scheduler *scheduler);

void Repeats() {
  Serial.println("15 second timer");
}

void EveningAlarm(){
  Serial.println("Evernyng!!!");
}

void RunTask(){
  Serial.println("Task is started!!!");

  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0,0);
  display.println("Polivaem...");
  display.display ();
  
  digitalWrite (ledPin, 1);
  delay(1000 * poliv_delay);
  digitalWrite (ledPin, 0);
  Serial.println("Task is finished!!!");

  display.println("Poliv end!");
  display.display ();
  delay(2000);
}


void setup()   {
  char ssid[SSID_SIZE];
  char pw[PW_SIZE];

  int timeout = 0;

  pinMode(5, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(5), handleInterrupt, FALLING);
  
  Serial.begin(9600);
  Serial.println("Start esp8266");
  // Init display
  display.begin();
  display.setContrast(50);
  display.clearDisplay();   // clears the screen and buffer
  display.setTextSize(1);
  display.setTextColor(BLACK);
  display.setCursor(0, 0);

  init_eeprom(EEPROM_SIZE);
  //clean_first_run();
  // Check if it is first run
  if (is_wifi_configured() != 1){
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));   // subnet FF FF FF 00
    WiFi.softAP(def_ssid, def_password);

    dnsServer.setTTL(300);
    dnsServer.setErrorReplyCode(DNSReplyCode::ServerFailure);
    dnsServer.start(DNS_PORT, "*", apIP);

    server.on("/", handleAPSettings);
    server.on("/submit_ap", HTTP_POST, handleSubmitAP);
    //server.on("/status", handleStatus);
    server.on("/generate_204", handleAPSettings);  //Android captive portal. Maybe not needed. Might be handled by notFound handler.
    server.on("/fwlink", handleAPSettings);  //Microsoft captive portal. Maybe not needed. Might be handled by notFound handler.

    display.println("First start");
    display.print("SSID:");
    display.println(def_ssid);
    display.print("PW:");
    display.println(def_password);
    display.print("IP:");
    display.println(apIP);
    display.display();
  }
  else{
    get_ssid(ssid);
    get_pw(pw);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, pw);

    short int dot_n = 0;
    display.clearDisplay();
    display.setCursor(0,0);
    display.println("Connect to AP");
    display.println(ssid);
    display.setCursor(0,20);
    timeout = 60;
    while (WiFi.status() != WL_CONNECTED && timeout > 0){
      display.print(".");
      if (dot_n == 4){
        dot_n = 0;
        display.clearDisplay();
        display.setCursor(0,0);
        display.println("Connect to AP");
        display.println(ssid);
        display.setCursor(0,20);
      }
      display.display();
      delay(500);
      dot_n++;
      timeout--;
    }
    if (WiFi.status() != WL_CONNECTED){
      display.clearDisplay();
      display.setCursor(0,0);
      display.println("Cant't connect to AP");
      display.println("Going to restart...");
      display.display();
      delay(5000);
      ESP.restart();
    }  

    display.clearDisplay();
    display.setCursor(0,0);
    display.println("Connected!");
    display.println("IP address:");
    IPAddress ip = WiFi.localIP();
    Serial.println(ip);
    display.println(ip);
    display.display();
    delay(2000);

    server.on("/", handleRoot);
    server.on("/setup", handleSetup);
    server.on("/submit_scheduler", handleSubmit);
    server.on ( "/led=1", handleRoot);
    server.on ( "/led=0", handleRoot);
    server.on ( "/inline", []() {
      server.send ( 200, "text/plain", "this works as well" );
    } );
    server.onNotFound ( handleNotFound );
  
    // Pin for engine
    pinMode ( ledPin, OUTPUT );
    digitalWrite (ledPin, 0);

    Udp.begin(localPort);
    setSyncProvider(getNtpTime);
    setSyncInterval(60);
    //setTime(8,29,0,1,1,11);
    //Serial.println("Set time");
    //setTime(H,M,S,8,9,17); // set time to Saturday 8:29:00am Jan 1 2011
    Alarm.timerRepeat(15, Repeats);
    Alarm.alarmRepeat(15,13,0, EveningAlarm);

    digitalClockDisplay();

    poliv_delay = poliv_delay_read();
    
    read_scheduler(&my_scheduler);
    if (my_scheduler._week.all != 0) setup_scheduler(&my_scheduler);
  }

  server.begin();
  Serial.println("HTTP server started");

}


void loop() {
  if (is_wifi_configured() != 1) dnsServer.processNextRequest();
  server.handleClient();

  if (interruption_happened != 0){
   interruption_happened = 0;
   display.clearDisplay();
   display.setTextSize(1);
   display.setCursor(0,0);
   display.println("Reseting...");
   display.println("Clean eeprom...");
   display.display ();
   clean_eeprom();
   //reset_wifi_configured();
   display.println("Going to restart...");
   display.display ();
   delay(2000);
   ESP.restart();
  }

  if (is_wifi_configured() == 1){
    timePrint();
    Alarm.delay(10);
  }
}

void timePrint () {
  display.clearDisplay();
  display.setCursor(0,0);
  IPAddress ip = WiFi.localIP();
  display.println(ip);

  display.setCursor(15,15);  
  display.print(day());
  display.print(".");
  display.print(month());
  display.print(".");
  display.print(year());
  display.setCursor(1,24);
  display.setTextSize(3);
  display.print(hour());
  display.setCursor(47,24);
  display.setTextSize(3);
  display.print(minute());
  display.setTextSize(1);
  display.setCursor(35,24);
  display.print(second());
  display.display ();
}

void handleAPSettings() {
   if (captivePortal()) { // If caprive portal redirect instead of displaying the page.
    return;
  }
  
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.

  char html[1000];
  // Build an HTML page to display on the web-server root address
  snprintf ( html, 1000,

"<html>\
  <head>\
    <title>ESP8266 WiFi Network</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; font-size: 1.5em; Color: #000000; }\
      h1 { Color: #AA0000; }\
    </style>\
  </head>\
  <body>\
    <h1>ESP8266 Wi-Fi AP. Please setup connection</h1>\
    <form action='/submit_ap' method='POST'>\
      SSID<br>\
      <input type='text' name='ssid'><br>\
      Password<br>\
      <input type='text' name='password'><br>\
      <input type='submit' value='Submit'>\
    </form>\
  </body>\
</html>");
  //server.send ( 200, "text/html", html );
  //digitalWrite (4, 1 );
    server.sendContent(html);
  
  server.client().stop(); // Stop is needed because we sent no content length
  delay(1000);
}

void handleRoot() {
  Serial.println("inside root callback");
  digitalWrite (ledPin, server.arg("led").toInt());
  ledState = digitalRead(ledPin);

  /* Dynamically generate the LED toggle link, based on its current state (on or off)*/
  char ledText[80];

  if (! ledState) {
    strcpy(ledText, "LED is OFF. <a href=\"/?led=1\">Turn it ON!</a>");
  }

  else {
    strcpy(ledText, "LED is ON. <a href=\"/?led=0\">Turn it OFF!</a>");
  }

  ledState = digitalRead(ledPin);

  char html[1000];
  int sec = millis() / 1000;
  int min = sec / 60;
  int hr = min / 60;

  // Build an HTML page to display on the web-server root address
  snprintf ( html, 1000,

             "<html>\
  <head>\
    <meta http-equiv='refresh' content='10'/>\
    <title>ESP8266 WiFi Network</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; font-size: 1.5em; Color: #000000; }\
      h1 { Color: #AA0000; }\
    </style>\
  </head>\
  <body>\
    <h1>ESP8266 Wi-Fi Access Point and Web Server Demo</h1>\
    <p>Uptime: %02d:%02d:%02d</p>\
    <p>%s<p>\
    <form action='/submit' method='POST'>\
      <input type='text' name='fname'><br>\
      <input type='submit' value='Submit'>\
    </form>\
    <p>This page refreshes every 10 seconds. Click <a href=\"javascript:window.location.reload();\">here</a> to refresh the page now.</p>\
  </body>\
</html>",

             hr, min % 60, sec % 60,
             ledText);
  server.send ( 200, "text/html", html );
  //digitalWrite (4, 1 );
}

void handleSetup() {
  Serial.println("inside setup callback");
  char html[1000];
  char monday[15];
  char tuesday[15];
  char wednesday[15];
  char thursday[15];
  char friday[15];
  char saturday[15];
  char sunday[15];
  
  read_scheduler(&my_scheduler);
  if (my_scheduler._week.days.monday == 1) {
    strcpy(monday, "checked='on'");
  }
  if (my_scheduler._week.days.tuesday == 1) {
    strcpy(tuesday, "checked='on'");
  }
  if (my_scheduler._week.days.wednesday == 1) {
    strcpy(wednesday, "checked='on'");
  }
  if (my_scheduler._week.days.thursday == 1) {
    strcpy(thursday, "checked='on'");
  }
  if (my_scheduler._week.days.friday == 1) {
    strcpy(friday, "checked='on'");
  }
  if (my_scheduler._week.days.saturday == 1) {
    strcpy(saturday, "checked='on'");
  }
  if (my_scheduler._week.days.sunday == 1) {
    strcpy(sunday, "checked='on'");
  }
  
  snprintf ( html, 1000,
"<html>\
  <head>\
    <title>ESP8266 Scheduler</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; font-size: 1.5em; Color: #000000; }\
      h1 { Color: #AA0000; }\
    </style>\
  </head>\
  <body>\
    <h1>ESP8266 scheduler</h1>\
    <form action='/submit_scheduler' method='POST'>\
      <input type='text' name='poliv_delay' value='%d'>Poliv delay<br>\
      <input type='checkbox' name='monday' %s>M<br>\
      <input type='checkbox' name='tuesday' %s>T<br>\
      <input type='checkbox' name='wednesday' %s>W<br>\
      <input type='checkbox' name='thursday' %s>Th<br>\
      <input type='checkbox' name='friday' %s>F<br>\
      <input type='checkbox' name='saturday' %s>St<br>\
      <input type='checkbox' name='sunday' %s>S<br>\
      <input type='text' name='hour' value='%d'>Hour<br>\
      <input type='text' name='minute' value='%d'>Minute<br>\
      <input type='submit' value='Submit'>\
    </form>\
  </body>\
</html>", poliv_delay, monday, tuesday, wednesday, thursday, friday, saturday, sunday, my_scheduler._time.hour, my_scheduler._time.minute);
  server.send ( 200, "text/html", html );
  //digitalWrite (4, 1 );
}

void handleNotFound() {
  //digitalWrite ( LED_BUILTIN, 0 );
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += ( server.method() == HTTP_GET ) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for ( uint8_t i = 0; i < server.args(); i++ ) {
    message += " " + server.argName ( i ) + ": " + server.arg ( i ) + "\n";
  }

  server.send ( 404, "text/plain", message );
}

void handleSubmitAP() {
  char retcode = 0;
  
  if (server.args() > 0 ) {
    for ( uint8_t i = 0; i < server.args(); i++ ) {
      if (server.argName(i) == "ssid") {
        retcode = save_ssid(server.arg(i));
        if (retcode != 0) return;
      }
      if (server.argName(i) == "password") {
        retcode = save_pw(server.arg(i));
        if (retcode != 0) return;
      }
      set_wifi_configured();
    }
  }
  server.sendHeader("Location", String("/"), true);
  server.send ( 302, "text/plain", "");;
  ESP.restart();
}

void handleInterrupt() {
  interruption_happened = 1;
}


void handleSubmit() {
  String hour;
  String minute;
  if (server.args() > 0 ) {
    my_scheduler._week.days.monday = 0;
    my_scheduler._week.days.tuesday = 0;
    my_scheduler._week.days.wednesday = 0;
    my_scheduler._week.days.thursday = 0;
    my_scheduler._week.days.friday = 0;
    my_scheduler._week.days.saturday = 0;
    my_scheduler._week.days.sunday = 0;
    for ( uint8_t i = 0; i < server.args(); i++ ) {
      if (server.argName(i) == "poliv_delay"){
        poliv_delay = server.arg(i).toInt();
        poliv_delay_save(poliv_delay);
      }
      if (server.argName(i) == "monday"){
          if (server.arg(i) == "on") my_scheduler._week.days.monday = 1;
      }
      if (server.argName(i) == "tuesday"){
          if (server.arg(i) == "on") my_scheduler._week.days.tuesday = 1;
      }
      if (server.argName(i) == "wednesday"){
          if (server.arg(i) == "on") my_scheduler._week.days.wednesday = 1;
      }
      if (server.argName(i) == "thursday"){
          if (server.arg(i) == "on") my_scheduler._week.days.thursday = 1;
      }
      if (server.argName(i) == "friday"){
          if (server.arg(i) == "on") my_scheduler._week.days.friday = 1;
      }
      if (server.argName(i) == "saturday"){
          if (server.arg(i) == "on") my_scheduler._week.days.saturday = 1;
      }
      if (server.argName(i) == "sunday"){
          if (server.arg(i) == "on") my_scheduler._week.days.sunday = 1;
      }
      if (server.argName(i) == "hour"){
          my_scheduler._time.hour = server.arg(i).toInt();
      }
      if (server.argName(i) == "minute"){
          my_scheduler._time.minute = server.arg(i).toInt();
      }

      save_scheduler(&my_scheduler);
      setup_scheduler(&my_scheduler);     
    }
  }
  server.sendHeader("Location", String("/setup"), true);
  server.send ( 302, "text/plain", "");;
  //ESP.restart();
}

/*-------- NTP code ----------*/

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

time_t getNtpTime()
{
  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  Serial.println("Transmit NTP Request");
  sendNTPpacket(timeServer);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println("Receive NTP Response");
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  Serial.println("No NTP Response :-(");
  return 0; // return 0 if unable to get the time
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}

/*-------- Serial Debug Code ----------*/

void digitalClockDisplay() {
  // digital clock display of the time
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.print(" ");
  Serial.print(day());
  Serial.print(".");
  Serial.print(month());
  Serial.print(".");
  Serial.print(year());
  Serial.println("");
}

void printDigits(int digits) {
  // utility for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if (digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

void setup_scheduler(struct Scheduler *scheduler){
  unsigned char hour = my_scheduler._time.hour;
  unsigned char minute = my_scheduler._time.minute;
  unsigned char day_flag = 0;
  int i=6;
  timeDayOfWeek_t _day;

  for (int i=0; i<7; i++){
    Alarm.free(scheduler_ids[i]);
    scheduler_ids[i] = dtINVALID_ALARM_ID;
  }
  
  for (int day = dowSunday; day != dowSaturday; day++){
    day_flag = ((my_scheduler._week.all >> i) & 0x01);
    if (day_flag != 0){
      _day = static_cast<timeDayOfWeek_t>(day);
      scheduler_ids[i] = Alarm.alarmRepeat(_day, hour, minute,0,RunTask);
    }
    else scheduler_ids[i] = dtINVALID_ALARM_ID;
    i++;
    if (i == 7) i = 0;
  }
}

