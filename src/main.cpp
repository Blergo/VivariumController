#include <Arduino.h>
#include <WiFi.h>
#include <Adafruit_SPIDevice.h>
#include <RTClib.h>
#include <time.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <TFT_eSPI.h>
#include <lv_conf.h>
#include <lvgl.h>

TFT_eSPI tft = TFT_eSPI(); 

RTC_DS1307 rtc;

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 3600;

const char* ssid = "InfiniteWisdom";
const char* password = "K1net1CK1net1C";
struct tm timeinfo;

void TFTUpdate(void * parameter);

void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(9600);
  tft.init();
  tft.setRotation(2);
  tft.fillScreen(TFT_BLACK);
  initWiFi();

  if(!rtc.begin()) {
      Serial.println("Couldn't find RTC!");
      Serial.flush();
      while (1) delay(10);
  }

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  getLocalTime(&timeinfo);
  //rtc.adjust(DateTime(timeinfo.tm_year+1900, timeinfo.tm_mon+1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec));



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