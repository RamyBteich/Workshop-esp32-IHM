#include "AppInit.h"

#include "I2C_Driver.h"
#include "Audio_PCM5101.h"
#include "Display_SPD2010.h"
#include "Gyro_QMI8658.h"
#include "LVGL_Driver.h"
#include "RTC_PCF85063.h"
#include "SD_Card.h"
#include "Touch_SPD2010.h"
#include "TCA9554PWR.h"
#include "ui.h"

void AppInit(void) {
  I2C_Init();
  Backlight_Init();
  Set_Backlight(60);
  TCA9554PWR_Init();
  LCD_Init();
  Lvgl_Init();
  SD_Init();
  Audio_Init();
  PCF85063_Init();
  QMI8658_Init();
  ui_init();
}
