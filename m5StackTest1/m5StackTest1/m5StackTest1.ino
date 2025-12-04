// Change this line "#include "/home/ramy/Desktop/WorkShop/m5StackTest1" 
// in ui .h to "#include <lvgl.h>" (line 13)


#include <M5Core2.h> 
#include <Ticker.h> // Minuteur périodique pour les ticks LVGL
#include <lvgl.h> 
#include "ui.h" // Déclarations d'interface générées par SquareLine

static const uint32_t LVGL_TICK_RATE_MS = 5; // Intervalle de tick attendu par LVGL
static const uint32_t LVGL_BUF_HEIGHT = 40; // Hauteur du buffer de dessin intermédiaire
static Ticker lv_tick_timer; // Minuteur qui incrémente les ticks LVGL
static lv_draw_buf_t draw_buf; // Descripteur du buffer de dessin transmis à LVGL
static uint8_t draw_buf_data[LV_DRAW_BUF_SIZE(320, LVGL_BUF_HEIGHT, LV_COLOR_FORMAT_RGB565)]; // Mémoire buffer

static lv_indev_t * touch_indev = nullptr; // Gestionnaire du périphérique d'entrée LVGL

static void touch_read(lv_indev_t * indev, lv_indev_data_t * data) // Rappel LVGL Touch_Screen
{
    (void)indev; 
    M5.Touch.update(); // actualise les lectures du tactile
    const bool pressed = M5.Touch.points > 0; // détermine si le tactile est actif
    data->state = pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED; // rapporte l'état à LVGL
    if(pressed) {
        lv_point_t point; // convertit en point LVGL
        point.x = M5.Touch.point[0].x;
        point.y = M5.Touch.point[0].y;
        data->point = point;
    }
    data->continue_reading = false; // indique une lecture unique
}

static void lv_tick_task() // Tick périodique utilisé par LVGL pour le timing
{
    lv_tick_inc(LVGL_TICK_RATE_MS);
}

static void display_flush(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map) // Rappel d'effacement LVGL
{
    const uint32_t width = (uint32_t)area->x2 - area->x1 + 1; // largeur de la zone mise à jour
    const uint32_t height = (uint32_t)area->y2 - area->y1 + 1; // hauteur de la zone mise à jour

    M5.Lcd.startWrite(); // commence la transaction DMA TFT
    M5.Lcd.setAddrWindow(area->x1, area->y1, width, height); // définit le rectangle de destination
    M5.Lcd.pushColors(reinterpret_cast<uint16_t *>(px_map), width * height, true); // envoie les pixels
    M5.Lcd.endWrite(); // termine la transaction

    lv_display_flush_ready(disp); // informe LVGL que l'effacement est terminé
}

void setup() // Phase d'initialisation Arduino : configure l'écran et LVGL
{
    M5.begin(); // initialise la pile matérielle M5Core2
    M5.Lcd.setRotation(1); // oriente l'écran pour le système de coordonnées LVGL

    lv_init(); // initialise les structures centrales de LVGL

    const uint32_t hor_res = M5.Lcd.width(); // récupère la largeur de l'écran
    const uint32_t ver_res = M5.Lcd.height(); // récupère la hauteur de l'écran

    lv_draw_buf_init(&draw_buf, hor_res, LVGL_BUF_HEIGHT, LV_COLOR_FORMAT_RGB565, 0, draw_buf_data,
                     sizeof(draw_buf_data)); // prépare le tampon de dessin pour LVGL

    lv_display_t * disp = lv_display_create(hor_res, ver_res); // crée un objet d'écran LVGL
    lv_display_set_draw_buffers(disp, &draw_buf, nullptr); // attache le tampon de dessin
    lv_display_set_flush_cb(disp, display_flush); // définit la routine d'effacement
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565); // correspond au format couleur du TFT
    lv_display_set_default(disp); // définit cet écran comme cible par défaut

    touch_indev = lv_indev_create(); // instancie un périphérique d'entrée LVGL
    lv_indev_set_type(touch_indev, LV_INDEV_TYPE_POINTER); // configure le mode pointeur (tactile)
    lv_indev_set_read_cb(touch_indev, touch_read); // enregistre notre lecteur tactile
    lv_indev_set_display(touch_indev, disp); // lie l'entrée à l'écran

    lv_tick_timer.attach_ms(LVGL_TICK_RATE_MS, lv_tick_task); // alimente le compteur de ticks LVGL

    ui_init(); // construit l'interface SquareLine
}

void loop() // Boucle principale Arduino : gère LVGL et le tactile
{
    M5.update(); // actualise l'état du tactile / des boutons
    lv_timer_handler(); // exécute le gestionnaire LVGL (animations, événements)
    delay(LVGL_TICK_RATE_MS); // temporisation simple pour limiter la charge CPU
}
