#include <Arduino.h>

#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>

#include <EEPROM.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <ArduinoJson.h>

#include "main.h"

#include <lvgl.h>
static lv_style_t style_btn_BLUE;
static lv_style_t style_btn_RED;
static lv_style_t style_btn_GREEN;
static lv_style_t style_btn_YELLOW;
static lv_style_t style_btn_DARKGREY;

#include <demos/lv_demos.h>
static uint32_t screenWidth;
static uint32_t screenHeight;
static lv_disp_draw_buf_t draw_buf;
static lv_color_t disp_draw_buf[800 * 480 / 10];
static lv_disp_drv_t disp_drv;
// //UI
// #include <ui.h>
// static int first_flag = 0;
// extern int zero_clean;
// extern int goto_widget_flag;
// extern int bar_flag;
// extern lv_obj_t * ui_MENU;
// extern lv_obj_t * ui_TOUCH;
// extern lv_obj_t * ui_JIAOZHUN;
// extern lv_obj_t * ui_Label2;
// static lv_obj_t * ui_Label;//TOUCH界面label
// static lv_obj_t * ui_Label3;//TOUCH界面label3
// static lv_obj_t * ui_Labe2;//Menu界面进度条label
// static lv_obj_t * bar;//Menu界面进度条

#include <SPI.h>
SPIClass& spi = SPI;
uint16_t touchCalibration_x0 = 300, touchCalibration_x1 = 3600, touchCalibration_y0 = 300, touchCalibration_y1 = 3600;
uint8_t  touchCalibration_rotate = 1, touchCalibration_invert_x = 2, touchCalibration_invert_y = 0;
static int val = 100;
#include <Ticker.h>          //Call the ticker. H Library
Ticker ticker1;

int i = 0;
#include <Arduino_GFX_Library.h>
#define TFT_BL 2

#if defined(DISPLAY_DEV_KIT)
Arduino_GFX *lcd = create_default_Arduino_GFX();
#else /* !defined(DISPLAY_DEV_KIT) */

Arduino_ESP32RGBPanel *bus = new Arduino_ESP32RGBPanel(
  GFX_NOT_DEFINED /* CS */, GFX_NOT_DEFINED /* SCK */, GFX_NOT_DEFINED /* SDA */,
  41 /* DE */, 40 /* VSYNC */, 39 /* HSYNC */, 0 /* PCLK */,
  14 /* R0 */, 21 /* R1 */, 47 /* R2 */, 48 /* R3 */, 45 /* R4 */,
  9 /* G0 */, 46 /* G1 */, 3 /* G2 */, 8 /* G3 */, 16 /* G4 */, 1 /* G5 */,
  15 /* B0 */, 7 /* B1 */, 6 /* B2 */, 5 /* B3 */, 4 /* B4 */
);

// option 1:
// 7寸 50PIN 800*480
Arduino_RPi_DPI_RGBPanel *lcd = new Arduino_RPi_DPI_RGBPanel(
  bus,
  //  800 /* width */, 0 /* hsync_polarity */, 8/* hsync_front_porch */, 2 /* hsync_pulse_width */, 43/* hsync_back_porch */,
  //  480 /* height */, 0 /* vsync_polarity */, 8 /* vsync_front_porch */, 2/* vsync_pulse_width */, 12 /* vsync_back_porch */,
  //  1 /* pclk_active_neg */, 16000000 /* prefer_speed */, true /* auto_flush */);

  //  800 /* width */, 0 /* hsync_polarity */, 210 /* hsync_front_porch */, 30 /* hsync_pulse_width */, 16 /* hsync_back_porch */,
  //  480 /* height */, 0 /* vsync_polarity */, 22 /* vsync_front_porch */, 13 /* vsync_pulse_width */, 10 /* vsync_back_porch */,
  //  1 /* pclk_active_neg */, 16000000 /* prefer_speed */, true /* auto_flush */);
  //  800 /* width */, 1 /* hsync_polarity */, 80 /* hsync_front_porch */, 48 /* hsync_pulse_width */,  40/* hsync_back_porch */,
  //  480 /* height */, 1 /* vsync_polarity */, 50 /* vsync_front_porch */, 1 /* vsync_pulse_width */, 31 /* vsync_back_porch */,
  //  0 /* pclk_active_neg */, 30000000 /* prefer_speed */, true /* auto_flush */);
  800 /* width */, 1 /* hsync_polarity */, 40 /* hsync_front_porch */, 48 /* hsync_pulse_width */, 40 /* hsync_back_porch */,
  480 /* height */, 1 /* vsync_polarity */, 13 /* vsync_front_porch */, 1 /* vsync_pulse_width */, 31 /* vsync_back_porch */,
  1 /* pclk_active_neg */, 6000000 /* prefer_speed */, true /* auto_flush */);
  //1 /* pclk_active_neg */, 16000000 /* prefer_speed */, true /* auto_flush */);

#endif /* !defined(DISPLAY_DEV_KIT) */
#include "touch.h"

void monitoringData(void);
void setStatusColor(uint8_t row, uint8_t col, uint8_t status);

WebServer server(80);
char webBuffer[250];
String bodyPOST;

struct cooler // Для работы в ОЗУ
{
  char name[10];
  int8_t status;
  float tempCur;
  int8_t tempMax;
  int8_t tempMin;
  uint32_t timeErrorStart;
  uint32_t timeErrorMax;
  uint32_t timeLastSync;
};
struct coolerData // Для хранения в EEPROM
{
  // uint8_t relay; // Контакт открытия 1/0
  int8_t tempMax;
  int8_t tempMin;
  uint32_t timeErrorMax;
};

cooler coolersStatus[7][6];
coolerData coolersDataArr[7][6];

uint8_t alarmSound = 0;              // Флаг звука тревоги
uint32_t lastScreenUpdate = 0;       // Последнее время обновления экрана
uint32_t screenUpdatePeriod = 60000; // Период обновления экрана
uint32_t alarmBlokingTime = 3600000; // Время отключения сигнала мс

uint8_t barSize = 0; // Показание термометра оживляющего картинку

StaticJsonDocument<300> coolersData;
char json[300];

uint8_t newColor = 0;
uint64_t LastDadaUpdate = 0;

lv_obj_t *btn1[7][6];
uint8_t btn_id[7][6] = {0}; // 7 строк / 5 колонок
lv_obj_t *bar1;



/* Display flushing */
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

#if (LV_COLOR_16_SWAP != 0)
  lcd->draw16bitBeRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#else
  lcd->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#endif

  lv_disp_flush_ready(disp);
}

