#include <Arduino.h>
#include "AppInit.h"
#include "LVGL_Driver.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

unsigned long lastUpdateTime = 0;

void setup() {
  Serial.begin(115200);
  delay(100);
  AppInit();
}

void loop() {
  AppLoop();
  Lvgl_Loop();
  delay(5);

  vTaskDelay(pdMS_TO_TICKS(5));

}
