#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void app_post_system_event(int32_t event_id, const void *data, size_t len);
void app_post_report_event(int32_t event_id, const void *data, size_t len);
void app_post_sensor_event(int32_t event_id, const void *data, size_t len);
void app_post_command_event(int32_t event_id, const void *data, size_t len);

#ifdef __cplusplus
}
#endif