void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{
  if (touch_has_signal())
  {
    if (touch_touched())
    {
      data->state = LV_INDEV_STATE_PR;

      if (data->point.x > 801) //Ошибка запуска контроллера, нужна перезагрузка
      {
        /*Change the active screen's background color*/
        lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x003a57), LV_PART_MAIN);

        /*Create a white label, set its text and align it to the center*/
        lv_obj_t * label = lv_label_create(lv_scr_act());
        lv_label_set_text(label, "***********   Please RESET controller!   ***********");
        lv_obj_set_style_text_color(lv_scr_act(), lv_color_hex(0xffffff), LV_PART_MAIN);
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

        Serial.println( "" );
        Serial.println( "***********************************************************************" );
        Serial.println( "*********************  Please RESET controller!  **********************" );
        Serial.println( "***********************************************************************" );
        while (1)
        {
        }
      }

      /*Set the coordinates*/
      data->point.x = touch_last_x;
      data->point.y = touch_last_y;
      Serial.print( "Data x " );
      Serial.println( data->point.x );
      Serial.print( "Data y " );
      Serial.println( data->point.y );
    }
    else if (touch_released())
    {
      data->state = LV_INDEV_STATE_REL;
    }
  }
  else
  {
    data->state = LV_INDEV_STATE_REL;
  }
}

void callback1()  //Callback function
{
  // if (bar_flag == 6)
  // {
  //   if (val > 1)
  //   {
  //     val--;
  //     lv_bar_set_value(bar, val, LV_ANIM_OFF);
  //     lv_label_set_text_fmt(ui_Labe2, "%d %%", val);
  //   }
  //   else
  //   {
  //     lv_obj_clear_flag(ui_touch, LV_OBJ_FLAG_CLICKABLE);
  //     lv_label_set_text(ui_Labe2, "Loading");
  //     delay(150);
  //     val = 100;
  //     bar_flag = 0; //Stop progress bar sign
  //     goto_widget_flag = 1; //Enter widget logo

  //   }
  // }
}

//Touch Label control
void label_xy()
{
  // ui_Label = lv_label_create(ui_TOUCH);
  // lv_obj_enable_style_refresh(true);
  // lv_obj_set_width(ui_Label, LV_SIZE_CONTENT);   /// 1
  // lv_obj_set_height(ui_Label, LV_SIZE_CONTENT);    /// 1
  // lv_obj_set_x(ui_Label, -55);
  // lv_obj_set_y(ui_Label, -40);
  // lv_obj_set_align(ui_Label, LV_ALIGN_CENTER);
  // lv_obj_set_style_text_color(ui_Label, lv_color_hex(0xFF0000), LV_PART_MAIN | LV_STATE_DEFAULT);
  // lv_obj_set_style_text_opa(ui_Label, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
  // lv_obj_set_style_text_font(ui_Label, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);

  // ui_Label3 = lv_label_create(ui_TOUCH);
  // lv_obj_enable_style_refresh(true);
  // lv_obj_set_width(ui_Label3, LV_SIZE_CONTENT);   /// 1
  // lv_obj_set_height(ui_Label3, LV_SIZE_CONTENT);    /// 1
  // lv_obj_set_x(ui_Label3, 85);
  // lv_obj_set_y(ui_Label3, -40);
  // lv_obj_set_align(ui_Label3, LV_ALIGN_CENTER);
  // lv_obj_set_style_text_color(ui_Label3, lv_color_hex(0x00FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
  // lv_obj_set_style_text_opa(ui_Label3, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
  // lv_obj_set_style_text_font(ui_Label3, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);

}

#define Z_THRESHOLD 350 // Touch pressure threshold for validating touches
#define _RAWERR 20 // Deadband error allowed in successive position samples
void begin_touch_read_write(void) {
  digitalWrite(38, HIGH); // Just in case it has been left low
  spi.setFrequency(600000);
  digitalWrite(38, LOW);
}

void end_touch_read_write(void) {
  digitalWrite(38, HIGH); // Just in case it has been left low
  spi.setFrequency(600000);

}

uint16_t getTouchRawZ(void) {

  begin_touch_read_write();

  // Z sample request
  int16_t tz = 0xFFF;
  spi.transfer(0xb0);               // Start new Z1 conversion
  tz += spi.transfer16(0xc0) >> 3;  // Read Z1 and start Z2 conversion
  tz -= spi.transfer16(0x00) >> 3;  // Read Z2

  end_touch_read_write();

  return (uint16_t)tz;
}

uint8_t getTouchRaw(uint16_t *x, uint16_t *y) {
  uint16_t tmp;

  begin_touch_read_write();

  // Start YP sample request for x position, read 4 times and keep last sample
  spi.transfer(0xd0);                    // Start new YP conversion
  spi.transfer(0);                       // Read first 8 bits
  spi.transfer(0xd0);                    // Read last 8 bits and start new YP conversion
  spi.transfer(0);                       // Read first 8 bits
  spi.transfer(0xd0);                    // Read last 8 bits and start new YP conversion
  spi.transfer(0);                       // Read first 8 bits
  spi.transfer(0xd0);                    // Read last 8 bits and start new YP conversion

  tmp = spi.transfer(0);                   // Read first 8 bits
  tmp = tmp << 5;
  tmp |= 0x1f & (spi.transfer(0x90) >> 3); // Read last 8 bits and start new XP conversion

  *x = tmp;

  // Start XP sample request for y position, read 4 times and keep last sample
  spi.transfer(0);                       // Read first 8 bits
  spi.transfer(0x90);                    // Read last 8 bits and start new XP conversion
  spi.transfer(0);                       // Read first 8 bits
  spi.transfer(0x90);                    // Read last 8 bits and start new XP conversion
  spi.transfer(0);                       // Read first 8 bits
  spi.transfer(0x90);                    // Read last 8 bits and start new XP conversion

  tmp = spi.transfer(0);                 // Read first 8 bits
  tmp = tmp << 5;
  tmp |= 0x1f & (spi.transfer(0) >> 3);  // Read last 8 bits

  *y = tmp;

  end_touch_read_write();

  return true;
}

uint8_t validTouch(uint16_t *x, uint16_t *y, uint16_t threshold) {
  uint16_t x_tmp, y_tmp, x_tmp2, y_tmp2;

  // Wait until pressure stops increasing to debounce pressure
  uint16_t z1 = 1;
  uint16_t z2 = 0;
  while (z1 > z2)
  {
    z2 = z1;
    z1 = getTouchRawZ();
    delay(1);
    Serial.print("z1:");
    Serial.println(z1);
  }


  if (z1 <= threshold) return false;

  getTouchRaw(&x_tmp, &y_tmp);


  delay(1); // Small delay to the next sample
  if (getTouchRawZ() <= threshold) return false;

  delay(2); // Small delay to the next sample
  getTouchRaw(&x_tmp2, &y_tmp2);


  if (abs(x_tmp - x_tmp2) > _RAWERR) return false;
  if (abs(y_tmp - y_tmp2) > _RAWERR) return false;

  *x = x_tmp;
  *y = y_tmp;

  return true;
}

