#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t io_drv_init(void);

esp_err_t io_drv_set_output(uint8_t output_id, bool value);
esp_err_t io_drv_get_output(uint8_t output_id, bool *value);
esp_err_t io_drv_get_output_level(uint8_t output_id, int *level);

#ifdef __cplusplus
}
#endif