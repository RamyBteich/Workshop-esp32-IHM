# m5StackTest1.ino expliqué  
## Aperçu
Ce sketch associe un **M5Core2** à **LVGL** en utilisant l’interface générée par SquareLine (`ui.h`).  
Il définit les callbacks d’affichage et de tactile, allume le moteur LVGL et fait tourner l’UI dans la boucle Arduino.

## 
```cpp
#include <M5Core2.h>
#include <lvgl.h>
#include "ui.h"
```

* `<M5Core2.h>` donne accès à l’écran, au tactile et aux autres périphériques du M5Core2.
* `<lvgl.h>` expose l’API centrale de LVGL (widgets, dessins, timers, etc.).
* `"ui.h"` contient la fonction `ui_init()` générée par SquareLine Studio.

## Buffer de dessin LVGL
```cpp
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[320];
```

* LVGL remplit `buf` avec une ligne complète (320 pixels) pour limiter la RAM utilisée.
* `draw_buf` sert de structure d’échange entre LVGL et l’affichage.

## Lecture du tactile
```cpp
static void touch_read(lv_indev_drv_t *drv, lv_indev_data_t *data);
```

* LVGL appelle ce callback pour connaître l’état du tactile.
* `M5.Touch.update()` rafraîchit la lecture hardware.
* `pressed` est vrai si un point tactile est détecté.
* En cas de pression, `data->point` reçoit les coordonnées du premier contact.
* LVGL utilise ces infos pour générer les événements pointeur.

## Rendu vers l’écran
```cpp
static void display_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p);
```

* Cette fonction est invoquée quand LVGL a rendu une zone (`area`) dans le buffer.
* On calcule largeur/hauteur, on ouvre une transaction vers l’écran et on pousse les pixels via `M5.Lcd.pushColors`.
* `lv_disp_flush_ready(disp)` permet à LVGL de réutiliser les pixels.

## setup()
1. `M5.begin()` initialise la carte et ses périphériques.
2. `M5.Lcd.setRotation(1)` aligne l’écran avec le repère de LVGL.
3. `lv_init()` démarre le moteur LVGL.
4. `lv_disp_draw_buf_init(&draw_buf, buf, NULL, 320)` associe le buffer d’une ligne au driver LVGL.

### Driver d’affichage
```cpp
static lv_disp_drv_t disp_drv;
lv_disp_drv_init(&disp_drv);
disp_drv.draw_buf = &draw_buf;
disp_drv.flush_cb = display_flush;
disp_drv.hor_res = 320;
disp_drv.ver_res = 240;
lv_disp_drv_register(&disp_drv);
```

* Définit la résolution et le callback de rendu.

### Driver tactile
```cpp
static lv_indev_drv_t indev_drv;
lv_indev_drv_init(&indev_drv);
indev_drv.type = LV_INDEV_TYPE_POINTER;
indev_drv.read_cb = touch_read;
lv_indev_drv_register(&indev_drv);
```

* Enregistre `touch_read()` comme source d’événements pointeur pour LVGL.

### Interface SquareLine
`ui_init();` construit les écrans, widgets et la logique générés par SquareLine Studio.

## loop()
```cpp
void loop() {
    M5.update();
    lv_tick_inc(5);
    lv_timer_handler();
    delay(5);
}
```

* `M5.update()` actualise les capteurs et l’écran tactile.
* `lv_tick_inc(5)` avance l’horloge interne de LVGL de 5 ms.
* `lv_timer_handler()` exécute les animations, événements et redessins.
* `delay(5)` empêche la boucle de tourner sans pause et reste cohérent avec les ticks.

## Résumé

- LVGL est alimenté par un buffer d’une ligne pour limiter la mémoire.
- `display_flush` et `touch_read` relient LVGL aux pilotes matériels du M5Core2.
- `setup()` initialise LVGL, les drivers et l’UI SquareLine.
- `loop()` tient à jour le tactile, les ticks et le gestionnaire LVGL.