void calibrateTouch(uint16_t *parameters, uint32_t color_fg, uint32_t color_bg, uint8_t size) {
  int16_t values[] = {0, 0, 0, 0, 0, 0, 0, 0};
  uint16_t x_tmp, y_tmp;
  uint16_t _width = 800;
  uint16_t _height = 480;

  for (uint8_t i = 0; i < 4; i++) {
    lcd->fillRect(0, 0, size + 1, size + 1, color_bg);
    lcd->fillRect(0, _height - size - 1, size + 1, size + 1, color_bg);
    lcd->fillRect(_width - size - 1, 0, size + 1, size + 1, color_bg);
    lcd->fillRect(_width - size - 1, _height - size - 1, size + 1, size + 1, color_bg);

    if (i == 5) break; // used to clear the arrows

    switch (i) {
      case 0: // up left
        lcd->drawLine(0, 0, 0, size, color_fg);
        lcd->drawLine(0, 0, size, 0, color_fg);
        lcd->drawLine(0, 0, size , size, color_fg);
        break;
      case 1: // bot left
        lcd->drawLine(0, _height - size - 1, 0, _height - 1, color_fg);
        lcd->drawLine(0, _height - 1, size, _height - 1, color_fg);
        lcd->drawLine(size, _height - size - 1, 0, _height - 1 , color_fg);
        break;
      case 2: // up right
        lcd->drawLine(_width - size - 1, 0, _width - 1, 0, color_fg);
        lcd->drawLine(_width - size - 1, size, _width - 1, 0, color_fg);
        lcd->drawLine(_width - 1, size, _width - 1, 0, color_fg);
        break;
      case 3: // bot right
        lcd->drawLine(_width - size - 1, _height - size - 1, _width - 1, _height - 1, color_fg);
        lcd->drawLine(_width - 1, _height - 1 - size, _width - 1, _height - 1, color_fg);
        lcd->drawLine(_width - 1 - size, _height - 1, _width - 1, _height - 1, color_fg);
        break;
    }

    // user has to get the chance to release
    if (i > 0) delay(1000);

    for (uint8_t j = 0; j < 8; j++) {
      while (touch_has_signal())
      {
        if (touch_touched())
        {
          Serial.print( "calibrateTouch Data x :" );
          Serial.println( touch_last_x );
          Serial.print( "calibrateTouch Data y :" );
          Serial.println( touch_last_y );
          break;
        }
      }
    }
  }
}

void touch_calibrate()//屏幕校准
{
  uint16_t calData[5];
  uint8_t calDataOK = 0;
  Serial.println("屏幕校准");

  //校准
  //  lcd->fillScreen(BLACK);
  //  lcd->setCursor(20, 0);
  //  Serial.println("setCursor");
  //  lcd->setTextFont(2);
  //  Serial.println("setTextFont");
  //  lcd->setTextSize(1);
  //  Serial.println("setTextSize");
  //  lcd->setTextColor(TFT_WHITE, TFT_BLACK);

  //  lcd->println("按指示触摸角落");
  Serial.println("按指示触摸角落");
  //  lcd->setTextFont(1);
  //  lcd->println();
  //      lcd->setCursor(175, 100);
  //      lcd->printf("Touch Adjust");
  //  Serial.println("setTextFont(1)");
  lv_timer_handler();
  calibrateTouch(calData, MAGENTA, BLACK, 17);
  Serial.println("calibrateTouch(calData, TFT_MAGENTA, TFT_BLACK, 15)");
  Serial.println(); Serial.println();
  Serial.println("//在setup()中使用此校准代码:");
  Serial.print("uint16_t calData[5] = ");
  Serial.print("{ ");

  for (uint8_t i = 0; i < 5; i++)
  {
    Serial.print(calData[i]);
    if (i < 4) Serial.print(", ");
  }

  Serial.println(" };");
  //  Serial.print("  tft.setTouch(calData);");
  //  Serial.println(); Serial.println();
  //  lcd->fillScreen(BLACK);
  //  lcd->println("XZ OK!");
  //  lcd->println("Calibration code sent to Serial port.");


}

void setTouch(uint16_t *parameters) {
  touchCalibration_x0 = parameters[0];
  touchCalibration_x1 = parameters[1];
  touchCalibration_y0 = parameters[2];
  touchCalibration_y1 = parameters[3];

  if (touchCalibration_x0 == 0) touchCalibration_x0 = 1;
  if (touchCalibration_x1 == 0) touchCalibration_x1 = 1;
  if (touchCalibration_y0 == 0) touchCalibration_y0 = 1;
  if (touchCalibration_y1 == 0) touchCalibration_y1 = 1;

  touchCalibration_rotate = parameters[4] & 0x01;
  touchCalibration_invert_x = parameters[4] & 0x02;
  touchCalibration_invert_y = parameters[4] & 0x04;
}

static void event_handler(lv_event_t *e)
{
  lv_event_code_t code = lv_event_get_code(e);

  if (code == LV_EVENT_CLICKED)
  {
    uint8_t btnClicked = *(uint8_t *)lv_event_get_user_data(e);
    // находим нужный нам холодильник-датчик
    uint8_t row = btnClicked / 10;
    uint8_t col = btnClicked % 10;

    if (col != 0)
      return; // Обрабатываем [0] итоговую строку по холодильнику

    if (coolersStatus[row][col].status == 2)
    {
      lv_obj_t *label = lv_obj_get_child(btn1[col][col], 0);
      coolersStatus[row][col].status = 3;
      coolersStatus[row][col].timeErrorStart = millis();
      setStatusColor(row, col, coolersStatus[row][col].status);
      alarmSound = 0; // Отключаем сигнал тревоги
    }
  }
}

