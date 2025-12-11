// Change this line "#include "/home/ramy/Desktop/WorkShop/m5StackTest1" 
// in ui.h to "#include <lvgl.h>" (line 13)

#include <M5Core2.h>     
#include <lvgl.h>        
#include "ui.h"          
#include <WiFi.h>         // WiFi ESP32
#include <PubSubClient.h> // Client MQTT
#include <cctype>
#include <cstring>
#include <cstdio>
#include <cstdlib>

#define WIFI_SSID        "labo-stagiaire"
#define WIFI_PASSWORD    "Uk0IHXsaTt4u"
#define MQTT_BROKER_HOST "192.168.7.13"
#define MQTT_BROKER_PORT 1883
#define MQTT_CLIENT_ID   "rbteich"

// Topics MQTT utilisés
static const char MQTT_TOPIC_STATUS[]  = "api/3/room/C102/lamp/all/id/1/indication";
static const char MQTT_TOPIC_REQUEST[] = "api/3/room/C102/lamp/all/id/1/request";

static WiFiClient wifiClient;               // Socket WiFi
static PubSubClient mqttClient(wifiClient); // Client MQTT basé sur WiFi

// États système
static bool wifiConnected = false;
static bool mqttConnected = false;
static bool suppressSwitchEvent = false; // Empêche les boucles d'évènements LVGL
static bool lampState = false;           // Stocke l'état ON/OFF de la lampe

// Délai entre essais WiFi
static const unsigned long WIFI_RETRY_DELAY_MS = 5000;
static unsigned long lastWifiAttempt = 0;

// LVGL : buffer de rendu
static lv_disp_draw_buf_t draw_buf;          // Descripteur du buffer
static lv_color_t * draw_buf_data = nullptr; // Mémoire du buffer
static const uint32_t LVGL_BUF_HEIGHT  = 20; // Taille verticale du buffer en lignes

static void touch_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data)
{
    (void)indev_drv;

    M5.Touch.update(); 
    bool pressed = M5.Touch.points > 0; 
    data->state = pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
    if (pressed) {
        data->point.x = M5.Touch.point[0].x;
        data->point.y = M5.Touch.point[0].y;
    }

    data->continue_reading = false; 
}

static void updateStatusText(void) 
{
    if (!ui_TextArea1) return;
    char buffer[64];
    snprintf(buffer, sizeof(buffer),
             "WiFi: %s\nMQTT: %s",
             wifiConnected ? "connected" : "disconnected",
             mqttConnected ? "connected" : "disconnected");

    lv_textarea_set_text(ui_TextArea1, buffer);
}

static void refreshConnectionStatus(void) // MET À JOUR LES VARIABLES wifiConnected ET mqttConnected
{
    bool currentWifi = WiFi.status() == WL_CONNECTED;
    bool currentMqtt = mqttClient.connected();

    if (currentWifi != wifiConnected || currentMqtt != mqttConnected) {
        wifiConnected = currentWifi;
        mqttConnected = currentMqtt;
        updateStatusText();
    }
}

static void connectToWiFi(void)  // CONNEXION AU WiFi
{
    if (WiFi.status() == WL_CONNECTED) return;

    unsigned long now = millis();
    if (lastWifiAttempt != 0 && (now - lastWifiAttempt) < WIFI_RETRY_DELAY_MS)
        return;

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    lastWifiAttempt = now;
}

static void connectToMQTT(void) // CONNEXION AU BROKER MQTT
{
    if (mqttClient.connected() || !wifiConnected) return;

    if (mqttClient.connect(MQTT_CLIENT_ID)) {
        mqttClient.subscribe(MQTT_TOPIC_STATUS); // On s’abonne au topic de statut
    }
}

