#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

bool ct_sensor_init(void);
bool ct_sensor_start(void);

uint16_t ct_sensor_get_last_raw(void);
bool ct_sensor_is_alarm_active(void);

#ifdef __cplusplus
}
#endif