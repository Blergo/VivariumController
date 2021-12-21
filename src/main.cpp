#include <Arduino.h>
#include <Wire.h>
#include <RtcDS1307.h>
#include <SPI.h>
#include <Adafruit_GrayOLED.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

#define TFT_CS   15
#define TFT_DC   33

RtcDS1307<TwoWire> Rtc(Wire);

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

TaskHandle_t TaskRTCUpdate_Handler;

void TaskRTCUpdate(void *pvParameters);
void TaskTFTUpdate(void *pvParameters);

unsigned long testFillScreen() {
  unsigned long start = micros();
  tft.fillScreen(ILI9341_BLACK);
  yield();
  tft.fillScreen(ILI9341_RED);
  yield();
  tft.fillScreen(ILI9341_GREEN);
  yield();
  tft.fillScreen(ILI9341_BLUE);
  yield();
  tft.fillScreen(ILI9341_BLACK);
  yield();
  return micros() - start;
}

void setup() {
  Serial.begin(9600);
  Rtc.Begin();
  tft.begin();
  Serial.println(testFillScreen());
  xTaskCreate(TaskRTCUpdate, "RTCUpdate", 128, NULL, 5, NULL); 
  xTaskCreate(TaskTFTUpdate, "TFTUpdate", 128, NULL, 5, NULL); 
}

void TaskRTCUpdate(void *pvParameters) {
  (void) pvParameters;
  for(;;) {
    RtcDateTime now = Rtc.GetDateTime();
    Serial.print(now);
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

void TaskTFTUpdate(void *pvParameters) {
  
}

void loop() {
  // put your main code here, to run repeatedly:
}