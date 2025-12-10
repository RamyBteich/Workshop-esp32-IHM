#include "AppInit.h"

#include <lvgl.h>

#include "Display_SPD2010.h"
#include "I2C_Driver.h"
#include "LVGL_Driver.h"
#include "SwitchControl.h"
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

// ===============================
// WiFi / MQTT configuration
// ===============================

// --- WiFi configuration ---
#define WIFI_SSID        "labo-stagiaire"
#define WIFI_PASSWORD    "Uk0IHXsaTt4u"

// --- MQTT configuration ---
#define MQTT_BROKER_HOST "192.168.7.13"
#define MQTT_BROKER_PORT 1883
#define MQTT_CLIENT_ID   "rbteich"
#define MQTT_USERNAME    NULL
#define MQTT_PASSWORD    NULL

// --- MQTT Topics ---
static const char MQTT_TOPIC_STATUS[]  = "api/3/room/C102/lamp/all/id/1/indication";
static const char MQTT_TOPIC_REQUEST[] = "api/3/room/C102/lamp/all/id/1/request";

// ===============================
// Global objects / state
// ===============================

static WiFiClient wifiClient;
static PubSubClient mqttClient(wifiClient);

static char mqttClientId[32];

// Global state flags
static bool wifiConnected     = false;
static bool mqttConnected     = false;
static bool lampState         = false;  // ON/OFF state of lamp
static bool suppressSwitchEvt = false;  // avoid feedback loops

// WiFi retry timer
static const unsigned long WIFI_RETRY_DELAY_MS = 5000;
static unsigned long lastWifiAttempt           = 0;

// ===============================
// Helper: prepare unique MQTT client ID
// ===============================
static void prepare_mqtt_client_id()
{
    uint64_t chipid = ESP.getEfuseMac();
    uint32_t short_id = static_cast<uint32_t>(chipid & 0xFFFFFFFF);
    snprintf(mqttClientId, sizeof(mqttClientId), "%s-%08X", MQTT_CLIENT_ID, short_id);
}

// ===============================
// UI: update status text
// ===============================
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

// ===============================
// Refresh connection flags & UI
// ===============================
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

// ===============================
// WiFi connect (no #ifdef checks, uses #define directly)
// ===============================
static void connectToWiFi()
{
    if (WiFi.status() == WL_CONNECTED) {
        return;
    }

    unsigned long now = millis();
    if (lastWifiAttempt != 0 &&
        (now - lastWifiAttempt) < WIFI_RETRY_DELAY_MS) {
        // wait until retry delay passes
        return;
    }

    Serial.printf("Connecting to WiFi SSID: %s\n", WIFI_SSID);
    WiFi.mode(WIFI_MODE_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    lastWifiAttempt = now;
}

// ===============================
// MQTT connect
// ===============================
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

// ===============================
// Helper: interpret MQTT payload as ON/OFF
// ===============================
static bool payloadIndicatesOn(const unsigned char *payload, unsigned int length)
{
    if (length == 0 || payload == nullptr) {
        return false;
    }

    char temp[16];
    unsigned int copy_len = length < (sizeof(temp) - 1) ? length : (sizeof(temp) - 1);
    memcpy(temp, payload, copy_len);
    temp[copy_len] = '\0';

    // lowercase
    for (unsigned int i = 0; i < copy_len; ++i) {
        temp[i] = static_cast<char>(tolower(static_cast<unsigned char>(temp[i])));
    }

    return (strcmp(temp, "1") == 0)   ||
           (strcmp(temp, "on") == 0)  ||
           (strcmp(temp, "true") == 0)||
           (strcmp(temp, "high") == 0);
}

// ===============================
// Apply new lamp state coming from MQTT
// ===============================
static void applyLampStateFromMqtt(bool on)
{
    if (on == lampState) {
        return;
    }

    lampState = on;

    // Use SwitchControl to update LVGL switch without causing extra callbacks
    suppressSwitchEvt = true;
    SwitchControl_SetFromMqtt(on);
    suppressSwitchEvt = false;
}

// ===============================
// MQTT message callback
// ===============================
static void mqttMessageCallback(char *topic, unsigned char *payload, unsigned int length)
{
    if (!topic) return;

    if (strcmp(topic, MQTT_TOPIC_STATUS) != 0) {
        // ignore other topics
        return;
    }

    bool newState = payloadIndicatesOn(payload, length);
    applyLampStateFromMqtt(newState);
}

// ===============================
// Publish lamp request when UI switch changes
// (this is passed to SwitchControl_Init)
// ===============================
static void publishLampRequest(bool on)
{
    lampState = on; // keep local state in sync

    if (!mqttClient.connected()) {
        return;
    }

    const char *payload = on ? "on" : "off";
    mqttClient.publish(MQTT_TOPIC_REQUEST, payload, true);
}

// ===============================
// Network configuration (MQTT server + callback)
// ===============================
static void configureNetwork()
{
    mqttClient.setServer(MQTT_BROKER_HOST, MQTT_BROKER_PORT);
    mqttClient.setCallback(mqttMessageCallback);

    // initial WiFi attempt
    connectToWiFi();
}

// ===============================
// Public API: AppInit / AppLoop / AppIsMqttConnected
// ===============================
void AppInit()
{
    Serial.println("AppInit");

    // Hardware + LVGL init
    I2C_Init();
    Backlight_Init();
    Set_Backlight(60);
    TCA9554PWR_Init();
    LCD_Init();
    Lvgl_Init();

    // SwitchControl: this will call publishLampRequest when UI switch is used
    SwitchControl_Init(publishLampRequest);

    ui_init();

    // Register the LVGL switch so SwitchControl can control it
    // (adjust ui_Switch1 if your object has another name)
    SwitchControl_RegisterSwitch(ui_Switch1);

    // WiFi / MQTT
    prepare_mqtt_client_id();
    configureNetwork();

    // Initial connection status display
    refreshConnectionStatus();
    updateStatusText();
}

void AppLoop()
{
    // Keep status flags and UI up to date
    refreshConnectionStatus();

    // Reconnect WiFi if needed
    if (!wifiConnected) {
        connectToWiFi();
    }

    // Reconnect MQTT if needed
    connectToMQTT();

    // Process incoming MQTT messages
    if (mqttClient.connected()) {
        mqttClient.loop();
    }

    // LVGL timing / drawing is handled by your LVGL driver elsewhere
}

bool AppIsMqttConnected()
{
    return mqttClient.connected();
}
