# Documentation complète du sketch `m5StackTest2.ino`

---

## Bibliothèques et dépendances

Le sketch inclut les fichiers suivants :

```cpp
#include <M5Core2.h>          // abstraction de l’écran tactile, écran, boutons, etc.
#include <Ticker.h>           // minuterie logicielle pour le tick LVGL.
#include <lvgl.h>             // moteur d’interface graphique.
#include "ui.h"               // interface générée par SquareLine Studio.
#include <WiFi.h>             // gestion Wi-Fi spécifique ESP32.
#include <PubSubClient.h>     // client MQTT simple.
#include <cctype>             // fonctions de manipulation de caractères (tolower).
#include <cstring>            // memcpy, strlen, strcmp.
#include <cstdio>             // snprintf.
#include <cstdlib>            // malloc/free.
#include <esp_heap_caps.h>    // allocation SPIRAM optimisée pour LVGL.
```

> Astuce : dans `ui.h`, corriger la ligne mentionnée en tête du fichier (`#include <lvgl.h>` à la place de `#include ".../m5StackTest2"`).

---

## 🌐 Configuration réseau et MQTT (début du fichier)

```cpp
#define WIFI_SSID        "labo-stagiaire"
#define WIFI_PASSWORD    "Uk0IHXsaTt4u"
#define MQTT_BROKER_HOST "192.168.7.13"
#define MQTT_BROKER_PORT 1883
#define MQTT_CLIENT_ID   "rbteich"
```

Les topics :
```cpp
MQTT_TOPIC_STATUS:// réception (mise à jour UI).
MQTT_TOPIC_REQUEST:// publication sur les actions utilisateur.
```

## LVGL + tactile (fonctions autour de `touch_read`, `display_flush`, `lv_tick_task`)

| Fonction | Description |
| --- | --- |
| `touch_read` | Interroge `M5.Touch`, fournit l’état et les coordonnées à LVGL. |
| `display_flush` | Envoie une région LVGL vers l’écran via `M5.Lcd`. |
| `lv_tick_task` | Incrémente le tick LVGL toutes les 5 ms (via `Ticker`). |


---

## Gestion des connexions Wi-Fi et MQTT

### `connectToWiFi()` (appelé dans `setup()` puis dans `loop()`)
* Lance la connexion en mode STA.
* Réessaie uniquement toutes les 5 secondes (`WIFI_RETRY_DELAY_MS`).

### `connectToMQTT()` (dans `loop()`)
* Attend une connexion Wi-Fi.
* Si le client MQTT n’est pas connecté, tente l’identification + s’abonne au topic de statut.

### `refreshConnectionStatus()`
* Inspecte l’état réel du Wi-Fi et de MQTT.
* Met à jour le texte `ui_TextArea1` si ça change via `updateStatusText()`.

---

## Synchronisation lampe ↔ UI ↔ MQTT

### `updateLampSwitch(bool on)`
* Modifie l’état LVGL (`ui_offon_all_lamps`)

### `onMQTTMessage(...)`
* Compare le topic reçu (`MQTT_TOPIC_STATUS`).
* Convertit la charge utile en minuscules, reconnaît `on/off``.
* Si l’état diffère, met à jour `lampState` + déclenche `updateLampSwitch`.

### `lampSwitchEvent(lv_event_t * e)`
* Géré sur l’objet `ui_offon_all_lamps`.
* Si l’utilisateur change le switch, publie une requête MQTT (`MQTT_TOPIC_REQUEST`).

---

## `setup()` : préparation complète

1. Démarre la carte `M5.begin()` et configure l’écran (`rotation = 1`).
2. Initialise LVGL (`lv_init()`).
3. Alloue un buffer LVGL de `hor_res * LVGL_BUF_HEIGHT` pixels, préfère la SPIRAM via `heap_caps_malloc`.
4. Configure le driver d’affichage + le driver tactile.
5. Démarre le `Ticker` LVGL (`lv_tick_timer.attach_ms`).
6. Charge l’UI SquareLine via `ui_init()` et branche `lampSwitchEvent`.
7. Configure le client MQTT (`setServer`, `setCallback`).
8. Met à jour le statut affiché et démarre la connexion Wi-Fi initiale.

---

## `loop()` : orchestration continue


1. `M5.update()`  boucle interne de la librairie M5Core2.
2. `refreshConnectionStatus()` synchronise les indicateurs Wi-Fi/MQTT.
3. `connectToWiFi()` / `connectToMQTT()`  maintien des liaisons réseau.
4. `mqttClient.loop()` si connecté.
5. `lv_timer_handler()` + `delay(5)`  fait tourner LVGL.

---
