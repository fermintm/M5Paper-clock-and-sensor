#include "M5EPD.h"
#include "I2C_BM8563.h"
#include "binaryttf.h"
#include "ImageResource.h"
#include <WiFi.h>
#include "SPIFFS.h"
#include "FS.h"

#define BM8563_I2C_SDA 21
#define BM8563_I2C_SCL 22

M5EPD_Canvas canvas(&M5.EPD);

static constexpr int TEXTSIZE_BATTERY = 24;
static constexpr int TEXTSIZE_DATE = 72;
static constexpr int TEXTSIZE_TIME = 180;
static constexpr int TEXTSIZE_SENSOR_LABEL = 24;
static constexpr int TEXTSIZE_SENSOR_VALUE = 48;

char temStr[10];
char humStr[10];

float tem; // Temperatura
float hum; // Humedad

I2C_BM8563 rtc(I2C_BM8563_DEFAULT_ADDRESS, Wire);

// Date
char yyStr[5];  // YYYY
char moStr[3];  // MM
char ddStr[3];  // DD
// Time
char hhStr[3];  // HH
char mmStr[3];  // MM
char ssStr[3];  // SS
struct tm timeInfo;

// Timezone
const int tz = 1;

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = tz * 3600;
const int   daylightOffset_sec = 3600;

int disp_flag = 0;

void butt()
{
  if (M5.BtnL.wasPressed())
  {
    adjustt();
  }
  if (M5.BtnR.pressedFor(2000))
  {
    M5.EPD.Clear(true);
    canvas.deleteCanvas();
    canvas.createCanvas(960, 540);
    canvas.drawJpgFile(SPIFFS, "/fer.jpg", 0, 0);
    canvas.pushCanvas(0, 0, UPDATE_MODE_GLD16);
    delay(1000);
    M5.shutdown();
    M5.disableEPDPower();
    M5.disableEXTPower();
    M5.disableMainPower();
    esp_deep_sleep_start();
  }
}

void adjustt()
{
  constexpr uint_fast16_t WIFI_RETRY_MAX = 10; // 10 = 5s
  constexpr uint_fast16_t WAIT_ON_FAILURE = 2000;

  M5.EPD.Clear(true);

  WiFi.begin("MiPelanas", "routerestaca");

  for (int cnt_retry = 0;
       cnt_retry < WIFI_RETRY_MAX && !WiFi.isConnected();
       cnt_retry++)
  {
    delay(500);
  }
  canvas.println("");
  if (WiFi.isConnected())
  {
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);

  }
  else
  {
    delay(WAIT_ON_FAILURE);
  }

}
void batt()
{

  canvas.setTextSize(TEXTSIZE_BATTERY);
  char buf[20];
  canvas.fillCanvas(0);
  //    _bar->drawFastHLine(0, 43, 540, 15);
  canvas.setTextDatum(CC_DATUM);
  //    canvas.setTextDatum(CL_DATUM);

  canvas.setTextDatum(CR_DATUM);
  canvas.pushImage(498, 8 + 7, 32, 32, ImageResource_status_bar_battery_32x32);
  uint32_t vol = M5.getBatteryVoltage();

  if (vol < 3300)
  {
    vol = 3300;
  }
  else if (vol > 4350)
  {
    vol = 4350;
  }
  float battery = (float)(vol - 3300) / (float)(4350 - 3300);
  if (battery <= 0.01)
  {
    battery = 0.01;
  }
  if (battery > 1)
  {
    battery = 1;
  }
  uint8_t px = battery * 25;
  sprintf(buf, "%d%%", (int)(battery * 100));
  canvas.drawString(buf, 498 - 10, 25 + 7);
  canvas.fillRect(498 + 3, 8 + 10 + 7, px, 13, 15);
  // _bar->pushImage(498, 8, 32, 32, 2, ImageResource_status_bar_battery_charging_32x32);
}

