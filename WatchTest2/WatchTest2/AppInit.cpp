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

#ifndef WIFI_SSID
#define WIFI_SSID "labo-stagiaire"
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "Uk0IHXsaTt4u"
#endif

#ifndef MQTT_BROKER
#define MQTT_BROKER "192.168.7.13"
#endif

#ifndef MQTT_PORT
#define MQTT_PORT 1883
#endif

#ifndef MQTT_CLIENT_ID
#define MQTT_CLIENT_ID "rbteich-2"
#endif

#ifndef MQTT_USERNAME
#define MQTT_USERNAME NULL
#endif

#ifndef MQTT_PASSWORD
#define MQTT_PASSWORD NULL
#endif

#ifndef MQTT_STATUS_TOPIC
#define MQTT_STATUS_TOPIC "api/3/room/C102/lamp/all/id/1/indication"
#endif

#ifndef MQTT_REQUEST_TOPIC
#define MQTT_REQUEST_TOPIC "api/3/room/C102/lamp/all/id/1/request"
#endif

static WiFiClient s_wifi_client;
static PubSubClient s_mqtt_client(s_wifi_client);

static char s_mqtt_client_id[32];
static constexpr const char * s_mqtt_broker = MQTT_BROKER;
static constexpr const char * s_mqtt_status_topic = MQTT_STATUS_TOPIC;
static constexpr const char * s_mqtt_request_topic = MQTT_REQUEST_TOPIC;

static constexpr unsigned long s_mqtt_reconnect_interval_ms = 5000;
static unsigned long s_last_mqtt_reconnect = 0;

enum class ConnectionState : int {
  BrokerNotConfigured = -1,
  Disconnected = 0,
  Connected = 1,
};

static ConnectionState s_last_connection_state = ConnectionState::BrokerNotConfigured;
static bool s_force_status_update = true;

static void prepare_mqtt_client_id()
{
  uint64_t chipid = ESP.getEfuseMac();
  uint32_t short_id = static_cast<uint32_t>(chipid & 0xFFFFFFFF);
  snprintf(s_mqtt_client_id, sizeof(s_mqtt_client_id), "%s-%08X", MQTT_CLIENT_ID, short_id);
}

static ConnectionState compute_connection_state()
{
  if (s_mqtt_broker[0] == '\0') {
    return ConnectionState::BrokerNotConfigured;
  }
  return s_mqtt_client.connected() ? ConnectionState::Connected : ConnectionState::Disconnected;
}

static void refresh_state_text()
{
  if (!ui_TextArea1) {
    return;
  }

  ConnectionState current = compute_connection_state();
  if (!s_force_status_update && current == s_last_connection_state) {
    return;
  }

  const char *status = nullptr;
  switch (current) {
    case ConnectionState::Connected:
      status = "Connected";
      break;
    case ConnectionState::Disconnected:
      status = "Disconnected";
      break;
    case ConnectionState::BrokerNotConfigured:
    default:
      status = "Broker not configured";
      break;
  }

  char buffer[128];
  snprintf(buffer, sizeof(buffer), "MQTT %s\n%s", status, s_mqtt_status_topic);
  lv_textarea_set_text(ui_TextArea1, buffer);
  s_last_connection_state = current;
  s_force_status_update = false;
}

static bool payload_indicates_on(const unsigned char *payload, unsigned int length)
{
  if (length == 0 || payload == nullptr) {
    return false;
  }
  char temp[16];
  unsigned int copy_len = length < (sizeof(temp) - 1) ? length : (sizeof(temp) - 1);
  memcpy(temp, payload, copy_len);
  temp[copy_len] = '\0';
  for (unsigned int i = 0; i < copy_len; ++i) {
    temp[i] = static_cast<char>(tolower(static_cast<unsigned char>(temp[i])));
  }

  return (strcmp(temp, "1") == 0) || (strcmp(temp, "on") == 0) || (strcmp(temp, "true") == 0) ||
         (strcmp(temp, "high") == 0);
}

static void mqtt_message_callback(char *topic, unsigned char *payload, unsigned int length)
{
  if (strcmp(topic, s_mqtt_status_topic) == 0) {
    const bool on = payload_indicates_on(payload, length);
    SwitchControl_SetFromMqtt(on);
  }
}

static void publish_lamp_request(bool on)
{
  if (!s_mqtt_client.connected() || s_mqtt_request_topic[0] == '\0') {
    return;
  }
  const char *payload = on ? "on" : "off";
  s_mqtt_client.publish(s_mqtt_request_topic, payload, true);
}

static void try_reconnect_mqtt()
{
  if (s_mqtt_broker[0] == '\0') {
    return;
  }
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  if (s_mqtt_client.connected()) {
    return;
  }

  Serial.printf("Connecting MQTT %s\n", s_mqtt_broker);
  if (s_mqtt_client.connect(s_mqtt_client_id, MQTT_USERNAME, MQTT_PASSWORD)) {
    s_mqtt_client.subscribe(s_mqtt_status_topic);
    s_force_status_update = true;
  } else {
    Serial.printf("MQTT connect failed: %d\n", s_mqtt_client.state());
  }
}

static void configure_network()
{
  if (WIFI_SSID[0] == '\0') {
    Serial.println("WiFi SSID not configured");
    return;
  }
  WiFi.mode(WIFI_MODE_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void AppInit()
{
  Serial.println("AppInit");
  I2C_Init();
  Backlight_Init();
  Set_Backlight(60);
  TCA9554PWR_Init();
  LCD_Init();
  SD_Init();
  Audio_Init();
  PCF85063_Init();
  QMI8658_Init();
  Lvgl_Init();
  SwitchControl_Init(publish_lamp_request);
  ui_init();
  SwitchControl_RegisterSwitch(ui_Switch1);
  configure_network();
  if (s_mqtt_broker[0] != '\0') {
    s_mqtt_client.setServer(s_mqtt_broker, MQTT_PORT);
  }
  s_mqtt_client.setCallback(mqtt_message_callback);
  prepare_mqtt_client_id();
  s_force_status_update = true;
  s_last_connection_state = ConnectionState::BrokerNotConfigured;
}

void AppLoop()
{
  if (s_mqtt_broker[0] != '\0' && (WiFi.status() == WL_CONNECTED)) {
    unsigned long now = millis();
    if (now - s_last_mqtt_reconnect > s_mqtt_reconnect_interval_ms) {
      s_last_mqtt_reconnect = now;
      try_reconnect_mqtt();
    }
  }

  s_mqtt_client.loop();
  refresh_state_text();
}

bool AppIsMqttConnected()
{
  return s_mqtt_client.connected();
}