void lv_btn_setup(void)
{
  uint8_t btnSizeX = (800 - 9 * 10) / 7;
  uint8_t btnSizeY = (480 - 8 * 10) / 7;
  uint8_t barSizeX = 10;
  uint8_t barSizeY = 10;

  bar1 = lv_bar_create(lv_scr_act());
  lv_obj_set_size(bar1, barSizeX, screenHeight - 2 * barSizeY);
  lv_obj_set_pos(bar1, barSizeX, barSizeY);
  lv_bar_set_value(bar1, 0, LV_ANIM_OFF);

  lv_obj_t *label;

  for (uint8_t col = 0; col < 1; col++)
  {
    for (uint8_t row = 0; row < 7; row++)
    {
      btn1[row][col] = lv_btn_create(lv_scr_act());
      lv_obj_set_size(btn1[row][col], 2 * btnSizeX, btnSizeY);
      lv_obj_add_event_cb(btn1[row][col], event_handler, LV_EVENT_ALL, &(btn_id[row][col]));
      lv_obj_align(btn1[row][col], LV_ALIGN_OUT_LEFT_MID, 3 * barSizeX, barSizeY + row * (btnSizeY + barSizeY));

      label = lv_label_create(btn1[row][col]);
      char name[8];
      sprintf(name, "KAM - %i", row);
      lv_label_set_text(label, name);
      lv_obj_center(label);
      lv_obj_add_style(btn1[row][col], &style_btn_BLUE, 0);
    }
  }

  for (uint8_t col = 1; col < 6; col++)
  {
    for (uint8_t row = 0; row < 7; row++)
    {
      btn1[row][col] = lv_btn_create(lv_scr_act());
      lv_obj_set_size(btn1[row][col], btnSizeX, btnSizeY);

      lv_obj_add_event_cb(btn1[row][col], event_handler, LV_EVENT_ALL, &(btn_id[row][col]));
      lv_obj_align(btn1[row][col], LV_ALIGN_OUT_LEFT_MID, 3 * barSizeX + 2 * btnSizeX + barSizeX + (col - 1) * (btnSizeX + barSizeX), barSizeY + row * (btnSizeY + barSizeY));

      label = lv_label_create(btn1[row][col]);
      lv_obj_center(label);
      lv_label_set_text_fmt(label, "%i", coolersStatus[row][col].status);
    }
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200); // Init Display

  lcd->begin();
  lcd->fillScreen(BLACK);
  lcd->setTextSize(2);
  delay(200);

  pinMode(4, OUTPUT);

  bodyPOST.reserve(250);

  lv_init();
  touch_init();
  screenWidth = lcd->width();
  screenHeight = lcd->height();

  lv_disp_draw_buf_init(&draw_buf, disp_draw_buf, NULL, screenWidth * screenHeight / 10);

  /* Initialize the display */
  lv_disp_drv_init(&disp_drv);
  /* Change the following line to your display resolution */
  disp_drv.hor_res = screenWidth;
  disp_drv.ver_res = screenHeight;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  /* Initialize the (dummy) input device driver */
  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = my_touchpad_read;
  lv_indev_drv_register(&indev_drv);
#ifdef TFT_BL
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
#endif

  lcd->fillScreen(BLACK);

  lv_style_init(&style_btn_BLUE);
  lv_style_set_bg_color(&style_btn_BLUE, lv_color_hex(0x0048ba));

  lv_style_init(&style_btn_GREEN);
  lv_style_set_bg_color(&style_btn_GREEN, lv_color_hex(0x458b00));

  lv_style_init(&style_btn_RED);
  lv_style_set_bg_color(&style_btn_RED, lv_color_hex(0xff2701));

  lv_style_init(&style_btn_YELLOW);
  lv_style_set_bg_color(&style_btn_YELLOW, lv_color_hex(0xffbf00));

  lv_style_init(&style_btn_DARKGREY);
  lv_style_set_bg_color(&style_btn_DARKGREY, lv_color_hex(0x333333));

  lv_obj_add_style(lv_scr_act(), &style_btn_DARKGREY, 0);

  lv_btn_setup();

  SetProjectName();
  GetSerialNumber();

  PreferencesRead();

  for (uint8_t row = 0; row < 7; row++)
  {
    for (uint8_t col = 0; col < 6; col++)
    {
      btn_id[row][col] = row * 10 + col;
      coolersStatus[row][col].status = 0;
      coolersStatus[row][col].tempCur = 0;
      coolersStatus[row][col].tempMin = coolersDataArr[row][col].tempMin;
      coolersStatus[row][col].tempMax = coolersDataArr[row][col].tempMax;
      coolersStatus[row][col].timeErrorMax = coolersDataArr[row][col].timeErrorMax;
      coolersStatus[row][col].timeLastSync = 0;
    }
  }

  sprintf(DebugMessage, "\n**************************************\n");
  DebugMSG(DebugMessage);
  sprintf(DebugMessage, "Main version = %s *\nESP32 Chip ID = %s", MainVersion.c_str(), SerialNumber.c_str());
  DebugMSG(DebugMessage);

  StartWorkServer();

  if (WiFi.status() == WL_CONNECTED)
  {
    /*use mdns for host name resolution*/
    const char *host = "esp32";
    if (!MDNS.begin(host))
    { // http://esp32.local
      Serial.println("Error setting up MDNS responder!");
      while (1)
      {
        delay(1000);
      }
    }

    // Если подключились удачно, запускаем сервер
    sprintf(DebugMessage, "WiFi connected:\n");
    DebugMSG(DebugMessage);

    sprintf(DebugMessage, "AP localIP: %s\n", WiFi.localIP().toString().c_str());
    DebugMSG(DebugMessage);
    sprintf(DebugMessage, "AP gatewayIP: %s\n", WiFi.gatewayIP().toString().c_str());
    DebugMSG(DebugMessage);
    sprintf(DebugMessage, "AP subnetMask: %s\n", WiFi.subnetMask().toString().c_str());
    DebugMSG(DebugMessage);
    sprintf(DebugMessage, "AP DNS: %s\n", WiFi.dnsIP().toString().c_str());
    DebugMSG(DebugMessage);

    SetupWebServerWork();

    delay(5000); // Чтобы успеть на экран посмотреть
  }
  else
  {
    // Не смогли подключиться к WiFi
    // Запускаем свою точку доступа и страницу начальной настройки
    StartSetupServer();
  }

}

void loop() {

  server.handleClient();
  lv_timer_handler();
  barSize += 1;
  if (barSize > 100)
    barSize = 0;
  lv_bar_set_value(bar1, barSize, LV_ANIM_OFF);

  if (millis() - lastScreenUpdate > screenUpdatePeriod)
  {

    alarmSound = 0;
    lastScreenUpdate = millis();
    // Проверяем статусы каждого из контроллеров

    uint32_t currTime = millis();

    // Обрабатываем данные всех датчиков всех контроллеров
    for (uint8_t row = 0; row < 7; row++) // Контроллеры - строки
    {
      uint8_t rowError = 0;

      for (uint8_t col = 1; col < 6; col++) // Датчики - колоники, колонка[0] - как итоговая состояния контроллера (наименование)
      {

        // Если настройка timeErrorMax == 0, датчик не используется
        if (coolersStatus[row][col].timeErrorMax == 0)
        {
          coolersStatus[row][col].status = -1; // -1 == Не используетя
          continue;                            // Перезодим к следующему датчику
        }

        // Проверяем выход температуры за допустимые пределы
        if (coolersStatus[row][col].tempCur > coolersStatus[row][col].tempMax || coolersStatus[row][col].tempCur < coolersStatus[row][col].tempMin)
        {
          // Ошибка по температуре
          // Температура вне нормы, если это начало ошибки - запоминаем время
          if (coolersStatus[row][col].timeErrorStart == 0)
          {
            coolersStatus[row][col].timeErrorStart = currTime;
          }

          // Проверяем допустимое время ошибки по температуре, устанавливаем статус ошибки = 2 ТРЕВОГА
          if ((currTime - coolersStatus[row][col].timeErrorStart) > coolersStatus[row][col].timeErrorMax)
          {
            coolersStatus[row][col].status = 2;
          }
          else
          {
            // Для тревоги еще рано, статус ошибки = 1 ПРЕДУПРЕЖДЕНИЕ
            coolersStatus[row][col].status = 1;
          }
        }
        else
        {
          // Нет ошбики по температуре
          // Температура в норме, обнуляем время ошибки
          coolersStatus[row][col].status = 0;
          coolersStatus[row][col].timeErrorStart = 0;
        }

        // Проверяем отсутствие данных по датчику более допустимого времени
        // проерять статусы вышестоящих ошибок не нужно, если и были ошибки - то время обновилось, мы ничего не испортим, дальше просто не пройдем
        if (currTime - coolersStatus[row][col].timeLastSync > coolersStatus[row][col].timeErrorMax)
        {
          // Данных давно не было
          // Температура вне нормы, если это начало ошибки - запоминаем время
          coolersStatus[row][col].status = 4;
        }
        else
        {
          if (coolersStatus[row][col].status == 4) // Убираем ошибку, только если это ошибка времени
          {
            coolersStatus[row][col].status = 0;
          }
        }

        // Если по текущему датчику есть ошибка, накапливаем её для присвоения итогового значения по контоллеру
        if (coolersStatus[row][col].status == 2 || coolersStatus[row][col].status == 4)
        {
          rowError++;
        }
      }

      if (rowError)
      {
        if (coolersStatus[row][0].status != 3)
          coolersStatus[row][0].status = 2; // Блокировки сигнала ошибки не было, устанавливаем контроллеру статус ошибка
        else
        {
          // Была блокировка тревоги. Проверяем не превышено ли время блокировки
          if (currTime - coolersStatus[row][0].timeErrorStart > alarmBlokingTime)
          {
            coolersStatus[row][0].status = 2; // Превышено, убираем блокировку
          }
        }
      }
      else
      {
        coolersStatus[row][0].status = 0; // Отмена ошибки по контроллеру
      }
    }

    // Обновляем информацию на экране
    for (uint8_t row = 0; row < 7; row++) // Контроллеры - строки
    {
      if (coolersStatus[row][0].status == 2)
        alarmSound++;

      lv_obj_t *label = lv_obj_get_child(btn1[row][0], 0); // Установим наименование контроллера
      lv_label_set_text_fmt(label, coolersStatus[row][0].name);
      setStatusColor(row, 0, coolersStatus[row][0].status);

      for (uint8_t col = 1; col < 6; col++) // Датчики - колоники, колонка[0] - как итоговая состояния контроллера (наименование)
      {
        lv_obj_t *label = lv_obj_get_child(btn1[row][col], 0);

        if (coolersStatus[row][col].timeErrorMax == 0)
        {
          lv_label_set_text_fmt(label, "-");
          continue;
        }

        lv_label_set_text_fmt(label, "%5.1f", coolersStatus[row][col].tempCur);
        setStatusColor(row, col, coolersStatus[row][col].status);
      }
    }
  }

  if (alarmSound)
  {
    digitalWrite(20, !digitalRead(4)); // Пищим
  }
  else
  {
    digitalWrite(20, LOW); // Не пищим
  }

  delay(50);
  
}