static void onMQTTMessage(char* topic, byte* payload, unsigned int length) // CALLBACK MQTT (réception d’un message) update ui lors d'un changement de l'état de la lampe
{
    if (!topic || strcmp(topic, MQTT_TOPIC_STATUS) != 0) return; // si le message vien du bon topic

    // Compare "on" ou "off"
    bool newState = (strncmp((char*)payload, "on", 2) == 0); //si c on allumée sinon éteinte

    
    if (newState != lampState) {
        lampState = newState;
        suppressSwitchEvent = true;
        updateLampSwitch(lampState);
        suppressSwitchEvent = false;
    }
}

static void updateLampSwitch(bool on) // MET À JOUR L’INTERRUPTEUR LVGL SELON l'état (visuelle)
{
    if (!ui_offon_all_lamps) return; //si l'objet existe

    suppressSwitchEvent = true;

    if (on)
        lv_obj_add_state(ui_offon_all_lamps, LV_STATE_CHECKED);
    else
        lv_obj_clear_state(ui_offon_all_lamps, LV_STATE_CHECKED);

    lv_obj_invalidate(ui_offon_all_lamps);
    suppressSwitchEvent = false;
}

static void lampSwitchEvent(lv_event_t * e) // ÉVÈNEMENT UI : SWITCH ON/OFF quand on touche le switch 
{
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED || suppressSwitchEvent)
        return;

    lv_obj_t * target = lv_event_get_target(e);
    bool on = lv_obj_has_state(target, LV_STATE_CHECKED);

    if (on == lampState) return;

    lampState = on;

    // Envoi de la commande MQTT au serveur
    if (mqttClient.connected()) {
        mqttClient.publish(MQTT_TOPIC_REQUEST, lampState ? "on" : "off");
    }
}


static void display_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * px_map)
{
    uint32_t width  = (area->x2 - area->x1 + 1);
    uint32_t height = (area->y2 - area->y1 + 1);

    M5.Lcd.startWrite();
    M5.Lcd.setAddrWindow(area->x1, area->y1, width, height);
    M5.Lcd.pushColors((uint16_t*)px_map, width * height, true);
    M5.Lcd.endWrite();

    lv_disp_flush_ready(disp_drv); // Indique à LVGL que l’affichage est fini
}

void setup()
{
    M5.begin();
    M5.Lcd.setRotation(1); 
    Serial.begin(115200);

    lv_init(); // Initialise LVGL

    // Dimensions de l'écran
    uint32_t hor_res = M5.Lcd.width();
    uint32_t ver_res = M5.Lcd.height();
    uint32_t buf_pixels = hor_res * LVGL_BUF_HEIGHT;

    draw_buf_data = (lv_color_t*) malloc(sizeof(lv_color_t) * buf_pixels);
    lv_disp_draw_buf_init(&draw_buf, draw_buf_data, nullptr, buf_pixels);



    // ----- Driver d’affichage -----
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res  = hor_res;
    disp_drv.ver_res  = ver_res;
    disp_drv.draw_buf = &draw_buf;
    disp_drv.flush_cb = display_flush;
    lv_disp_drv_register(&disp_drv);

    // ----- Driver du tactile -----
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type    = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touch_read;
    lv_indev_drv_register(&indev_drv);

    // ----- Interface SquareLine -----
    ui_init();
    if (ui_offon_all_lamps) {
        lv_obj_add_event_cb(ui_offon_all_lamps, lampSwitchEvent,
                            LV_EVENT_VALUE_CHANGED, NULL);
        updateLampSwitch(lampState);
    }

    // ----- MQTT -----
    mqttClient.setServer(MQTT_BROKER_HOST, MQTT_BROKER_PORT);
    mqttClient.setCallback(onMQTTMessage);

    updateStatusText();
    connectToWiFi();
}

void loop()
{
    M5.update();
    refreshConnectionStatus();

    // Reconnexion automatique WiFi / MQTT
    if (!wifiConnected)
        connectToWiFi();

    connectToMQTT();

    if (mqttClient.connected())
        mqttClient.loop(); // Traite les messages MQTT

    lv_tick_inc(5);
    lv_timer_handler();
    delay(5); 
}
