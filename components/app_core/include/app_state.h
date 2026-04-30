#pragma once

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef struct{
    char device_id[16];
    char fw_version[10];
    char latitude[20];
    char longitude[20];
} GeneralState;

typedef enum {
    APP_MODE_NONE = 0,
    APP_MODE_STA,
    APP_MODE_APSTA,
    APP_MODE_CONFIG
} app_mode_t;

void app_state_init(void);

/* WiFi */
void app_state_set_wifi_connected(bool connected);
bool app_state_get_wifi_connected(void);

/* MQTT */
void app_state_set_mqtt_connected(bool connected);
bool app_state_get_mqtt_connected(void);

/* Time sync */
void app_state_set_time_synced(bool synced);
bool app_state_get_time_synced(void);

/* Mode */
void app_state_set_mode(app_mode_t mode);
app_mode_t app_state_get_mode(void);

/* Config mode */
void app_state_set_config_mode(bool enabled);
bool app_state_get_config_mode(void);

/* Scheduler */
void app_state_set_scheduler_started(bool started);
bool app_state_get_scheduler_started(void);

/* IP address */
void app_state_set_ip_addr(const char *ip_addr);
const char *app_state_get_ip_addr(void);

/* general */
void app_state_set_device_id(const char *device_id);
void app_state_set_fw_version(const char *fw_version);
void app_state_set_latitude(const char *latitude);
void app_state_set_longitude(const char *longitude);

void app_state_get_latitude(char *latitude, size_t len);
void app_state_get_longitude(char *longitude, size_t len);

#ifdef __cplusplus
}
#endif