//*****************************************************************************
void DebugMSG(char *Msg)
{
  String myString = String(Msg);
  Serial.printf(myString.c_str());
  //tft.println(Msg);
}
//
void ResetMCU(void)
{
  server.send(200, "text/html", "ResetMCU - OK");

  delay(1000);
  ESP.restart();
}
//
void Device_Info(void)
{
  char Buff[100] = {0};
  WebContent = "<!DOCTYPE HTML>\r\n";
  WebContent += "<html>";
  WebContent += "<html lang='ru-RU'><head><meta charset='UTF-8'/>";
  WebContent += "<body>";

  WebContent += "<p>" + ProjectName + "</p>";
  WebContent += "<p> Main module ver: " + MainVersion + "</p>";
  // WebContent += "<p> Main Functions ver: " + MainFunctionsVersion + "</p>";
  // WebContent += "<p> MCU Functions ver: " + MCUFunctionsVersion + "</p>";

  sprintf(Buff, "<p> Serial number: %s </p>", SerialNumber.c_str());
  WebContent += Buff;

  sprintf(Buff, "<p> WiFi MAC: %s </p>", WiFi.macAddress().c_str());
  WebContent += Buff;

  server.send(200, "text/html", WebContent);
}
//

void GetSerialNumber(void)
{
  char Buff[50] = {0};
  uint64_t deviceID = ESP.getEfuseMac(); // The chip ID is essentially its MAC address(length: 6 bytes).
  sprintf(Buff, "Version = %s\nESP32 Chip ID = %04X", MainVersion.c_str(), (uint16_t)(deviceID >> 32));
  SerialNumber = Buff;
}
//
void SetProjectName(void)
{
  ProjectName = "SZ_ESP32_MonitoringALARM";
}
//
void PreferencesWrite(void)
{
  EEPROM.begin(1024); // Инициализируем
  delay(50);

  String SSID = server.arg("SSID");
  String PASS = server.arg("PASS");

  uint8_t UseDHCP = server.arg("UseDHCP").toInt();

  IP[0] = server.arg("IP0").toInt();
  IP[1] = server.arg("IP1").toInt();
  IP[2] = server.arg("IP2").toInt();
  IP[3] = server.arg("IP3").toInt();

  GW[0] = server.arg("GW0").toInt();
  GW[1] = server.arg("GW1").toInt();
  GW[2] = server.arg("GW2").toInt();
  GW[3] = server.arg("GW3").toInt();

  DNS[0] = server.arg("DNS0").toInt();
  DNS[1] = server.arg("DNS1").toInt();
  DNS[2] = server.arg("DNS2").toInt();
  DNS[3] = server.arg("DNS3").toInt();

  MASK[0] = server.arg("MASK0").toInt();
  MASK[1] = server.arg("MASK1").toInt();
  MASK[2] = server.arg("MASK2").toInt();
  MASK[3] = server.arg("MASK3").toInt();

  for (uint8_t i = 0; i < 32; ++i)
  {
    EEPROM.write(adrSSID + i, SSID[i]);
    EEPROM.write(adrPASS + i, PASS[i]);
  }

  for (uint8_t i = 0; i < 4; i++)
  {
    EEPROM.write(adrIP + i, IP[i]);
    EEPROM.write(adrGW + i, GW[i]);
    EEPROM.write(adrDNS + i, DNS[i]);
    EEPROM.write(adrMASK + i, MASK[i]);
  }

  EEPROM.write(adrUseDHCP, UseDHCP);

  EEPROM.commit();
  EEPROM.end();

  delay(1000);
  ResetMCU();
}
//
void PreferencesRead(void)
{
  // Восстанавливаем из памяти EEPROM данные имени сети и пароля
  EEPROM.begin(1024); // Инициализируем
  delay(50);

  for (uint8_t i = 0; i < 32; ++i)
  {
    SSID[i] = char(EEPROM.read(adrSSID + i));
    PASS[i] = char(EEPROM.read(adrPASS + i));
  }

  for (uint8_t i = 0; i < 4; i++)
  {
    IP[i] = EEPROM.read(adrIP + i);
    GW[i] = EEPROM.read(adrGW + i);
    DNS[i] = EEPROM.read(adrDNS + i);
    MASK[i] = EEPROM.read(adrMASK + i);
  }

  EEPROM.get(adrUseDHCP, UseDHCP);

  EEPROM.get(adrScreenUpdatePeriod, screenUpdatePeriod); // Период обновления экрана
  screenUpdatePeriod = screenUpdatePeriod * 1000;
  EEPROM.get(adrAlarmBlokingTime, alarmBlokingTime); // Время отключения сигнала мс
  alarmBlokingTime = alarmBlokingTime * 1000;

  EEPROM.get(512, coolersDataArr);

  EEPROM.end();
}
//

