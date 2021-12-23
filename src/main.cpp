#include <Arduino.h>
#include <WiFi.h>
#include <Adafruit_SPIDevice.h>
#include <RTClib.h>
#include <time.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <lv_conf.h>
#include <lvgl.h>

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 3600;

const char* ssid = "InfiniteWisdom";
const char* password = "K1net1CK1net1C";

const int MY_DISP_HOR_RES = 320;
const int MY_DISP_VER_RES = 240;

TFT_eSPI tft = TFT_eSPI(); 

RTC_DS1307 rtc;

const int blPin = -1;
const int blFreq = 5000;
const int blChannel = 0;
const int blResolution = 8;
int curDuty = 0;
int setDuty = 255;

TaskHandle_t TaskHandle_1;
TaskHandle_t TaskHandle_2;
TaskHandle_t TaskHandle_3;
TaskHandle_t TaskHandle_4;

static lv_disp_draw_buf_t disp_buf;
static lv_color_t buf_1[MY_DISP_HOR_RES * 10];
static lv_disp_drv_t disp_drv;
static lv_disp_t *disp;

lv_obj_t *screenTest;
lv_obj_t *labelTest;

struct tm timeinfo;

void TFTUpdate(void * parameter);
void CheckRTC(void * parameter);
void initWiFi(void * parameter);
void blPWM(void * parameter);

void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors(&color_p->full, w * h, true);
    tft.endWrite();

    lv_disp_flush_ready(disp);
}

void setup() {
  Serial.begin(9600);
  tft.init();
  tft.setRotation(1);

  lv_init();
  lv_disp_draw_buf_init(&disp_buf, buf_1, NULL, MY_DISP_HOR_RES*10);
  lv_disp_drv_init(&disp_drv);
  disp_drv.draw_buf = &disp_buf;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.hor_res = MY_DISP_HOR_RES;
  disp_drv.ver_res = MY_DISP_VER_RES;
  disp = lv_disp_drv_register(&disp_drv);

  ledcSetup(blChannel, blFreq, blResolution);
  ledcAttachPin(blPin, blChannel);

  ledcSetup(blChannel, blFreq, blResolution);
  ledcAttachPin(blPin, blChannel);

  xTaskCreate(initWiFi, "Initialize WiFi", 2000, NULL, 5, &TaskHandle_3);
  xTaskCreate(blPWM, "Backlight PWM", 2000, NULL, 1, &TaskHandle_4);

  if(!rtc.begin()) {
      Serial.println("Couldn't find RTC!");
      Serial.flush();
      while (1) delay(10);
  }

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  screenTest = lv_obj_create(NULL);

  labelTest = lv_label_create(screenTest);
  lv_label_set_long_mode(labelTest, LV_LABEL_LONG_WRAP);
  lv_label_set_text(labelTest, "Test Label!");
  lv_obj_set_align(labelTest, LV_ALIGN_CENTER);
  lv_obj_set_size(labelTest, 240, 40);
  lv_obj_set_pos(labelTest, 0, 15);

  xTaskCreate(CheckRTC, "Check RTC", 2000, NULL, 4, &TaskHandle_2);
  xTaskCreate(TFTUpdate, "TFT Update", 2000, NULL, 3, &TaskHandle_1);
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

void blPWM(void * parameter) {
  if (setDuty != curDuty) {
    ledcWrite(blChannel, setDuty);
    curDuty = setDuty;
  }
  vTaskDelay(50);
}

void TFTUpdate(void * parameter) {
  TickType_t xLastWakeTime1;
  const portTickType xFrequency1 = 5 / portTICK_RATE_MS;
  xLastWakeTime1 = xTaskGetTickCount ();
  for(;;) {
    vTaskDelayUntil( &xLastWakeTime1, xFrequency1 );
    lv_timer_handler();
  }
}

void CheckRTC(void * parameter) {
  while (WiFi.status() != WL_CONNECTED) {
    vTaskDelay(100);
  }
  TickType_t xLastWakeTime;
  const portTickType xFrequency = 3600000 / portTICK_RATE_MS;
  xLastWakeTime = xTaskGetTickCount ();
  for(;;){
    vTaskDelayUntil( &xLastWakeTime, xFrequency );
    if (! rtc.isrunning()) {
      Serial.println("RTC is NOT running, let's set the time!");
      if(!getLocalTime(&timeinfo)){
        Serial.println("Failed to obtain time");
      }
      rtc.adjust(DateTime(timeinfo.tm_year+1900, timeinfo.tm_mon+1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec));
    }
  }
}

void loop() {
}