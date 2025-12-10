// Change this line "#include "/home/../../WorkShop/m5StackTest1"
// in ui.h to "#include <lvgl.h>" (line 13)

#include <M5Core2.h>                // Bibliothèque principale du M5Core2
#include <lvgl.h>                   // Bibliothèque graphique LVGL
#include "ui.h"                     // Interface générée par SquareLine

static lv_disp_draw_buf_t draw_buf; // LVGL a besoin d’un buffer pour dessiner les pixels avant l’affichage.
static lv_color_t buf[320];         // Mémoire du buffer (1 ligne de 320 pixels)

static void touch_read(lv_indev_drv_t *drv, lv_indev_data_t *data) {    // Fonction appelée par LVGL pour connaître l’état du tactile

    M5.Touch.update();           
    bool pressed = M5.Touch.points > 0; // Vrai si un doigt touche l’écran

 
    data->state = pressed ? LV_INDEV_STATE_PRESSED
                          : LV_INDEV_STATE_RELEASED;

    if (pressed) {
        data->point.x = M5.Touch.point[0].x;
        data->point.y = M5.Touch.point[0].y;
    }
}

static void display_flush(lv_disp_drv_t * disp, const lv_area_t * area, lv_color_t *color_p) {  // LVGL appelle cette fonction pour dessiner une zone de l’écran.
    uint32_t w = area->x2 - area->x1 + 1; // Largeur de la zone à dessiner
    uint32_t h = area->y2 - area->y1 + 1; // Hauteur de la zone à dessiner

    M5.Lcd.startWrite();                                 
    M5.Lcd.setAddrWindow(area->x1, area->y1, w, h);      
    M5.Lcd.pushColors((uint16_t*)color_p, w * h, true);  
    M5.Lcd.endWrite();                                   

    lv_disp_flush_ready(disp);  // Informe LVGL que l’affichage est terminé
}


void setup() {
    M5.begin();                
    M5.Lcd.setRotation(1);      // Orientation  
    lv_init();                  // Initialise le moteur LVGL

    lv_disp_draw_buf_init(&draw_buf, buf, NULL, 320);  // Initialise le buffer LVGL

    // Driver d’affichage LVGL
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.draw_buf = &draw_buf;     // Associe notre buffer à LVGL
    disp_drv.flush_cb = display_flush; // Callback Fonction de rendu graphique
    disp_drv.hor_res = 320;            // Résolution horizontale
    disp_drv.ver_res = 240;            // Résolution verticale
    lv_disp_drv_register(&disp_drv);   

    // Driver du tactile LVGL
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER; // Type pointeur = écran tactile
    indev_drv.read_cb = touch_read;         // Callback pour lire l’état du tactile
    lv_indev_drv_register(&indev_drv);      

    ui_init();    // Initialisation de l’UI SquareLine

}

void loop() {
    M5.update();        
    lv_tick_inc(5);     // Signale à LVGL que 5ms se sont écoulées
    lv_timer_handler(); // Exécute les animations, événements, etc.
    delay(5);           
}
