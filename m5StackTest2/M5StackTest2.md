# m5StackTest2.ino expliqué
## Aperçu
Ce sketch équipe le **M5Core2** d’une interface LVGL (via `ui.h`) et le connecte au WiFi + MQTT du labo pour piloter un éclairage distant (salle C102, lampe 1). LVGL gère l’écran tactile (toggle + texte de statut) pendant que l’ESP32 dialogue avec le broker.

## Includes essentiels
```cpp
#include <M5Core2.h>
#include <lvgl.h>
#include "ui.h"
#include <WiFi.h>
#include <PubSubClient.h>
```
* `M5Core2.h` apporte l’écran, le tactile et M5.Touch.
* `lvgl.h` fournit le moteur graphique (widgets, buffers, timers, events).
* `ui.h` contient les widgets SquareLine (`ui_offon_all_lamps`, `ui_TextArea1`, etc.).
* `WiFi.h` et `PubSubClient.h` permettent la connexion réseau et MQTT.

## Constantes réseau
```cpp
#define WIFI_SSID        "labo-stagiaire"
#define WIFI_PASSWORD    "Uk0IHXsaTt4u"
#define MQTT_BROKER_HOST "192.168.7.13"
#define MQTT_BROKER_PORT 1883
#define MQTT_CLIENT_ID   "rbteich"
```
* Ces constantes sont suivies des topics MQTT statuts et commandes :  
  `api/3/room/C102/lamp/all/id/1/indication` pour recevoir l’état,  
  `api/3/room/C102/lamp/all/id/1/request` pour envoyer `on`/`off`.

## LVGL : buffer, tactile et flush
* `draw_buf_data` est alloué dynamiquement (`LVGL_BUF_HEIGHT = 20`) pour limiter la RAM tout en permettant l’affichage.
* `touch_read()` met à jour `M5.Touch`, remonte l’état de pression et fournit les coordonnées à LVGL.
* `display_flush()` écrit les pixels rendus dans `px_map` vers l’écran via `M5.Lcd.setAddrWindow` puis termine par `lv_disp_flush_ready()` pour libérer le buffer.

## Gestion MQTT ↔ interface
* `connectToWiFi()` relance la connexion WiFi toutes les 5 s si nécessaire, `connectToMQTT()` s’abonne au topic statut une fois le WiFi établi.
* `refreshConnectionStatus()` met à jour `wifiConnected` et `mqttConnected`, puis écrit `ui_TextArea1` via `updateStatusText()`.
* `onMQTTMessage()` écoute le topic statut et met à jour `lampState` dès qu’un payload commence par `"on"` (sinon OFF).
  Le callback appelle `updateLampSwitch()` avec `suppressSwitchEvent` pour éviter de déclencher `lampSwitchEvent`.
* `lampSwitchEvent()` lit l’état de l’interrupteur LVGL, publie `"on"`/`"off"` et met à jour `lampState` si la valeur a changé.

## setup()
1. `M5.begin()`, rotation paysage `M5.Lcd.setRotation(1)` et `Serial.begin(115200)`.
2. `lv_init()` + allocation du buffer LVGL, puis `lv_disp_draw_buf_init`.
3. Initialisation des drivers d’affichage et tactile LVGL, `disp_drv.flush_cb = display_flush` et `indev_drv.read_cb = touch_read`.
4. `ui_init()` construit l’IHM SquareLine, lie `ui_offon_all_lamps` à `lampSwitchEvent()` et force l’état actuel via `updateLampSwitch(lampState)`.
5. Configuration MQTT (`setServer`, `setCallback`), affichage du statut initial et lancement de `connectToWiFi()`.

## loop()
```cpp
void loop() {
    M5.update();
    refreshConnectionStatus();
    if (!wifiConnected) connectToWiFi();
    connectToMQTT();
    if (mqttClient.connected()) mqttClient.loop();
    lv_tick_inc(5);
    lv_timer_handler();
    delay(5);
}
```
* La boucle met à jour le tactile (`M5.update`), vérifie les connexions, gère MQTT, et alimente LVGL avec des ticks + timers toutes les 5 ms.

## Résumé
- Une interface LVGL simple (toggle + texte) dialogue avec MQTT pour piloter la lampe.
- `refreshConnectionStatus`, `updateStatusText` et `updateLampSwitch` gardent l’UI synchronisée sans boucles d’évènements.
- `touch_read`, `display_flush`, `lv_tick_inc` et `lv_timer_handler` alimentent le moteur LVGL sur le M5Core2.