void SetStartUpPreferences(void)
{
  PreferencesWrite();
}
//

//
void send_MainPage(void)
{
  String qsid = server.arg("name");

  IPAddress ip = WiFi.softAPIP();
  String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);

  WebContent = "<!DOCTYPE HTML>\r\n";
  WebContent += "<html>";
  WebContent += "<html lang='ru-RU'><head><meta charset='UTF-8'/>";
  WebContent += "<body>";
  WebContent += "<p>" + ProjectName + " ver: " + MainVersion + "</p>";

  sprintf(webBuffer, "<p># WiFi signal level: %i, MAC Address: %s</p>", WiFi.RSSI(), WiFi.macAddress().c_str());
  WebContent += webBuffer;
  uint32_t workingTime = millis() / 1000;
  sprintf(webBuffer, "<p># Working time: %i sec.</p>", workingTime);
  WebContent += webBuffer;

  WebContent += "<table border='10' cellpadding='10' cellspacing='10'>";
  WebContent += "	<tbody>";

  WebContent += "<tr>";
  WebContent += "<td colspan='2'>";
  WebContent += "<p> <form method='get' action='set_CoolerCodes'>  </p>";
  WebContent += "<p> Set NEW coolers codes /set_CoolerCodes</p>";
  sprintf(webBuffer, "<label> Screen Update Period (sec): </label><input name='screenUpdatePeriod' value=%i size=10 length=10</p>", screenUpdatePeriod / 1000);
  WebContent += webBuffer;
  sprintf(webBuffer, "<label> Alarm Bloking Time (sec): </label><input name='alarmBlokingTime' value=%i size=10 length=10</p>", alarmBlokingTime / 1000);
  WebContent += webBuffer;
  for (uint8_t row = 0; row < 7; row++)
  {
    sprintf(webBuffer, "<p><label>LINE %i : </label>", row);
    WebContent += webBuffer;
    for (uint8_t col = 1; col < 6; col++)
    {
      sprintf(webBuffer, "<p><label>Sensor %i:  </label>", col);
      WebContent += webBuffer;
      sprintf(webBuffer, "<label>  Min: </label><input name='Min%i' value=%i size=5 length=5", row * 10 + col, coolersStatus[row][col].tempMin);
      WebContent += webBuffer;
      sprintf(webBuffer, "<label>  Max: </label><input name='Max%i' value=%i size=5 length=5", row * 10 + col, coolersStatus[row][col].tempMax);
      WebContent += webBuffer;
      sprintf(webBuffer, "<label>  Time (sek): </label><input name='Time%i' value=%i size=5 length=5</p>", row * 10 + col, coolersStatus[row][col].timeErrorMax / 1000);
      WebContent += webBuffer;
      // sprintf(webBuffer, "<label> Switch: </label><input name='SwitchCode%i' value=%i size=5 length=5>", i + 1, coolersStatus[i].tempMax);
      // WebContent += webBuffer;
      // sprintf(webBuffer, "<label>       Alarm: Temp </label><input name='AlarmTemp%i' value=%i size=5 length=5>", i + 1, coolersStatus[i].timeErrorMax);
      // WebContent += webBuffer;
    }
  }
  WebContent += "<p></p>";
  WebContent += "<p> <input type='submit' value='Update'></form> </p>";

  WebContent += "</td>";
  WebContent += "</tr>";

  WebContent += "</tr>";
  // Device Info
  WebContent += "<td>";
  WebContent += "<p></p>";
  WebContent += "<p> <form method='get' action='Device_Info'>  </p>";
  WebContent += "<p><label>TOKEN: </label><input name='TOKEN' size=10 length=10></p>";
  WebContent += "<p><input type='submit' value='Device Info'></form> </p>";
  WebContent += "</td>";
  // Update FirmWare
  WebContent += "<td>";
  WebContent += "<p> <form method='get' action='ResetMCU'>  </p>";
  WebContent += "<p><label>TOKEN: </label><input name='TOKEN' size=10 length=10></p>";
  WebContent += "<p><input type='submit' value='RESET MCU'></form> </p>";
  WebContent += "</td>";
  WebContent += "</tr>";

  WebContent += "</tbody>";
  WebContent += "</table>";

  WebContent += "</body>";
  WebContent += "</html>";

  server.send(200, "text/html", WebContent);
};

//
void onNotFound(void)
{
  Serial.println(server.uri());
  server.send(200, "text/html", "Page not found! Sorry...");
};

//
void SendStartSetupPage(void)
{
  IPAddress ip = WiFi.softAPIP();
  String ipStr = ip.toString();
  WebContent = "<!DOCTYPE HTML>\r\n";
  WebContent += "<html>";
  WebContent += "<body>";
  WebContent += "<p>Monitoring ALARM Screen ver:" + MainVersion + "<p>";
  WebContent += "<form action=\"/scan\" method=\"POST\"><input type=\"submit\" value=\"scan\"></form>";
  WebContent += ipStr;
  WebContent += "<p>";

  WebContent += "</p>";
  WebContent += "<p> <form method='get' action='SetStartUpPreferences'>  </p>";
  WebContent += "<p> <label>SSID:      </label><input name='SSID' length=32> </p>";
  WebContent += "<p> <label>PASS:      </label><input name='PASS' length=32> </p>";
  WebContent += "<p> <label>Use DHCP ( 0 / 1 ):      </label><input name='UseDHCP' length=1> </p>";
  WebContent += "<p> <label>IP adress : </label><input name='IP0' size=3 length=3><input name='IP1' size=3 length=3><input name='IP2' size=3 length=3><input name='IP3' size=3 length=3> </p>";
  WebContent += "<p> <label>IP GateWay: </label><input name='GW0' size=3 length=3><input name='GW1' size=3 length=3><input name='GW2' size=3 length=3><input name='GW3' size=3 length=3> </p>";
  WebContent += "<p> <label>IP DNS    : </label><input name='DNS0' size=3 length=3><input name='DNS1' size=3 length=3><input name='DNS2' size=3 length=3><input name='DNS3' size=3 length=3> </p>";
  WebContent += "<p> <label>SubnetMASK: </label><input name='MASK0' size=3 length=3 value='255'><input name='MASK1' size=3 length=3 value='255'><input name='MASK2' size=3 length=3 value='255'><input name='MASK3' size=3 length=3 value='0'> </p>";
  WebContent += "<p> <input type='submit'></form> </p>";

  WebContent += "</body>";
  WebContent += "</html>";
  server.send(200, "text/html", WebContent);
};

//
void setNewIP(void)
{

  EEPROM.begin(1024); // Инициализируем
  uint8_t UseDHCP = server.arg("UseDHCP").toInt();

  IP[0] = server.arg("IP0").toInt();
  IP[1] = server.arg("IP1").toInt();
  IP[2] = server.arg("IP2").toInt();
  IP[3] = server.arg("IP3").toInt();

  for (uint8_t i = 0; i < 4; i++)
  {
    EEPROM.write(adrIP + i, IP[i]);
  }

  EEPROM.write(adrUseDHCP, UseDHCP);
  EEPROM.commit();
  EEPROM.end();

  delay(1000);
  ResetMCU();
}

