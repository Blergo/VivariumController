#include <Arduino.h>
#include <WiFi.h>
#include <Adafruit_SPIDevice.h>
#include <RTClib.h>
#include <time.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <lv_conf.h>
#include <lvgl.h>
#include <EEPROM.h>

#define TOUCH_CS  34
#define TOUCH_IRQ 35

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 3600;

const char* ssid = "InfiniteWisdom";
const char* password = "K1net1CK1net1C";

const int MY_DISP_HOR_RES = 320;
const int MY_DISP_VER_RES = 240;

TFT_eSPI tft = TFT_eSPI(); 
XPT2046_Touchscreen ts(TOUCH_CS, TOUCH_IRQ);
RTC_DS1307 rtc;

float xCalM = 0.0, yCalM = 0.0;
float xCalC = 0.0, yCalC = 0.0;

const int blPin = 32;
const int blFreq = 5000;
const int blChannel = 0;
const int blResolution = 8;
int curDuty = 0;
int setDuty = 255;
int blDuration = 20000;
int blTimeout = 0;

int NTPisRun = 0;

TaskHandle_t TaskHandle_1;
TaskHandle_t TaskHandle_2;
TaskHandle_t TaskHandle_3;
TaskHandle_t TaskHandle_4;
TaskHandle_t TaskHandle_5;

lv_obj_t *tabview;
lv_obj_t *tab1;
lv_obj_t *tab2;
lv_obj_t *WiFisw;
lv_obj_t *WiFilabel;
lv_obj_t *NTPsw;
lv_obj_t *NTPlabel;
lv_obj_t *CalBtn;
lv_obj_t *CalLabel;

static lv_disp_draw_buf_t disp_buf;
static lv_color_t buf_1[MY_DISP_HOR_RES * 10];
static lv_disp_drv_t disp_drv;
static lv_disp_t *disp;
static lv_indev_drv_t indev_drv;

struct tm timeinfo;

void BuildUI(void * parameter);
void TFTUpdate(void * parameter);
void CheckRTC(void * parameter);
void initWiFi(void * parameter);
void blPWM(void * parameter);

class ScreenPoint {

  public:
  int16_t x;
  int16_t y;
 
  ScreenPoint(){}
 
  ScreenPoint(int16_t xIn, int16_t yIn){
    x = xIn;
    y = yIn;
    }
};

ScreenPoint getScreenCoords(int16_t x, int16_t y){
int16_t xCoord = round((x * xCalM) + xCalC);
int16_t yCoord = round((y * yCalM) + yCalC);
if(xCoord < 0) xCoord = 0;
if(xCoord >= tft.width()) xCoord = tft.width() - 1;
if(yCoord < 0) yCoord = 0;
if(yCoord >= tft.height()) yCoord = tft.height() - 1;
return(ScreenPoint(xCoord, yCoord));
}

void calibrateTouchScreen(){
  TS_Point p;
  int16_t x1,y1,x2,y2;
 
  tft.fillScreen(ILI9341_BLACK);
  // wait for no touch
  while(ts.touched());
  tft.drawFastHLine(10,20,20,ILI9341_RED);
  tft.drawFastVLine(20,10,20,ILI9341_RED);
  while(!ts.touched());
  delay(50);
  p = ts.getPoint();
  x1 = p.x;
  y1 = p.y;
  tft.drawFastHLine(10,20,20,ILI9341_BLACK);
  tft.drawFastVLine(20,10,20,ILI9341_BLACK);
  delay(500);
  while(ts.touched());
  tft.drawFastHLine(tft.width() - 30,tft.height() - 20,20,ILI9341_RED);
  tft.drawFastVLine(tft.width() - 20,tft.height() - 30,20,ILI9341_RED);
  while(!ts.touched());
  delay(50);
  p = ts.getPoint();
  x2 = p.x;
  y2 = p.y;
  tft.drawFastHLine(tft.width() - 30,tft.height() - 20,20,ILI9341_BLACK);
  tft.drawFastVLine(tft.width() - 20,tft.height() - 30,20,ILI9341_BLACK);
  int16_t xDist = tft.width() - 40;
  int16_t yDist = tft.height() - 40;

  xCalM = (float)xDist / (float)(x2 - x1);
  xCalC = 20.0 - ((float)x1 * xCalM);

  yCalM = (float)yDist / (float)(y2 - y1);
  yCalC = 20.0 - ((float)y1 * yCalM);

  EEPROM.put(0, xCalM);
  EEPROM.put(6, yCalM);
  EEPROM.put(11,xCalC);
  EEPROM.put(18,yCalC);
  EEPROM.commit();
  lv_obj_invalidate(tabview);
}

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

void touchpad_read(lv_indev_drv_t * drv, lv_indev_data_t*data){
  if (ts.touched()) {
    if (blTimeout == 0){
      setDuty=255;
    }
    blTimeout = millis()+blDuration;
    ScreenPoint sp = ScreenPoint();
    TS_Point p = ts.getPoint();
    sp = getScreenCoords(p.x, p.y);
    data->point.x = sp.x;
    data->point.y = sp.y;
    data->state = LV_INDEV_STATE_PRESSED;
  } else {
    data->state = LV_INDEV_STATE_RELEASED; 
  }
}

