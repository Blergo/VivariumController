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
#include <ModbusRtu.h>

#define TOUCH_CS  34
#define TOUCH_IRQ 35
#define MODBUS_BAUD 9600
#define MODBUS_TIMEOUT 1000
#define MODBUS_POLLING 1000
#define MODBUS_RETRY 10
#define MODBUS_TXEN -1
#define MODBUS_TX 25
#define MODBUS_RX 26

uint16_t au16data[16];
uint8_t u8state;
Modbus master(0,Serial1,0);
modbus_t telegram;
unsigned long u32wait;

char ntpServer[] = "pool.ntp.org";
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 3600;
char ssid[32];
char password[32];

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

bool WiFiState;
bool NTPState;

TaskHandle_t TaskHandle_1;
TaskHandle_t TaskHandle_2;
TaskHandle_t TaskHandle_3;
TaskHandle_t TaskHandle_4;
TaskHandle_t TaskHandle_5;
TaskHandle_t TaskHandle_6;
TaskHandle_t TaskHandle_7;
TaskHandle_t TaskHandle_8;

lv_obj_t * keyboard;
lv_obj_t * tabview;
lv_obj_t * tab1;
lv_obj_t * tab2;
lv_obj_t * WiFisw;
lv_obj_t * WiFilabel;
lv_obj_t * WiFiSetBtn;
lv_obj_t * WiFiSetLabel;
lv_obj_t * WiFiSetBkBtn;
lv_obj_t * WiFiSetBkLabel;
lv_obj_t * WiFiCnctBtn;
lv_obj_t * WiFiCnctLabel;
lv_obj_t * NTPsw;
lv_obj_t * NTPlabel;
lv_obj_t * CalBtn;
lv_obj_t * CalLabel;
lv_obj_t * SaveBtn;
lv_obj_t * SaveLabel;
lv_obj_t * WiFiSSID;
lv_obj_t * WiFiSSIDLabel;
lv_obj_t * WiFiPass;
lv_obj_t * WiFiPassLabel;


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
void disWiFi(void * parameter);
void blPWM(void * parameter);
void SaveSettings(void * parameter);
void ModbusUpdate(void * parameter);

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
  Serial1.begin(9600, SERIAL_8N1, MODBUS_RX, MODBUS_TX);
  tft.init();
  tft.setRotation(1);
  ts.begin();
  ts.setRotation(1);

  ledcSetup(blChannel, blFreq, blResolution);
  ledcAttachPin(blPin, blChannel);
  xTaskCreate(blPWM, "Backlight PWM", 1500, NULL, 1, &TaskHandle_3);

  EEPROM.begin(90);
  EEPROM.get(0, xCalM);
  EEPROM.get(6, yCalM);
  EEPROM.get(11, xCalC);
  EEPROM.get(18, yCalC);
  EEPROM.get(24, WiFiState);

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

  xTaskCreate(BuildUI, "Build UI", 2500, NULL, 6, &TaskHandle_1);
  blTimeout = millis()+blDuration;

  xTaskCreate(ModbusUpdate, "Modbus Update", 2000, NULL, 5, &TaskHandle_8);

  if(!rtc.begin()) {
      Serial.println("Couldn't find RTC!");
      Serial.flush();
      while (1) delay(10);
  }

  vTaskDelay(100);

  if(WiFiState == true){
    lv_obj_add_state(WiFisw, LV_STATE_CHECKED);
    EEPROM.get(26,ssid);
    EEPROM.get(58,password);
    lv_textarea_set_placeholder_text(WiFiSSID, ssid);
    lv_textarea_set_placeholder_text(WiFiPass, password);
    EEPROM.get(25, NTPState);
    xTaskCreate(initWiFi, "Initialize WiFi", 2000, NULL, 5, &TaskHandle_2);
  }
  else if(WiFiState == false){
    lv_obj_add_state(NTPsw, LV_STATE_DISABLED);
    lv_obj_add_state(WiFiSetBtn, LV_STATE_DISABLED);
  }
  if(NTPState == true){
    lv_obj_add_state(NTPsw, LV_STATE_CHECKED);
    xTaskCreate(CheckRTC, "Check RTC", 2000, NULL, 4, &TaskHandle_4);
  }

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  xTaskCreate(TFTUpdate, "TFT Update", 2500, NULL, 3, &TaskHandle_5);
}

