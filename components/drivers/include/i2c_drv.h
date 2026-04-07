#pragma once

#include "esp_err.h"
#include "stdbool.h"
#include "driver/i2c.h"  // Cambiado de i2c_master.h a i2c.h

#define I2C_MASTER_NUM             I2C_NUM_0

typedef struct {
    uint16_t device_address;
    uint32_t scl_speed;
} i2c_drv_cfg_t;


esp_err_t i2c_drv_init(i2c_drv_cfg_t *cfg);

bool send_data_i2c(const uint8_t *data, size_t len, i2c_drv_cfg_t *cfg);

bool read_data_i2c(uint8_t *data, size_t len, i2c_drv_cfg_t *cfg);