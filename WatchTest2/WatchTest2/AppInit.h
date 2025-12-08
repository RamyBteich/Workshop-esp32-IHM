#pragma once

/// Initialize LVGL, the UI and the network stack (WiFi + MQTT).
void AppInit();

/// Run per-iteration work such as MQTT loop and UI updates.
void AppLoop();

/// Returns true when the MQTT connection is currently active.
bool AppIsMqttConnected();