//
void SendSetupPage(void)
{
  String qsid = server.arg("name");

  IPAddress ip = WiFi.softAPIP();
  String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);

  WebContent = "<!DOCTYPE HTML>\r\n";
  WebContent += "<html>";
  WebContent += "<html lang='ru-RU'><head><meta charset='UTF-8'/>";
  WebContent += "<body>";
  WebContent += "<p> SETUP: " + ProjectName + " ver: " + MainVersion + "</p>";

  WebContent += "<table align=\"left\" border=\"10\" cellpadding=\"10\" cellspacing=\"10\">";
  WebContent += "<caption>SETUP PAGE</caption>";
  WebContent += "<tbody>";
  WebContent += " <tr>";

  WebContent += "<td>";
  WebContent += "<p> <form method='get' action='setNewPIN'>  </p>"; //
  WebContent += "<p> <label>PIN CODE:    </label><input name='PIN_CODE' length=4> </p>";
  WebContent += "<p> <label>NEW PIN:    </label><input name='NEW_PIN' length=4> </p>";
  WebContent += "<p> <input type='submit' value='Set new PIN'></form> </p>";
  WebContent += "<p></p>";
  WebContent += "</td>";

  WebContent += "<td>";
  WebContent += "<p>";

  WebContent += "</p>";
  WebContent += "<p> <form method='get' action='setNewIP'>  </p>";
  WebContent += "<p> <label>Use DHCP ( 0 / 1 ):      </label><input name='UseDHCP' length=1> </p>";
  WebContent += "<p> <label>IP adress : </label><input name='IP0' size=3 length=3><input name='IP1' size=3 length=3><input name='IP2' size=3 length=3><input name='IP3' size=3 length=3> </p>";
  WebContent += "<p> <input type='submit'></form> </p>";
  WebContent += "</td>";

  WebContent += "	</tr>";
  WebContent += "	<tr>";

  WebContent += "<td>";
  // Update Firmware
  WebContent += "<p> <form method='get' action='update_firmware'>  </p>";
  WebContent += "<p> <label>PIN CODE:    </label><input name='PIN_CODE' length=4> </p>";
  WebContent += "<p> <input type='submit' value='Update Firmware'></form> </p>";
  WebContent += "</td>";

  WebContent += "<td>";
  // Update update_fileSystem
  WebContent += "<p> <form method='get' action='update_fileSystem'>  </p>";
  WebContent += "<p> <label>PIN CODE:    </label><input name='PIN_CODE' length=4> </p>";
  WebContent += "<p> <input type='submit' value='Update file system '></form> </p>";
  WebContent += "</td>";

  WebContent += "	</tr>";
  WebContent += "</tbody>";
  WebContent += "</table>";

  WebContent += "</body>";
  WebContent += "</html>";

  server.send(200, "text/html", WebContent);
};
//

void set_CoolerCodes(void)
{

  coolerData coolersDataArr[7][6];

  for (uint8_t row = 0; row < 7; row++)
  {
    for (uint8_t col = 1; col < 6; col++)
    {

      sprintf(webBuffer, "Min%i", row * 10 + col);
      coolersDataArr[row][col].tempMin = server.arg(webBuffer).toInt();

      sprintf(webBuffer, "Max%i", row * 10 + col);
      coolersDataArr[row][col].tempMax = server.arg(webBuffer).toInt();

      sprintf(webBuffer, "Time%i", row * 10 + col);
      coolersDataArr[row][col].timeErrorMax = server.arg(webBuffer).toInt() * 1000; // Переведем в мксек из секунд
    }
  }

  EEPROM.begin(1024); // Инициализируем
  EEPROM.put(512, coolersDataArr);

  screenUpdatePeriod = server.arg("screenUpdatePeriod").toInt();
  alarmBlokingTime = server.arg("alarmBlokingTime").toInt();
  EEPROM.put(adrScreenUpdatePeriod, screenUpdatePeriod); // Период обновления экрана
  EEPROM.put(adrAlarmBlokingTime, alarmBlokingTime);     // Время отключения сигнала мс
  screenUpdatePeriod *= 1000;
  alarmBlokingTime *= 1000;

  EEPROM.commit();
  EEPROM.end();

  for (uint8_t row = 0; row < 7; row++)
  {
    for (uint8_t col = 1; col < 6; col++)
    {
      coolersStatus[row][col].tempMin = coolersDataArr[row][col].tempMin;
      coolersStatus[row][col].tempMax = coolersDataArr[row][col].tempMax;
      coolersStatus[row][col].timeErrorMax = coolersDataArr[row][col].timeErrorMax;
    }
  }
  server.send(200, "text/html", "set_CoolerCodes - OK");
}

//
void StartSetupServer()
{
  char SSID[24] = {0};
  char PASS[12] = {0};
  sprintf(SSID, "ALARM_Setup_WiFi");
  sprintf(PASS, "123456789");

  sprintf(DebugMessage, "\n**************************************\n");
  DebugMSG(DebugMessage);
  sprintf(DebugMessage, "Starting Soft AP for setup server.\n");
  DebugMSG(DebugMessage);
  sprintf(DebugMessage, "**************************************\n");
  DebugMSG(DebugMessage);

  WiFi.disconnect();
  IPAddress local_ip(192, 168, 4, 1);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);

  WiFi.mode(WIFI_STA);
  if (!WiFi.softAP(SSID, PASS))
  {
    sprintf(DebugMessage, "Soft AP creation failed.\n");
    DebugMSG(DebugMessage);
    while (1)
      ;
  }

  // WiFi.softAPConfig(local_ip, gateway, subnet);

  sprintf(DebugMessage, "\nSetup Server AP local IP: %s\n", WiFi.softAPIP().toString().c_str());
  DebugMSG(DebugMessage);

  server.on("/", SendStartSetupPage);
  server.on("/SetStartUpPreferences", SetStartUpPreferences);
  server.onNotFound(SendStartSetupPage);
  server.begin();

  sprintf(DebugMessage, "Server started!\n");
  DebugMSG(DebugMessage);
}

