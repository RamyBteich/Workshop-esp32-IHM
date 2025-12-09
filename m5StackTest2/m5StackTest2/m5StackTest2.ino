// Change this line "#include "/home/ramy/Desktop/WorkShop/m5StackTest1" 
// in ui .h to "#include <lvgl.h>" (line 13)


#include <M5Core2.h>
#include <Ticker.h>
#include <lvgl.h>
#include "ui.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <cctype>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <esp_heap_caps.h>

#define WIFI_SSID        "labo-stagiaire"
#define WIFI_PASSWORD    "Uk0IHXsaTt4u"
#define MQTT_BROKER_HOST "192.168.7.13"
#define MQTT_BROKER_PORT 1883
#define MQTT_CLIENT_ID   "rbteich"

static const char MQTT_TOPIC_STATUS[] = "api/3/room/C102/lamp/all/id/1/indication";
static const char MQTT_TOPIC_REQUEST[] = "api/3/room/C102/lamp/all/id/1/request";

static WiFiClient wifiClient;
static PubSubClient mqttClient(wifiClient);

static bool wifiConnected = false;
static bool mqttConnected = false;
static bool suppressSwitchEvent = false;
static bool lampState = false;
static const unsigned long WIFI_RETRY_DELAY_MS = 5000;
static unsigned long lastWifiAttempt = 0;

static void updateStatusText(void);
static void updateLampSwitch(bool on);
static void refreshConnectionStatus(void);
static void connectToWiFi(void);
static void connectToMQTT(void);
static void onMQTTMessage(char * topic, byte * payload, unsigned int length);
static void lampSwitchEvent(lv_event_t * e);

static const uint32_t LVGL_TICK_RATE_MS = 5;
static const uint32_t LVGL_BUF_HEIGHT = 20;

static Ticker lv_tick_timer;
static lv_disp_draw_buf_t draw_buf;
static lv_color_t * draw_buf_data = nullptr;

static void touch_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data)
{
    (void)indev_drv;
    M5.Touch.update();
    const bool pressed = M5.Touch.points > 0;
    data->state = pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
    if(pressed) {
        lv_point_t point;
        point.x = M5.Touch.point[0].x;
        point.y = M5.Touch.point[0].y;
        data->point = point;
    }
    data->continue_reading = false;
}
//////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
static void connectToWiFi(void)
{
    if(WiFi.status() == WL_CONNECTED) {
        return;
    }

    const unsigned long now = millis();
    if(lastWifiAttempt != 0 && (now - lastWifiAttempt) < WIFI_RETRY_DELAY_MS) {
        return;
    }

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    lastWifiAttempt = now;
}

static void connectToMQTT(void)
{
    if(mqttClient.connected() || !wifiConnected) {
        return;
    }

    if(mqttClient.connect(MQTT_CLIENT_ID)) {
        mqttClient.subscribe(MQTT_TOPIC_STATUS);
    }
}

static void updateStatusText(void)
{
    if(!ui_TextArea1) {
        return;
    }

    char buffer[64];
    snprintf(buffer, sizeof(buffer), "WiFi: %s\nMQTT: %s",
             wifiConnected ? "connected" : "disconnected",
             mqttConnected ? "connected" : "disconnected");
    lv_textarea_set_text(ui_TextArea1, buffer);
}
static void refreshConnectionStatus(void)
{
    const bool currentWifi = WiFi.status() == WL_CONNECTED;
    const bool currentMqtt = mqttClient.connected();

    if(currentWifi != wifiConnected || currentMqtt != mqttConnected) {
        wifiConnected = currentWifi;
        mqttConnected = currentMqtt;
        updateStatusText();
    }
}

//////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

static void updateLampSwitch(bool on)
{
    if(!ui_offon_all_lamps) {
        return;
    }
    suppressSwitchEvent = true;
    if(on) {
        lv_obj_add_state(ui_offon_all_lamps, LV_STATE_CHECKED);
    } else {
        lv_obj_clear_state(ui_offon_all_lamps, LV_STATE_CHECKED);
    }
    lv_obj_invalidate(ui_offon_all_lamps);
    suppressSwitchEvent = false;
}


static void onMQTTMessage(char* topic, byte* payload, unsigned int length)
{
    if (!topic || strcmp(topic, MQTT_TOPIC_STATUS) != 0)
        return;

    bool newState = (strncmp((char*)payload, "on", 2) == 0);

    if (newState != lampState) {
        lampState = newState;
        updateLampSwitch(lampState);
    }
}

static void lampSwitchEvent(lv_event_t * e)
{
    if(lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED || suppressSwitchEvent) {
        return;
    }

    lv_obj_t * target = static_cast<lv_obj_t *>(lv_event_get_target(e));
    const bool on = static_cast<bool>(lv_obj_has_state(target, LV_STATE_CHECKED));

    if(on == lampState) {
        return;
    }

    lampState = on;

    if(mqttClient.connected()) {
        const char * payload = lampState ? "on" : "off";
        mqttClient.publish(MQTT_TOPIC_REQUEST, payload);
    }
}

static void lv_tick_task()
{
    lv_tick_inc(LVGL_TICK_RATE_MS);
}

static void display_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * px_map)
{
    const uint32_t width = (uint32_t)area->x2 - area->x1 + 1;
    const uint32_t height = (uint32_t)area->y2 - area->y1 + 1;

    M5.Lcd.startWrite();
    M5.Lcd.setAddrWindow(area->x1, area->y1, width, height);
    M5.Lcd.pushColors(reinterpret_cast<uint16_t *>(px_map), width * height, true);
    M5.Lcd.endWrite();

    lv_disp_flush_ready(disp_drv);
}

void setup()
{
    M5.begin();
    M5.Lcd.setRotation(1);
    Serial.begin(115200);

    lv_init();

    const uint32_t hor_res = M5.Lcd.width();
    const uint32_t ver_res = M5.Lcd.height();

    const uint32_t buf_pixels = hor_res * LVGL_BUF_HEIGHT;
    draw_buf_data = static_cast<lv_color_t *>(heap_caps_malloc(sizeof(lv_color_t) * buf_pixels, MALLOC_CAP_SPIRAM));
    if(!draw_buf_data) {
        draw_buf_data = static_cast<lv_color_t *>(malloc(sizeof(lv_color_t) * buf_pixels));
    }
    lv_disp_draw_buf_init(&draw_buf, draw_buf_data, nullptr, buf_pixels);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = hor_res;
    disp_drv.ver_res = ver_res;
    disp_drv.draw_buf = &draw_buf;
    disp_drv.flush_cb = display_flush;
    lv_disp_drv_register(&disp_drv);

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touch_read;
    lv_indev_drv_register(&indev_drv);

    lv_tick_timer.attach_ms(LVGL_TICK_RATE_MS, lv_tick_task);

    ui_init();
    if(ui_offon_all_lamps) {
        lv_obj_add_event_cb(ui_offon_all_lamps, lampSwitchEvent, LV_EVENT_VALUE_CHANGED, NULL);
        updateLampSwitch(lampState);
    }

    mqttClient.setServer(MQTT_BROKER_HOST, MQTT_BROKER_PORT);
    mqttClient.setCallback(onMQTTMessage);

    updateStatusText();
    connectToWiFi();
}

void loop()
{
    M5.update();
    refreshConnectionStatus();

    if(!wifiConnected) {
        connectToWiFi();
    }

    connectToMQTT();

    if(mqttClient.connected()) {
        mqttClient.loop();
    }

    lv_timer_handler();
    delay(LVGL_TICK_RATE_MS);
}
