#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t output_id;
    bool value;
} app_cmd_output_t;

typedef struct {
    char url[128];
} app_cmd_ota_t;

typedef struct {
    float value_1;
    float value_2;
} app_sensor_pair_t;

typedef struct {
    char topic[128];
    char payload[256];
} app_mqtt_rx_data_t;

#ifdef __cplusplus
}
#endif