void setup()
{

  M5.begin();

  M5.EPD.Clear(true);

  SPIFFS.begin();
  SD.begin();

  M5.EPD.Clear();
  canvas.createCanvas(960, 540);

  canvas.loadFont(binaryttf, sizeof(binaryttf));
  canvas.drawJpgFile(SPIFFS, "/fer2.jpg", 0, 0);
  canvas.pushCanvas(0, 0, UPDATE_MODE_GLD16);
  delay(1500);
  canvas.deleteCanvas();

  Wire.begin(BM8563_I2C_SDA, BM8563_I2C_SCL);

  rtc.begin();

  M5.disableEXTPower();

  M5.SHT30.Begin();

  canvas.createCanvas(960, 540);
  canvas.createRender(960, 540);
  canvas.createRender(TEXTSIZE_BATTERY);
  canvas.createRender(TEXTSIZE_DATE);
  canvas.createRender(TEXTSIZE_TIME);
  canvas.createRender(TEXTSIZE_SENSOR_LABEL);
  canvas.createRender(TEXTSIZE_SENSOR_VALUE);

  // Get local time
  struct tm timeInfo;
  if (getLocalTime(&timeInfo)) {
    // Set RTC time
    I2C_BM8563_TimeTypeDef timeStruct;
    timeStruct.hours   = timeInfo.tm_hour;
    timeStruct.minutes = timeInfo.tm_min;
    timeStruct.seconds = timeInfo.tm_sec;
    rtc.setTime(&timeStruct);

    // Set RTC Date
    I2C_BM8563_DateTypeDef dateStruct;
    dateStruct.weekDay = timeInfo.tm_wday;
    dateStruct.month   = timeInfo.tm_mon + 1;
    dateStruct.date    = timeInfo.tm_mday;
    dateStruct.year    = timeInfo.tm_year + 1900;
    rtc.setDate(&dateStruct);
  }

  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
}

void loop()
{

  M5.update();
  butt();
  batt();

  I2C_BM8563_DateTypeDef dateStruct;
  I2C_BM8563_TimeTypeDef timeStruct;

  // Get RTC
  rtc.getDate(&dateStruct);
  rtc.getTime(&timeStruct);

  canvas.setTextDatum(CR_DATUM);
  canvas.setTextSize(TEXTSIZE_DATE);
  //canvas.createRender(100, 50);
  sprintf(yyStr, "%04d", dateStruct.year + 0000);
  sprintf(moStr, "%02d", dateStruct.month + 1);
  sprintf(ddStr, "%02d", dateStruct.date);
  canvas.drawString(String(ddStr) + "-" + String(moStr) + "-" + String(yyStr), 670, 100);

  canvas.setTextDatum(TC_DATUM);
  canvas.setTextSize(TEXTSIZE_TIME);
  //canvas.createRender(960 / 2, 170);
  sprintf(hhStr, "%02d", timeStruct.hours);
  sprintf(mmStr, "%02d", timeStruct.minutes);
  sprintf(ssStr, "%02d", timeStruct.seconds);
  canvas.drawString(String(hhStr) + ":" + String(mmStr), 960 / 2, 170);

  //canvas.setTextDatum(BC_DATUM);
  canvas.setTextSize(TEXTSIZE_SENSOR_VALUE);
  //canvas.createRender(540, 960);
  M5.SHT30.UpdateData();
  tem = M5.SHT30.GetTemperature();
  hum = M5.SHT30.GetRelHumidity();
  dtostrf(tem, 2, 1 , temStr);
  dtostrf(hum, 2, 1 , humStr);
  canvas.drawString("   " + String(temStr) + "*C", 200, 480);
  canvas.drawString("   " + String(humStr) + "%" , 750, 480);
  canvas.drawJpgFile(SPIFFS, "/T.jpg", 150, 325);
  canvas.drawJpgFile(SPIFFS, "/H.jpg", 690, 325);

  //canvas.pushCanvas(0, 0, UPDATE_MODE_INIT); = 0, // * N/A 2000ms Display initialization
  //canvas.pushCanvas(0, 0, UPDATE_MODE_DU); // Low 260ms Monochrome menu, text input, and touch screen input
  //canvas.pushCanvas(0, 0, UPDATE_MODE_GC16); // * Very Low 450ms High quality images
  //canvas.pushCanvas(0, 0, UPDATE_MODE_GL16); // * Medium 450ms Text with white background
  //canvas.pushCanvas(0, 0, UPDATE_MODE_GLR16); // Low 450ms Text with white background
  //canvas.pushCanvas(0, 0, UPDATE_MODE_GLD16); // Low 450ms Text and graphics with white background
  //canvas.pushCanvas(0, 0, UPDATE_MODE_DU4); // * Medium 120ms Fast page flipping at reduced contrast
  canvas.pushCanvas(0, 0, UPDATE_MODE_A2); // Medium 290ms Anti-aliased text in menus / touch and screen input
  //canvas.pushCanvas(0, 0, UPDATE_MODE_NONE);

  if (timeStruct.minutes == 00 && disp_flag == 0) {
    //disp_moo();
    disp_flag = 1;
  }
  delay(5000);
}
