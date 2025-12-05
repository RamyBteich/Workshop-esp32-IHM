SET(SOURCES ui_Main_.c
    ui_MQTT_Info.c
    ui_Lamp.c
    ui.c
    ui_comp_hook.c
    ui_helpers.c)

add_library(ui ${SOURCES})