//
void StartWorkServer()
{
  // Отключаем адаптер от сети, если он был подключен
  WiFi.disconnect();

  sprintf(DebugMessage, "\n**************************************\n");
  DebugMSG(DebugMessage);

  WiFi.mode(WIFI_STA);

  WiFi.begin(SSID, PASS);

  sprintf(DebugMessage, "\nNetwork settings:\n");
  DebugMSG(DebugMessage);
  if (!UseDHCP)
  {
    sprintf(DebugMessage, "IP  %3i.%3i.%3i.%3i\n", IP[0], IP[1], IP[2], IP[3]);
    DebugMSG(DebugMessage);
    sprintf(DebugMessage, "GW  %3i.%3i.%3i.%3i\n", GW[0], GW[1], GW[2], GW[3]);
    DebugMSG(DebugMessage);
    sprintf(DebugMessage, "DNS %3i.%3i.%3i.%3i\n", DNS[0], DNS[1], DNS[2], DNS[3]);
    DebugMSG(DebugMessage);
    sprintf(DebugMessage, "SubMASK %3i.%3i.%3i.%3i\n", MASK[0], MASK[1], MASK[2], MASK[3]);
    DebugMSG(DebugMessage);
    sprintf(DebugMessage, "SSID: %s\n", SSID);
    DebugMSG(DebugMessage);
    // sprintf(DebugMessage, "PASS: %s\n\n", PASS);
    // DebugMSG(DebugMessage);

    IPAddress staticIP(IP[0], IP[1], IP[2], IP[3]);
    IPAddress gateway(GW[0], GW[1], GW[2], GW[3]);
    IPAddress subnet(MASK[0], MASK[1], MASK[2], MASK[3]);
    IPAddress dns1(DNS[0], DNS[1], DNS[2], DNS[3]);
    IPAddress dns2(8, 8, 8, 8);
    WiFi.config(staticIP, gateway, subnet, dns1, dns2);
  }
  else
  {
    sprintf(DebugMessage, "\nUsing DHCP mode:\n");
    DebugMSG(DebugMessage);
  }

  int conAttempt = 0; // Количество неудачных попыток запуска сервера

  while ((WiFi.status() != WL_CONNECTED) && (conAttempt < 10)) // 10 секунд попытка подключиться к сети
  {
    delay(1000);
    sprintf(DebugMessage, ".");
    DebugMSG(DebugMessage);
    conAttempt++;
  }
}

//
void SetupWebServerWork()
{
  server.on("/", send_MainPage);

  server.on("/setButton", Device_Info);

  server.on("/ResetMCU", ResetMCU);

  server.on("/setup", SendSetupPage);

  server.on("/setNewIP", setNewIP);

  server.on("/set_CoolerCodes", set_CoolerCodes);

  server.on("/monitoringData", HTTP_POST, monitoringData);

  server.onNotFound(onNotFound);

  

  server.on("/firmwareUpdate", HTTP_GET, []()
            {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", firmwareUpdate); });

  /*handling uploading firmware file */
  server.on(
      "/update", HTTP_POST,

      []()
      {
        server.sendHeader("Connection", "close");
        server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
        ESP.restart();
      },

      []()
      {
        HTTPUpload &upload = server.upload();
        if (upload.status == UPLOAD_FILE_START)
        {
          Serial.printf("Update: %s\n", upload.filename.c_str());
          if (!Update.begin(UPDATE_SIZE_UNKNOWN))
          { // start with max available size
            Update.printError(Serial);
          }
        }
        else if (upload.status == UPLOAD_FILE_WRITE)
        {
          /* flashing firmware to ESP*/
          if (Update.write(upload.buf, upload.currentSize) != upload.currentSize)
          {
            Update.printError(Serial);
          }
        }
        else if (upload.status == UPLOAD_FILE_END)
        {
          if (Update.end(true))
          { // true to set the size to the current progress
            Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
          }
          else
          {
            Update.printError(Serial);
          }
        }
      });

  server.begin();
  sprintf(DebugMessage, "HTTP server started\n");
  DebugMSG(DebugMessage);
}

//
void monitoringData(void)
{
  if (server.hasArg("plain") == false)
  {
    Serial.println("server.hasArg(plain) == false");
  }
  bodyPOST = server.arg("plain");

  uint32_t mks = micros();

  for (uint16_t i = 0; i < bodyPOST.length(); ++i)
  {
    if (i > 298)
    {
      break; // Переполнение массива
    }
    json[i] = bodyPOST[i];
  }
  json[bodyPOST.length()] = 0;

  // Deserialize the JSON document
  DeserializationError error = deserializeJson(coolersData, json);
  Serial.println(bodyPOST);

  // Test if parsing succeeds.
  if (error)
  {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }

  uint8_t row = coolersData["Line"];

  const char *Name = coolersData["Name"];
  for (uint8_t i = 0; i < 10; i++)
  {
    coolersStatus[row][0].name[i] = Name[i];
  }

  uint32_t currTime = millis();
  coolersStatus[row][0].timeLastSync = currTime;

  float Data1 = (float)coolersData["Data1"];
  uint8_t ID1 = (uint8_t)coolersData["ID1"];
  coolersStatus[row][ID1].tempCur = (float)coolersData["Data1"];
  coolersStatus[row][ID1].timeLastSync = currTime;

  float Data2 = (float)coolersData["Data2"];
  uint8_t ID2 = (uint8_t)coolersData["ID2"];
  coolersStatus[row][ID2].tempCur = (float)coolersData["Data2"];
  coolersStatus[row][ID2].timeLastSync = currTime;

  float Data3 = (float)coolersData["Data3"];
  uint8_t ID3 = (uint8_t)coolersData["ID3"];
  coolersStatus[row][ID3].tempCur = (float)coolersData["Data3"];
  coolersStatus[row][ID3].timeLastSync = currTime;

  float Data4 = (float)coolersData["Data4"];
  uint8_t ID4 = (uint8_t)coolersData["ID4"];
  coolersStatus[row][ID4].tempCur = (float)coolersData["Data4"];
  coolersStatus[row][ID4].timeLastSync = currTime;

  float Data5 = (float)coolersData["Data5"];
  uint8_t ID5 = (uint8_t)coolersData["ID5"];
  coolersStatus[row][ID5].tempCur = (float)coolersData["Data5"];
  coolersStatus[row][ID5].timeLastSync = currTime;

  server.send(200, "text/html", "monitoringData - OK");
}

//
void setStatusColor(uint8_t row, uint8_t col, uint8_t status)
{

  //Serial.printf("setStatusColor row = %i col = %i status = %i\n", row, col, status);

  lv_obj_remove_style(btn1[row][col], &style_btn_GREEN, 0);
  lv_obj_remove_style(btn1[row][col], &style_btn_YELLOW, 0);
  lv_obj_remove_style(btn1[row][col], &style_btn_RED, 0);
  lv_obj_remove_style(btn1[row][col], &style_btn_BLUE, 0);
  lv_obj_remove_style(btn1[row][col], &style_btn_DARKGREY, 0);

  switch (coolersStatus[row][col].status)
  {
  case -1: // Не обслуживается
    lv_obj_add_style(btn1[row][col], &style_btn_DARKGREY, 0);
    break;

  case 0: // Норма
    lv_obj_add_style(btn1[row][col], &style_btn_GREEN, 0);
    break;

  case 1: // Предупреждение
    lv_obj_add_style(btn1[row][col], &style_btn_YELLOW, 0);
    break;

  case 2: // ТРЕВОГО!!!
    lv_obj_add_style(btn1[row][col], &style_btn_RED, 0);
    break;

  case 3: // Отключенная тревога
    lv_obj_add_style(btn1[row][col], &style_btn_BLUE, 0);
    break;

  case 4: // Нет данных более допустимого времени
    lv_obj_add_style(btn1[row][col], &style_btn_DARKGREY, 0);
    break;

  default:
    break;
  }
}
