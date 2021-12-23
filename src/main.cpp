#include <Arduino.h>
#include <WiFi.h>
#include <Adafruit_SPIDevice.h>
#include <RTClib.h>
#include <time.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <lv_conf.h>
#include <lvgl.h>

TFT_eSPI tft = TFT_eSPI(); 

RTC_DS1307 rtc;

TaskHandle_t TaskHandle_1;
TaskHandle_t TaskHandle_2;
TaskHandle_t TaskHandle_3;

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 3600;

const char* ssid = "InfiniteWisdom";
const char* password = "K1net1CK1net1C";
struct tm timeinfo;

void TFTUpdate(void * parameter);
void CheckRTC(void * parameter);
void initWiFi(void * parameter);

void setup() {
  Serial.begin(9600);
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  xTaskCreate(initWiFi, "Initialize WiFi", 2000, NULL, 5, &TaskHandle_3);

  if(!rtc.begin()) {
      Serial.println("Couldn't find RTC!");
      Serial.flush();
      while (1) delay(10);
  }

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  xTaskCreate(CheckRTC, "Check RTC", 2000, NULL, 4, &TaskHandle_2);
  xTaskCreate(TFTUpdate, "TFT Update", 2000, NULL, 1, &TaskHandle_1);

}

void initWiFi(void * parameter) {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
  vTaskDelete(NULL);
}

void TFTUpdate(void * parameter) {
  for(;;){
    DateTime now = rtc.now();
    char buf1[] = "DD/MM/YY-hh:mm:ss";
    Serial.println(now.toString(buf1));
    delay(1000);
  }
}

void CheckRTC(void * parameter) {
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running, let's set the time!");
    rtc.adjust(DateTime(timeinfo.tm_year+1900, timeinfo.tm_mon+1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec));
  }
  vTaskDelete(NULL);
}

void loop() {
}