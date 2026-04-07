#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

bool io_sensor_init(void);

bool io_sensor_set_output(uint8_t output_id, bool value);
bool io_sensor_get_output(uint8_t output_id, bool *value);

#ifdef __cplusplus
}
#endif