void setup() {
  Serial.begin(9600);
  tft.init();
  tft.setRotation(1);
  ts.begin();
  ts.setRotation(1);

  ledcSetup(blChannel, blFreq, blResolution);
  ledcAttachPin(blPin, blChannel);
  xTaskCreate(blPWM, "Backlight PWM", 2000, NULL, 1, &TaskHandle_3);

  EEPROM.begin(23);
  EEPROM.get(0, xCalM);
  EEPROM.get(6, yCalM);
  EEPROM.get(11,xCalC);
  EEPROM.get(18,yCalC);

  lv_init();
  lv_disp_draw_buf_init(&disp_buf, buf_1, NULL, MY_DISP_HOR_RES*10);
  lv_disp_drv_init(&disp_drv);
  disp_drv.draw_buf = &disp_buf;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.hor_res = MY_DISP_HOR_RES;
  disp_drv.ver_res = MY_DISP_VER_RES;
  disp = lv_disp_drv_register(&disp_drv);

  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = touchpad_read;
  lv_indev_drv_register(&indev_drv);

  blTimeout = millis()+blDuration;

  xTaskCreate(BuildUI, "Build UI", 2000, NULL, 5, &TaskHandle_1);
  xTaskCreate(initWiFi, "Initialize WiFi", 2000, NULL, 5, &TaskHandle_2);

  if(!rtc.begin()) {
      Serial.println("Couldn't find RTC!");
      Serial.flush();
      while (1) delay(10);
  }

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  xTaskCreate(TFTUpdate, "TFT Update", 2500, NULL, 3, &TaskHandle_5);
}

static void event_handler_btn(lv_event_t * e){
  lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_CLICKED){
      calibrateTouchScreen();
    }
}

static void event_handler_sw(lv_event_t * e){

    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);
    if(code == LV_EVENT_VALUE_CHANGED) {
      if(obj == WiFisw && lv_obj_has_state(obj, LV_STATE_CHECKED)) {
        lv_obj_clear_state(NTPsw, LV_STATE_DISABLED);
      }
      else if(obj == WiFisw){
        lv_obj_clear_state(NTPsw, LV_STATE_CHECKED);
        lv_obj_add_state(NTPsw, LV_STATE_DISABLED);
        if(NTPisRun == 1){
          vTaskDelete(TaskHandle_4);
          NTPisRun = 0;
        }
      }
      else if(obj == NTPsw && lv_obj_has_state(obj, LV_STATE_CHECKED)) {
        xTaskCreate(CheckRTC, "Check RTC", 2000, NULL, 4, &TaskHandle_4);       
      }
      else if (obj == NTPsw){
        vTaskDelete(TaskHandle_4);
        NTPisRun = 0;
      }
    }
}

void BuildUI(void * parameter) {

    tabview = lv_tabview_create(lv_scr_act(), LV_DIR_TOP, 50);

    tab1 = lv_tabview_add_tab(tabview, "Test");
    tab2 = lv_tabview_add_tab(tabview, "Settings");
    
    WiFisw = lv_switch_create(tab2);
    lv_obj_add_event_cb(WiFisw, event_handler_sw, LV_EVENT_ALL, NULL);

    WiFilabel = lv_label_create(tab2);
    lv_label_set_text(WiFilabel, "WiFi");
    lv_obj_align_to(WiFilabel, WiFisw, LV_ALIGN_OUT_RIGHT_MID, 20, 0);

    NTPsw = lv_switch_create(tab2);
    lv_obj_align_to(NTPsw, WiFisw, LV_ALIGN_OUT_BOTTOM_MID, 0, 20);
    lv_obj_add_event_cb(NTPsw, event_handler_sw, LV_EVENT_ALL, NULL);
    lv_obj_add_state(NTPsw, LV_STATE_DISABLED);

    NTPlabel = lv_label_create(tab2);
    lv_label_set_text(NTPlabel, "NTP");
    lv_obj_align_to(NTPlabel, NTPsw, LV_ALIGN_OUT_RIGHT_MID, 20, 0);

    CalBtn = lv_btn_create(tab2);
    lv_obj_add_event_cb(CalBtn, event_handler_btn, LV_EVENT_ALL, NULL);

    CalLabel = lv_label_create(CalBtn);
    lv_obj_align_to(CalBtn, NTPsw, LV_ALIGN_OUT_BOTTOM_RIGHT, 0, 20);
    lv_label_set_text(CalLabel, "Calibrate Touch");
    lv_obj_center(CalLabel);

    vTaskDelete(NULL);
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
  for(;;){
    if (millis() > blTimeout && blTimeout != 0){
      blTimeout = 0;
      setDuty = 50;
    }
    if (setDuty != curDuty) {
      ledcWrite(blChannel, setDuty);
      curDuty = setDuty;
    }
    vTaskDelay(50);
  }
}

void TFTUpdate(void * parameter) {
  TickType_t xLastWakeTime1;
  const portTickType xFrequency1 = 10 / portTICK_RATE_MS;
  xLastWakeTime1 = xTaskGetTickCount ();
  for(;;) {
    vTaskDelayUntil( &xLastWakeTime1, xFrequency1 );
    lv_timer_handler();
  }
}

void CheckRTC(void * parameter) {
  NTPisRun = 1;
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
      Serial.println("Updating RTC..");
      rtc.adjust(DateTime(timeinfo.tm_year+1900, timeinfo.tm_mon+1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec));
    }
  }
}

void loop() {
}