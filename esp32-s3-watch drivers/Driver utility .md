# Driver utility

Ce document en français liste chaque pilote en une ligne afin de voir rapidement qui se charge de quoi dans la montre ESP32-S3 (chaque paire `.cpp/.h` est regroupée sur la même ligne).

- `Audio_PCM5101.cpp/.h` : pilote I2S du DAC PCM5101 (initialisation, timer de boucle audio, lecture depuis la SD, commandes volume/pause/lecture/gestion d'un test de musique).
- `BAT_Driver.cpp/.h` : acquisition de la tension de batterie via un ADC 12 bits et application d'un offset de calibration pour estimer la charge.
- `Display_SPD2010.cpp/.h` : pilotage haut niveau de la dalle SPD2010 (reset, SPI/QSPI, test graphique, fenêtre de dessin partielle et rétroéclairage PWM).
- `esp_lcd_spd2010.c/.h` : couche vendor ESP-IDF qui implémente `esp_lcd_panel_t` pour la SPD2010 (opérations d'initialisation, draw, inversion, gap, mirror, swap et on/off).
- `Gyro_QMI8658.cpp/.h` : configuration complète du capteur QMI8658 par I2C (échelles, ODR, filtres, lecture brute et mise à jour des données accéléro/gyro).
- `I2C_Driver.cpp/.h` : abstraction Wire : init du bus, lecture/écriture avec vérification d'erreur pour les autres capteurs.
- `LVGL_Driver.cpp/.h` : pont LVGL ↔ écran : buffers, flush/rounder/tick/tâches tactiles, callback d'entrée et boucle LVGL.
- `LVGL_Music.cpp/.h` : implémentation du démo musical LVGL intégrée (création des panels, gestion des listes MP3, timers de spectre, boutons et animations).
- `MIC_MSM.cpp/.h` : pilote microphone+reconnaissance vocale (I2S), gestion des événements wakeword/commandes et actions sur le rétroéclairage ou la musique.
- `PWR_Key.cpp/.h` : gestion du bouton d'alimentation (détection de longues pressions pour veille/restart/shutdown et contrôle de l'alimentation via un GPIO). 
- `RTC_PCF85063.cpp/.h` : pilotage de l'horloge RTC PCF85063 (init, lecture/écriture de date/heure/alarmes, conversions BCD et formatage de chaîne).
- `SD_Card.cpp/.h` : configuration du bus SD_MMC, manipulation de D3 via l'expander, recherche de fichiers et récupération des dossiers MP3.
- `TCA9554PWR.cpp/.h` : interface I2C vers l'expanseur TCA9554PWR (registres, configuration de mode, écriture/niveau/toggle de broches EXIO).
- `Touch_SPD2010.cpp/.h` : pilotage tactile SPD2010 (I2C 16 bits, ISR, lecture multi-points, commandes de mode, conversion des rapports de contact).
- `Wireless.cpp/.h` : tests de scan WiFi/BLE (mise en mode station, scan, comptage des périphériques via tâche FreeRTOS). 


