#include "AppInit.h"

#include <lvgl.h>

#include "Display_SPD2010.h"
#include "I2C_Driver.h"
#include "LVGL_Driver.h"
#include "TCA9554PWR.h"
#include "ui.h"

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <esp_system.h>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstring>

//  WiFi configuration 
#define WIFI_SSID        "labo-stagiaire"
#define WIFI_PASSWORD    "Uk0IHXsaTt4u"

//  MQTT configuration 
#define MQTT_BROKER_HOST "192.168.7.13"
#define MQTT_BROKER_PORT 1883
#define MQTT_CLIENT_ID   "rbteich"
#define MQTT_USERNAME    NULL
#define MQTT_PASSWORD    NULL

//  MQTT Topics 
static const char MQTT_TOPIC_STATUS[]  = "api/3/room/C102/lamp/all/id/1/indication";
static const char MQTT_TOPIC_REQUEST[] = "api/3/room/C102/lamp/all/id/1/request";

// Réseaux / MQTT 
static WiFiClient wifiClient;
static PubSubClient mqttClient(wifiClient);
static char mqttClientId[32];

// États système
static bool wifiConnected       = false;
static bool mqttConnected       = false;
static bool lampState           = false; // état ON/OFF de la lampe
static bool suppressSwitchEvent = false; // empêche les boucles d’évènements LVGL

// WiFi retry timer
static const unsigned long WIFI_RETRY_DELAY_MS = 5000;
static unsigned long lastWifiAttempt           = 0;

static void prepare_mqtt_client_id()
{
    uint64_t chipid   = ESP.getEfuseMac();
    uint32_t short_id = static_cast<uint32_t>(chipid & 0xFFFFFFFF);
    snprintf(mqttClientId, sizeof(mqttClientId), "%s-%08X", MQTT_CLIENT_ID, short_id);
}

static void updateStatusText()
{
    if (!ui_TextArea1) return;

    char buffer[128];
    snprintf(buffer, sizeof(buffer),
             "WiFi: %s\nMQTT: %s\n%s",
             wifiConnected ? "connected" : "disconnected",
             mqttConnected ? "connected" : "disconnected",
             MQTT_TOPIC_STATUS);

    lv_textarea_set_text(ui_TextArea1, buffer);
}

static void refreshConnectionStatus()
{
    bool currentWifi = (WiFi.status() == WL_CONNECTED);
    bool currentMqtt = mqttClient.connected();

    if (currentWifi != wifiConnected || currentMqtt != mqttConnected) {
        wifiConnected = currentWifi;
        mqttConnected = currentMqtt;
        updateStatusText();
    }
}

static void connectToWiFi()
{
    if (WiFi.status() == WL_CONNECTED) {
        return;
    }

    unsigned long now = millis();
    if (lastWifiAttempt != 0 &&
        (now - lastWifiAttempt) < WIFI_RETRY_DELAY_MS) {
        return;
    }

    Serial.printf("Connecting to WiFi SSID: %s\n", WIFI_SSID);
    WiFi.mode(WIFI_MODE_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    lastWifiAttempt = now;
}

static void connectToMQTT()
{
    if (mqttClient.connected()) {
        return;
    }
    if (!wifiConnected) {
        return;
    }

    Serial.printf("Connecting to MQTT broker: %s\n", MQTT_BROKER_HOST);
    if (mqttClient.connect(mqttClientId, MQTT_USERNAME, MQTT_PASSWORD)) {
        Serial.println("MQTT connected");
        mqttClient.subscribe(MQTT_TOPIC_STATUS);  // subscribe to status topic
    } else {
        Serial.printf("MQTT connect failed: %d\n", mqttClient.state());
    }
}

// Gestion du switch LVGL et de la lampe
static void updateLampSwitch(bool on)
{
    
    if (!ui_Switch1) return;

    suppressSwitchEvent = true;

    if (on)
        lv_obj_add_state(ui_Switch1, LV_STATE_CHECKED);
    else
        lv_obj_clear_state(ui_Switch1, LV_STATE_CHECKED);

    lv_obj_invalidate(ui_Switch1); // redessine l’objet
    suppressSwitchEvent = false;
}

static void publishLampRequest(bool on)
{
    lampState = on;

    if (!mqttClient.connected()) {
        return;
    }

    const char *payload = on ? "on" : "off";
    // QoS 0 + retained true, comme ton ancienne version
    mqttClient.publish(MQTT_TOPIC_REQUEST, payload, true);
}

static void lampSwitchEvent(lv_event_t * e)
{
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED || suppressSwitchEvent)
        return;

    lv_obj_t * target = lv_event_get_target(e);
    bool on = lv_obj_has_state(target, LV_STATE_CHECKED);

    if (on == lampState) return; // Aucun changement réel

    // Mets à jour l’état local + envoie la commande MQTT
    publishLampRequest(on);
}

static void onMQTTMessage(char *topic, uint8_t *payload, unsigned int length)
{
    if (!topic) return;

    if (strcmp(topic, MQTT_TOPIC_STATUS) != 0) {
        // ignore autres topics
        return;
    }

    char temp[16];
    unsigned int copy_len = (length < sizeof(temp) - 1) ? length : (sizeof(temp) - 1);
    memcpy(temp, payload, copy_len);
    temp[copy_len] = '\0';

    for (unsigned int i = 0; i < copy_len; ++i) {
        temp[i] = static_cast<char>(tolower(static_cast<unsigned char>(temp[i])));
    }

    bool newState = (strncmp(temp, "on", 2) == 0);

    if (newState != lampState) {
        lampState = newState;
        updateLampSwitch(lampState); // mise à jour de l’UI
    }
}

// Configuration réseau / MQTT
static void configureNetwork()
{
    mqttClient.setServer(MQTT_BROKER_HOST, MQTT_BROKER_PORT);
    mqttClient.setCallback(onMQTTMessage);

    connectToWiFi();
}

void AppInit()
{
    Serial.println("AppInit");

    I2C_Init();
    Backlight_Init();
    Set_Backlight(60);
    TCA9554PWR_Init();
    LCD_Init();
    Lvgl_Init();

    ui_init();

    if (ui_Switch1) {
        lv_obj_add_event_cb(ui_Switch1, lampSwitchEvent, LV_EVENT_VALUE_CHANGED, nullptr);
        updateLampSwitch(lampState); 
    }

    prepare_mqtt_client_id();
    configureNetwork();

    refreshConnectionStatus();
    updateStatusText();
}

void AppLoop()
{
    refreshConnectionStatus();

    if (!wifiConnected) {
        connectToWiFi();
    }

    connectToMQTT();

    if (mqttClient.connected()) {
        mqttClient.loop();
    }

}

bool AppIsMqttConnected()
{
    return mqttClient.connected();
}
