#pragma once

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t adc_drv_init(void);

esp_err_t adc_drv_read_raw(uint16_t *raw);
esp_err_t adc_drv_read_avg_raw(uint16_t *raw_avg, uint16_t samples);

#ifdef __cplusplus
}
#endif