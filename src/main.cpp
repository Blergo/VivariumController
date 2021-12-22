#include <Arduino.h>
#include <WiFi.h>
#include <Adafruit_SPIDevice.h>
#include <RTClib.h>
#include <time.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <lv_conf.h>
#include <lvgl.h>

#define TFT_CS   15
#define TFT_DC   33

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

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
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(ILI9341_BLACK);
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
  rtc.adjust(DateTime(timeinfo.tm_year+1900, timeinfo.tm_mon+1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec));

  tft.fillScreen(0xffff);
  delay(500);
  tft.fillScreen(0x0000);
  delay(500);
  tft.fillScreen(ILI9341_RED);
  delay(500);
  tft.fillScreen(ILI9341_GREEN);

  tft.setCursor(0, 0);
  tft.setTextColor(ILI9341_BLUE);  
  tft.setTextSize(1);
  tft.println("Hello World!");

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