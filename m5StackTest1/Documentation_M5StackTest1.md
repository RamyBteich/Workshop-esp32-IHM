# m5StackTest1.ino Explained  
### Description of the LVGL + M5Core2 integration in this sketch

## Overview
`m5StackTest1.ino` wires a **M5Core2** board to **LVGL** using a minimal SquareLine UI (`ui.h`).  
It handles the display driver, the touch driver, and the essential LVGL tick/loop work so the generated UI can run on the device.

## Includes & dependencies
```cpp
#include <M5Core2.h>
#include <lvgl.h>
#include "ui.h"
```

* `<M5Core2.h>` gives access to the LCD, touch controller, and general board helpers.
* `<lvgl.h>` is LVGL’s core API for widgets, drawing, timers, etc.
* `"ui.h"` is the SquareLine-generated UI that will be built and loaded during `setup()`.

## LVGL draw buffer
```cpp
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[320];
```

* The sketch uses a single draw buffer that holds **one horizontal line** (320 pixels), which keeps RAM usage low.
* LVGL writes pixels into `buf` and the driver later flushes those pixels to the display.

## Touch reader
```cpp
static void touch_read(...)
```

* Called by LVGL whenever it needs fresh input data.
* `M5.Touch.update()` reads the touchscreen hardware.
* `pressed` reflects whether at least one touch point is detected.
* When pressed, the LVGL point structure is filled with the first touch’s `x`/`y`.
* LVGL uses `data->state` and `data->point` to drive pointer events.

## Display flush callback
```cpp
static void display_flush(...)
```

* LVGL requests this callback every time it has rendered a rectangular area (`area`) into the draw buffer.
* The code calculates width/height, opens a DMA-friendly transaction, and sends the pixels via `M5.Lcd.pushColors`.
* After the LCD write completes, `lv_disp_flush_ready` tells LVGL it can reuse the buffer.

## setup()
1. `M5.begin()` boots the board and peripherals.
2. `M5.Lcd.setRotation(1)` aligns the TFT with LVGL’s coordinate system.
3. `lv_init()` prepares LVGL’s internal structures.
4. `lv_disp_draw_buf_init(&draw_buf, buf, NULL, 320)` registers the single-line buffer with LVGL.

### Display driver initialization
```cpp
static lv_disp_drv_t disp_drv;
lv_disp_drv_init(&disp_drv);
disp_drv.draw_buf = &draw_buf;
disp_drv.flush_cb = display_flush;
disp_drv.hor_res = 320;
disp_drv.ver_res = 240;
lv_disp_drv_register(&disp_drv);
```

* Sets LVGL’s resolution and connects the flush callback that writes to the M5 LCD.

### Touch driver initialization
```cpp
static lv_indev_drv_t indev_drv;
...
lv_indev_drv_register(&indev_drv);
```

* Declares a pointer-type input device and registers `touch_read` as its reader.

### UI initialization
`ui_init();` calls the SquareLine-generated builder to create screens, widgets, and event handlers.

## loop()
```cpp
void loop() {
    M5.update();
    lv_tick_inc(5);
    lv_timer_handler();
    delay(5);
}
```

* `M5.update()` refreshes touch/button states.
* `lv_tick_inc(5)` advances LVGL’s internal clock at 5 ms intervals; it keeps animations and timers in sync.
* `lv_timer_handler()` processes LVGL tasks (animations, events, redrawing).
* `delay(5)` prevents the loop from spinning too fast and loosely matches the tick rate.

## Summary

- The sketch has a tight LVGL integration with a single-line draw buffer for RAM efficiency.
- Touch and display callbacks map LVGL’s requests to the M5Core2 hardware using the `M5` API.
- `setup()` builds LVGL’s drivers and launches the SquareLine UI.
- `loop()` keeps LVGL alive by updating the touch state, ticking the library, and letting the handler run.
