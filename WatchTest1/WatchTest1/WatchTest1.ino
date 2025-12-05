 #include <lvgl.h>
#include <TFT_eSPI.h>
#include "ui.h"

/*
 * This sketch wedges LVGL 8 with the SquareLine Studio UI generated in this folder.
 * Configure your board's display/touch settings via TFT_eSPI's User_Setup.h so that
 * the 1.46\" ESP32-S3 watch can use the ST7789 driver and touch controller it ships with.
 */

TFT_eSPI tft = TFT_eSPI();

static lv_disp_draw_buf_t draw_buf;
static lv_color_t draw_buf_data[LV_HOR_RES_MAX * 40];
static uint32_t last_tick;

static void my_disp_flush(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p)
{
    uint32_t width = area->x2 - area->x1 + 1;
    uint32_t height = area->y2 - area->y1 + 1;
    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, width, height);
    tft.pushColors((uint16_t *)color_p, width * height, true);
    tft.endWrite();
    lv_disp_flush_ready(drv);
}

static bool my_touchpad_read(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    uint16_t touch_x, touch_y, touch_z;
    const bool touched = tft.getTouch(&touch_x, &touch_y, &touch_z);

    if (!touched) {
        data->state = LV_INDEV_STATE_REL;
        return false;
    }

    data->state = LV_INDEV_STATE_PR;
    lv_disp_t *disp = lv_indev_get_disp(drv);
    const uint16_t hor_res = lv_disp_get_hor_res(disp);
    const uint16_t ver_res = lv_disp_get_ver_res(disp);
    data->point.x = lv_map(touch_x, 0, tft.width() - 1, 0, hor_res - 1);
    data->point.y = lv_map(touch_y, 0, tft.height() - 1, 0, ver_res - 1);
    return false;
}

void setup()
{
    lv_init();

    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);

    lv_disp_draw_buf_init(&draw_buf, draw_buf_data, NULL, LV_HOR_RES_MAX * 40);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = tft.width();
    disp_drv.ver_res = tft.height();
    disp_drv.draw_buf = &draw_buf;
    disp_drv.flush_cb = my_disp_flush;
    lv_disp_drv_register(&disp_drv);

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    lv_indev_drv_register(&indev_drv);

    ui_init();

    last_tick = millis();
}

void loop()
{
    const uint32_t now = millis();
    lv_tick_inc(now - last_tick);
    last_tick = now;
    lv_timer_handler();
    delay(5);
}
