#pragma once

#include "esp_event.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

ESP_EVENT_DECLARE_BASE(APP_SYSTEM_EVENTS);
ESP_EVENT_DECLARE_BASE(APP_REPORT_EVENTS);
ESP_EVENT_DECLARE_BASE(APP_SENSOR_EVENTS);
ESP_EVENT_DECLARE_BASE(APP_COMMAND_EVENTS);

/* =========================
 * SYSTEM EVENTS
 * ========================= */
typedef enum {
    APP_SYS_WIFI_CONNECTED = 1,
    APP_SYS_WIFI_DISCONNECTED,
    APP_SYS_MQTT_CONNECTED,
    APP_SYS_MQTT_DISCONNECTED,
    WIFI_RECONNECTION_CFG,
    WIFI_RECONNECTION_TRY,
    WIFI_RECONNECTION_STOP,
    APP_SYS_TIME_SYNCED
} app_system_event_id_t;

/* =========================
 * REPORT EVENTS
 * ========================= */
typedef enum {
    APP_RPT_TRACKING = 0,
    APP_RPT_BOOT,
    APP_RPT_OUTPUT_STATUS,
    APP_RPT_INPUT_STATUS,
    APP_RPT_SENSOR_STATUS,
    APP_RPT_ALERT,
    APP_RPT_CT_ALERT
} app_report_event_id_t;

/* =========================
 * SENSOR EVENTS
 * ========================= */
typedef enum {
    APP_SENSOR_RESERVED = 1
} app_sensor_event_id_t;

/* =========================
 * COMMAND EVENTS
 * ========================= */
typedef enum {
    APP_CMD_SET_OUTPUT = 1,
    APP_CMD_REBOOT,
    APP_CMD_REQUEST_REPORT,
    APP_CMD_START_OTA
} app_command_event_id_t;

#ifdef __cplusplus
}
#endif