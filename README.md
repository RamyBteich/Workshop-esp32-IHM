# ESP32/IHM WORKSHOP

Dans cet atelier, nous allons voir comment utiliser SquareLine Studio pour créer une interface sur le M5Stack Core2 ou l’ESP32-S3 Touch LCD 1.46B. Nous verrons comment publier des messages vers un broker MQTT afin de contrôler une lampe connectée etc... 

## Carte utilisée
- M5 Stack core 2
- ESP32/S3-Touch-LCD-1.46b

```cpp
- esp32 by Espressif Systems (v 2.0.8) pour la M5stack core2
- esp32 by Espressif Systems (v 3.3.0) pour la Montre ESP32-S3
```

## Liste des bibliothèques
```cpp
- M5Core2 by M5Stack
- PubSubClient by Nick O’Leary (v 2.8)
- ESP32-audioI2S-master by schreibfaul1 (v2.0.0)
- lvgl by kisvegabor (v 8.3.11)
```

## Logiciels utilisée
```cpp
- Arduino IDE (https://www.arduino.cc/en/software/)
- PSquare Line studio (https://squareline.io/downloads#lastRelease)
- MQTT Explorer (https://mqtt-explorer.com/)
```

## Tutoriel
Pour le tutoriel sur la création ou l'ouverture d'un projet existant, consultez les diapositives de la présentation. Tout y est expliqué. \
Cependant, il est important de noter que lors de l'exportation des fichiers .lvgl, il ne faut pas les exporter directement dans le dossier contenant le fichier .ino, car cela endommagerait tous les anciens fichiers présents dans ce dossier et entraînerait la réinstallation du fichier .lvgl, ce qui pourrait corrompre le fichier .ino. \
Et pour la montre, nous devons copier le contenu du dossier des pilotes « esp32-s3-watch-drivers » dans le même dossier que le fichier .ino.



[Cela devrait ressembler à ceci](/image/workshop.png)