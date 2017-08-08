#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
//#include "html_server.h"
#include "TimeClient.h"
#include "eeprom_funcs.h"

// Software SPI (slower updates, more flexible pin options):
// pin 13 - Serial clock out (SCLK)
// pin 12 - Serial data out (DIN)
// pin 14 - Data/Command select (D/C)
// pin 2 - LCD chip select (CS)
// pin 0 - LCD reset (RST)
Adafruit_PCD8544 display = Adafruit_PCD8544(13, 12, 14, 2, 0);

float utcOffset = 3; // enter your UTC
TimeClient timeClient(utcOffset);
int clockDelay = 1000 ; // clock update period
int updateDelay = 30*60*1000 ; // time server verify period min*sec*millis
unsigned long clockUpdate = 0 ; //
unsigned long timeUpdate = 0 ; //
void syncPrint () ;
void timePrint ();

//SSID and Password for ESP8266
const char* def_ssid = "RESP8266";
const char* def_password = "visonic123";
IPAddress apIP(192, 168, 0, 1);
const char *myHostname = "polivalka";

int timeout = 0;

ESP8266WebServer server(80);
const byte DNS_PORT = 53;
DNSServer dnsServer;

const int ledPin = D2; // an LED is connected to NodeMCU pin D1 (ESP8266 GPIO5) via a 1K Ohm resistor
bool ledState = false;

char interruption_happened = 0;


void handleAPSettings(void);
void handleSubmitAP(void);
void handleInterrupt(void);
void handleRoot();
void handleNotFound();


void setup()   {
  char ssid[SSID_SIZE];
  char pw[PW_SIZE];

  pinMode(5, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(5), handleInterrupt, FALLING);
  
  //Serial.begin(9600);
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

    WiFi.begin(ssid, pw);

    short int dot_n = 0;
    display.clearDisplay();
    display.setCursor(0,0);
    display.println("Connect to AP");
    display.println(ssid);
    display.setCursor(0,20);
    timeout = 20;
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
      reset_wifi_configured();
      delay(5000);
      ESP.restart();
    }  

    display.clearDisplay();
    display.setCursor(0,0);
    display.println("Connected!");
    display.println("IP address:");
    IPAddress ip = WiFi.localIP();
    display.println(ip);
    display.display();
    delay(2000);

    server.on("/", handleRoot);
    server.on ( "/led=1", handleRoot);
    server.on ( "/led=0", handleRoot);
    server.on ( "/inline", []() {
      server.send ( 200, "text/plain", "this works as well" );
    } );
    server.onNotFound ( handleNotFound );
  
    // Pin for engine
    pinMode ( ledPin, OUTPUT );
    digitalWrite (ledPin, 1);

    clockUpdate = millis () ; //
    timeUpdate = millis () ; //

    timeClient.updateTime();
  }

  server.begin();
  Serial.println("HTTP server started");

}


void loop() {
  dnsServer.processNextRequest();
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
    unsigned long currentMillis = millis();
     if ((currentMillis - timeUpdate) < 3000){
      syncPrint ();
     }
  
     if ((currentMillis - timeUpdate) > updateDelay) {
      timeClient.updateTime();
      timeUpdate = millis ();
     }
     if ((currentMillis - clockUpdate) >= clockDelay ) {
      display.clearDisplay();
      timePrint();
      clockUpdate = millis ();
    }
    display.display();
  }

}

void timePrint () {
//  String time = timeClient.getFormattedTime();
  String H = timeClient.getHours();
  String M = timeClient.getMinutes();
  String S = timeClient.getSeconds();
  display.setCursor(1,24);
//  display.print(time);
  display.setTextSize(3);
  display.print(H);
  display.setCursor(47,24);
  display.setTextSize(3);
  display.print(M);
  display.setTextSize(1);
  display.setCursor(35,24);
  display.print(S);
  //display.display ();
}

void syncPrint () {
   display.clearDisplay();
   display.setTextSize(1);
   display.setCursor(0,0);
   display.print("Chewck clock");
   display.setCursor(0,8);
   display.print("time.nist.gov");
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
  digitalWrite (ledPin, server.arg("led").toInt());
  ledState = digitalRead(ledPin);

  /* Dynamically generate the LED toggle link, based on its current state (on or off)*/
  char ledText[80];

  if (! ledState) {
    strcpy(ledText, "LED is on. <a href=\"/?led=1\">Turn it OFF!</a>");
  }

  else {
    strcpy(ledText, "LED is OFF. <a href=\"/?led=0\">Turn it ON!</a>");
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
  if (server.args() > 0 ) {
    for ( uint8_t i = 0; i < server.args(); i++ ) {
      if (server.argName(i) == "fname") {
        timeout = server.arg(i).toInt();
        display.clearDisplay();
        display.setCursor(0, 5);
        display.print("Val:");
        display.println(server.arg(i));
        display.display();
      }
    }
  }
  server.sendHeader("Location", String("/"), true);
  server.send ( 302, "text/plain", "");;
}


