#include "SwitchControl.h"

#include <lvgl.h>

static lv_obj_t *s_switch_obj = nullptr;
static SwitchControlPublishFn s_publish_callback = nullptr;
static bool s_ignore_event = false;

static void switch_event_cb(lv_event_t *event)
{
  if (s_ignore_event || s_publish_callback == nullptr) {
    return;
  }
  lv_obj_t *target = lv_event_get_target(event);
  if (target == nullptr) {
    return;
  }
  const bool is_on = lv_obj_has_state(target, LV_STATE_CHECKED);
  s_publish_callback(is_on);
}

void SwitchControl_Init(SwitchControlPublishFn publish)
{
  s_publish_callback = publish;
}

void SwitchControl_RegisterSwitch(lv_obj_t *obj)
{
  s_switch_obj = obj;
  if (s_switch_obj == nullptr) {
    return;
  }
  lv_obj_add_event_cb(s_switch_obj, switch_event_cb, LV_EVENT_VALUE_CHANGED, nullptr);
}

void SwitchControl_SetFromMqtt(bool on)
{
  if (s_switch_obj == nullptr) {
    return;
  }
  const bool currently_on = lv_obj_has_state(s_switch_obj, LV_STATE_CHECKED);
  if (currently_on == on) {
    return;
  }
  s_ignore_event = true;
  if (on) {
    lv_obj_add_state(s_switch_obj, LV_STATE_CHECKED);
  } else {
    lv_obj_clear_state(s_switch_obj, LV_STATE_CHECKED);
  }
  s_ignore_event = false;
}