static void event_handler_btn(lv_event_t * e){
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t * obj = lv_event_get_target(e);
    if(code == LV_EVENT_CLICKED && obj == CalBtn){
      calibrateTouchScreen();
    }
    else if(code == LV_EVENT_CLICKED && obj == SaveBtn){
      xTaskCreate(SaveSettings, "Save Settings", 2000, NULL, 2, &TaskHandle_7);
    }
    else if(code == LV_EVENT_CLICKED && obj == WiFiSetBtn){
      lv_obj_add_flag(WiFisw, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(WiFilabel, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(NTPsw, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(NTPlabel, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(CalBtn, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(SaveBtn, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(WiFiSetBtn, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(WiFiSetBkBtn, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(WiFiCnctBtn, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(WiFiSSID, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(WiFiSSIDLabel, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(WiFiPass, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(WiFiPassLabel, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
    }
    else if(code == LV_EVENT_CLICKED && obj == WiFiSetBkBtn){
      lv_obj_clear_flag(WiFisw, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(WiFilabel, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(NTPsw, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(NTPlabel, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(CalBtn, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(SaveBtn, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(WiFiSetBtn, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(WiFiSetBkBtn, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(WiFiCnctBtn, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(WiFiSSID, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(WiFiSSIDLabel, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(WiFiPass, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(WiFiPassLabel, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
      lv_textarea_set_placeholder_text(WiFiSSID, ssid);
      lv_textarea_set_placeholder_text(WiFiPass, password);
    }
    else if(code == LV_EVENT_CLICKED && obj == WiFiCnctBtn){
      strcpy(ssid, lv_textarea_get_text(WiFiSSID));
      strcpy(password, lv_textarea_get_text(WiFiPass));
      xTaskCreate(initWiFi, "Initialize WiFi", 2000, NULL, 5, &TaskHandle_2);
    }
}

static void event_handler_sw(lv_event_t * e){

    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);
    if(code == LV_EVENT_VALUE_CHANGED) {
      if(obj == WiFisw && lv_obj_has_state(obj, LV_STATE_CHECKED)) {
        lv_obj_clear_state(NTPsw, LV_STATE_DISABLED);
        lv_obj_clear_state(WiFiSetBtn, LV_STATE_DISABLED);
        xTaskCreate(initWiFi, "Initialize WiFi", 2000, NULL, 5, &TaskHandle_2);
        WiFiState = true;
      }
      else if(obj == WiFisw){
        lv_obj_clear_state(NTPsw, LV_STATE_CHECKED);
        lv_obj_add_state(NTPsw, LV_STATE_DISABLED);
        lv_obj_add_state(WiFiSetBtn, LV_STATE_DISABLED);
        xTaskCreate(disWiFi, "Disable WiFi", 2000, NULL, 5, &TaskHandle_6);
        WiFiState = false;
        if(NTPState == true){
          vTaskDelete(TaskHandle_4);
          NTPState = false;
        }
      }
      else if(obj == NTPsw && lv_obj_has_state(obj, LV_STATE_CHECKED)) {
        xTaskCreate(CheckRTC, "Check RTC", 2000, NULL, 4, &TaskHandle_4);      
      }
      else if (obj == NTPsw){
        vTaskDelete(TaskHandle_4);
        NTPState = false;
      }
    }
}

static void ta_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
    if(code == LV_EVENT_CLICKED || code == LV_EVENT_FOCUSED) {
        if(keyboard != NULL) lv_keyboard_set_textarea(keyboard, ta);
    }
}

void BuildUI(void * parameter) {

    tabview = lv_tabview_create(lv_scr_act(), LV_DIR_TOP, 30);

    tab1 = lv_tabview_add_tab(tabview, "Test");
    tab2 = lv_tabview_add_tab(tabview, "Settings");
    
    WiFisw = lv_switch_create(tab2);
    lv_obj_add_event_cb(WiFisw, event_handler_sw, LV_EVENT_ALL, NULL);

    WiFilabel = lv_label_create(tab2);
    lv_label_set_text(WiFilabel, "WiFi");
    lv_obj_align_to(WiFilabel, WiFisw, LV_ALIGN_OUT_RIGHT_MID, 20, 0);

    WiFiSetBtn = lv_btn_create(tab2);
    lv_obj_add_event_cb(WiFiSetBtn, event_handler_btn, LV_EVENT_ALL, NULL);
    lv_obj_align_to(WiFiSetBtn, WiFisw, LV_ALIGN_OUT_RIGHT_MID, 100, 0);

    WiFiSetLabel = lv_label_create(WiFiSetBtn);
    lv_label_set_text(WiFiSetLabel, "WiFi Settings");
    lv_obj_center(WiFiSetLabel);

    NTPsw = lv_switch_create(tab2);
    lv_obj_align_to(NTPsw, WiFisw, LV_ALIGN_OUT_BOTTOM_MID, 0, 20);
    lv_obj_add_event_cb(NTPsw, event_handler_sw, LV_EVENT_ALL, NULL);

    NTPlabel = lv_label_create(tab2);
    lv_label_set_text(NTPlabel, "NTP");
    lv_obj_align_to(NTPlabel, NTPsw, LV_ALIGN_OUT_RIGHT_MID, 20, 0);

    CalBtn = lv_btn_create(tab2);
    lv_obj_add_event_cb(CalBtn, event_handler_btn, LV_EVENT_ALL, NULL);
    lv_obj_align_to(CalBtn, NTPsw, LV_ALIGN_OUT_BOTTOM_RIGHT, 0, 20);

    CalLabel = lv_label_create(CalBtn);
    lv_label_set_text(CalLabel, "Calibrate Touch");
    lv_obj_center(CalLabel);

    SaveBtn = lv_btn_create(tab2);
    lv_obj_add_event_cb(SaveBtn, event_handler_btn, LV_EVENT_ALL, NULL);
    lv_obj_align_to(SaveBtn, NTPsw, LV_ALIGN_OUT_BOTTOM_RIGHT, 150, 20);

    SaveLabel = lv_label_create(SaveBtn);
    lv_label_set_text(SaveLabel, "Save Settings");
    lv_obj_center(SaveLabel);

    WiFiSSID = lv_textarea_create(tab2);
    lv_textarea_set_one_line(WiFiSSID, true);
    lv_textarea_set_password_mode(WiFiSSID, false);
    lv_obj_set_width(WiFiSSID, lv_pct(60));
    lv_textarea_set_max_length(WiFiSSID, 32);
    lv_obj_add_event_cb(WiFiSSID, ta_event_cb, LV_EVENT_ALL, NULL);
    lv_obj_align(WiFiSSID, LV_ALIGN_TOP_LEFT, 0, 10);
    lv_obj_add_flag(WiFiSSID, LV_OBJ_FLAG_HIDDEN);

    WiFiSSIDLabel = lv_label_create(tab2);
    lv_label_set_text(WiFiSSIDLabel, "WiFi SSID:");
    lv_obj_align_to(WiFiSSIDLabel, WiFiSSID, LV_ALIGN_OUT_TOP_LEFT, 0, 0);
    lv_obj_add_flag(WiFiSSIDLabel, LV_OBJ_FLAG_HIDDEN);

    WiFiPass = lv_textarea_create(tab2);
    lv_textarea_set_one_line(WiFiPass, true);
    lv_textarea_set_password_mode(WiFiPass, false);
    lv_obj_set_width(WiFiPass, lv_pct(60));
    lv_textarea_set_max_length(WiFiPass, 32);
    lv_obj_add_event_cb(WiFiPass, ta_event_cb, LV_EVENT_ALL, NULL);
    lv_obj_align_to(WiFiPass, WiFiSSID, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 20);
    lv_obj_add_flag(WiFiPass, LV_OBJ_FLAG_HIDDEN);

    WiFiPassLabel = lv_label_create(tab2);
    lv_label_set_text(WiFiPassLabel, "WiFi Password:");
    lv_obj_align_to(WiFiPassLabel, WiFiPass, LV_ALIGN_OUT_TOP_LEFT, 0, 0);
    lv_obj_add_flag(WiFiPassLabel, LV_OBJ_FLAG_HIDDEN);

    WiFiCnctBtn = lv_btn_create(tab2);
    lv_obj_add_event_cb(WiFiCnctBtn, event_handler_btn, LV_EVENT_ALL, NULL);
    lv_obj_align(WiFiCnctBtn, LV_ALIGN_TOP_RIGHT, 0, 20);
    lv_obj_add_flag(WiFiCnctBtn, LV_OBJ_FLAG_HIDDEN);

    WiFiCnctLabel = lv_label_create(WiFiCnctBtn);
    lv_label_set_text(WiFiCnctLabel, "Connect");
    lv_obj_center(WiFiCnctLabel);

    WiFiSetBkBtn = lv_btn_create(tab2);
    lv_obj_add_event_cb(WiFiSetBkBtn, event_handler_btn, LV_EVENT_ALL, NULL);
    lv_obj_align_to(WiFiSetBkBtn, WiFiCnctBtn, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 20);
    lv_obj_add_flag(WiFiSetBkBtn, LV_OBJ_FLAG_HIDDEN);

    WiFiSetBkLabel = lv_label_create(WiFiSetBkBtn);
    lv_label_set_text(WiFiSetBkLabel, "Back");
    lv_obj_center(WiFiSetBkLabel);

    keyboard = lv_keyboard_create(tab2);
    lv_obj_set_size(keyboard, LV_HOR_RES, LV_VER_RES / 3.4);
    lv_keyboard_set_textarea(keyboard, WiFiSSID);
    lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);

    vTaskDelete(NULL);
}

void initWiFi(void * parameter) {
  if (WiFi.status() == WL_CONNECTED){
    xTaskCreate(disWiFi, "Disable WiFi", 2000, NULL, 5, &TaskHandle_6);
  }
  while (WiFi.status() == WL_CONNECTED){
    vTaskDelay(100);
  }
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  if (WiFi.waitForConnectResult() == WL_CONNECTED) {
    Serial.println("WiFi Connected!");
  } 
    else {
    Serial.println("Unable to connect!");
  }
  vTaskDelete(NULL);
}

void disWiFi(void * parameter) {
  WiFi.disconnect();
  Serial.print("Disconnecting WiFi ..");
  while (WiFi.status() == WL_CONNECTED) {
    Serial.print('.');
    delay(500);
  }
  WiFi.mode(WIFI_OFF);
  Serial.println("WiFi Disconnected!");
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
    vTaskDelay(100);
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
  NTPState = true;
  Serial.println("NTP Client Running");
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

void SaveSettings(void * parameter) {
  EEPROM.put(0, xCalM);
  EEPROM.put(6, yCalM);
  EEPROM.put(11, xCalC);
  EEPROM.put(18, yCalC);
  EEPROM.put(24, WiFiState);
  EEPROM.put(25, NTPState);
  EEPROM.put(26, ssid);
  EEPROM.put(58, password);
  EEPROM.commit();
  Serial.println("Settings Saved");
  vTaskDelete(NULL);
}

void ModbusUpdate(void * parameter){
  master.start();
  master.setTimeOut( 2000 );
  u32wait = millis() + 1000;
  u8state = 0; 

  for(;;){
    switch( u8state ) {
      case 0: 
        if (millis() > u32wait) u8state++; // wait state
      break;
      case 1: 
        telegram.u8id = 1; // slave address
        telegram.u8fct = 3; // function code (this one is registers read)
        telegram.u16RegAdd = 0; // start address in slave
        telegram.u16CoilsNo = 5; // number of elements (coils or registers) to read
        telegram.au16reg = au16data; // pointer to a memory array in the Arduino

        master.query( telegram ); // send query (only once)
        u8state++;
      break;
      case 2:
        master.poll(); // check incoming messages
        if (master.getState() == COM_IDLE) {
          u8state = 0;
          u32wait = millis() + 100; 
        }
      break;
    }
    Serial.println(au16data[0]);
    Serial.println(au16data[1]);
    Serial.println(au16data[2]);
    Serial.println(au16data[3]);
    Serial.println(au16data[4]);
    vTaskDelay(200);
  }
}

void loop() {
}