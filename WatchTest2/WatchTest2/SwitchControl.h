#pragma once

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*SwitchControlPublishFn)(bool on);

void SwitchControl_Init(SwitchControlPublishFn publish);
void SwitchControl_RegisterSwitch(lv_obj_t *obj);
void SwitchControl_SetFromMqtt(bool on);

#ifdef __cplusplus
}
#endif
