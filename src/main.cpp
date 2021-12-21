#include <Arduino.h>
//#include <WiFi.h>
#include <Adafruit_SPIDevice.h>
#include <RTClib.h>
//#include <time.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

#define STMPE_CS 32
#define TFT_CS   15
#define TFT_DC   33
#define SD_CS    14

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

RTC_DS1307 rtc;
//int yr = 0;
//int mt = 0;
//int dy = 0;
//int hr = 0;
//int mi = 0;
//int se = 0;

//const char* ntpServer = "pool.ntp.org";
//const long  gmtOffset_sec = 0;
//const int   daylightOffset_sec = 3600;

//const char* ssid = "InfiniteWisdom";
//const char* password = "K1net1CK1net1C";
//struct tm timeinfo;

void TFTUpdate(void * parameter);

//void initWiFi() {
//  WiFi.mode(WIFI_STA);
//  WiFi.begin(ssid, password);
//  Serial.print("Connecting to WiFi ..");
//  while (WiFi.status() != WL_CONNECTED) {
//    Serial.print('.');
//    delay(1000);
//  }
//  Serial.println(WiFi.localIP());
//}

void setup() {
  Serial.begin(9600);
  tft.begin();
//  initWiFi();

  if(!rtc.begin()) {
      Serial.println("Couldn't find RTC!");
      Serial.flush();
      while (1) delay(10);
  }

//  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

//  if(!getLocalTime(&timeinfo)){
//    Serial.println("Failed to obtain time");
//    return;
//  }
//  getLocalTime(&timeinfo);
//  yr = timeinfo.tm_year + 1900;
//  mt = timeinfo.tm_mon + 1;
//  dy = timeinfo.tm_mday;
//  hr = timeinfo.tm_hour;
//  mi = timeinfo.tm_min;
//  se = timeinfo.tm_sec;
//  rtc.adjust(DateTime(yr, mt, dy, hr, mi, se));

  xTaskCreate(TFTUpdate, "TFT Update", 1000, NULL, 1, NULL);
}

void TFTUpdate(void * parameter){
  for(;;){
    DateTime now = rtc.now();
    char buf1[] = "DD/MM/YY-hh:mm:ss";
    Serial.println(now.toString(buf1));
    delay(1000);
  }
}

void loop() {
}