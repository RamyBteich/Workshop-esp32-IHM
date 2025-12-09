// Change this line "#include "/home/../../WorkShop/m5StackTest1" 
// in ui .h to "#include <lvgl.h>" (line 13)


#include <M5Core2.h>
#include <Ticker.h> // Minuteur périodique pour les ticks LVGL
#include <lvgl.h>
#include "ui.h"     // Déclarations d'interface générées par SquareLine

static const uint32_t LVGL_TICK_RATE_MS = 5;                         // Intervalle de tick attendu par LVGL
static const uint32_t LVGL_BUF_HEIGHT = 40;                          // Hauteur du buffer de dessin intermédiaire
static const uint32_t LVGL_BUFFER_PIXELS = 320 * LVGL_BUF_HEIGHT;    // Pixels mis en mémoire pour les buffers intermédiaires
static Ticker lv_tick_timer;                                         // Minuteur qui incrémente les ticks LVGL
static lv_disp_draw_buf_t draw_buf;                                  // Descripteur du buffer de dessin transmis à LVGL
static lv_color_t draw_buf_data[LVGL_BUFFER_PIXELS];                 // Mémoire buffer
static lv_disp_drv_t disp_drv;
static lv_indev_drv_t touch_drv;
static lv_indev_t * touch_indev = nullptr;                           // Gestionnaire du périphérique d'entrée LVGL
y
static void touch_read(lv_indev_drv_t * drv, lv_indev_data_t * data) // Rappel LVGL Touch_Screen
{
    (void)drv; 
    M5.Touch.update();                                                          // actualise les lectures du tactile
    const bool pressed = M5.Touch.points > 0;                                   // détermine si le tactile est actif
    data->state = pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;   // rapporte l'état à LVGL
    if(pressed) {
        lv_point_t point;                                                       // convertit en point LVGL
        point.x = M5.Touch.point[0].x;
        point.y = M5.Touch.point[0].y;
        data->point = point;
    }
    data->continue_reading = false;                                             // indique une lecture unique
}

static void lv_tick_task() // Tick périodique utilisé par LVGL pour le timing
{
    lv_tick_inc(LVGL_TICK_RATE_MS);
}

static void display_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_map) // Rappel d'effacement LVGL
{
    const uint32_t width = (uint32_t)area->x2 - area->x1 + 1;                           // largeur de la zone mise à jour
    const uint32_t height = (uint32_t)area->y2 - area->y1 + 1;                          // hauteur de la zone mise à jour

    M5.Lcd.startWrite();                                                                // commence la transaction DMA TFT
    M5.Lcd.setAddrWindow(area->x1, area->y1, width, height);                            // définit le rectangle de destination
    M5.Lcd.pushColors(reinterpret_cast<uint16_t *>(color_map), width * height, true);   // envoie les pixels
    M5.Lcd.endWrite();                                                                  // termine la transaction

    lv_disp_flush_ready(disp_drv);                                                      // informe LVGL que l'effacement est terminé
}

void setup()
{
    M5.begin();
    M5.Lcd.setRotation(1);  // oriente l'écran pour le système de coordonnées LVGL

    lv_init();              // initialise les structures centrales de LVGL

    const uint32_t hor_res = M5.Lcd.width(); // récupère la largeur de l'écran
    const uint32_t ver_res = M5.Lcd.height(); // récupère la hauteur de l'écran

    lv_disp_draw_buf_init(&draw_buf, draw_buf_data, nullptr, LVGL_BUFFER_PIXELS); // prépare le buffer de dessin pour LVGL

    lv_disp_drv_init(&disp_drv);
    disp_drv.draw_buf = &draw_buf;
    disp_drv.flush_cb = display_flush;
    disp_drv.hor_res = hor_res;
    disp_drv.ver_res = ver_res;
    lv_disp_t * disp = lv_disp_drv_register(&disp_drv); // crée un objet d'écran LVGL

    lv_indev_drv_init(&touch_drv);
    touch_drv.type = LV_INDEV_TYPE_POINTER;          // configure le mode pointeur (tactile)
    touch_drv.read_cb = touch_read;                  // enregistre notre lecteur tactile (lire l’état du tactile.)
    touch_drv.disp = disp;                           // lie l'entrée à l'écran
    touch_indev = lv_indev_drv_register(&touch_drv); //  on enregistre le driver d’entrée auprès de LVGL.

    lv_tick_timer.attach_ms(LVGL_TICK_RATE_MS, lv_tick_task); // alimente le compteur de ticks LVGL

    ui_init(); // construit l'interface SquareLine qui est défini dans <ui.h>
}

void loop() // Boucle principale Arduino : gère LVGL et le tactile
{
    M5.update();              // actualise l'état du tactile / des boutons
    lv_timer_handler();       // exécute le gestionnaire LVGL (animations, événements)
    delay(LVGL_TICK_RATE_MS); // temporisation simple pour limiter la charge CPU
}
