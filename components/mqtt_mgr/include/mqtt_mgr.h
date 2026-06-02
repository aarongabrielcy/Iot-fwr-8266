#pragma once

#include <stdbool.h>
#include "cfg.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum{
    OUTPUT_1 = 101,
    OUTPUT_2 = 102,
    OTA = 103,
    REBOOT = 104,
    TRK = 105
};

bool mqtt_mgr_init(void);
bool mqtt_mgr_start(app_cfg_t *cfg);
bool mqtt_mgr_stop(void);

bool mqtt_mgr_is_connected(void);

bool mqtt_mgr_publish_telemetry(const char *payload);
bool mqtt_mgr_publish_status(const char *payload);
bool mqtt_mgr_publish_ack(const char *payload);

#ifdef __cplusplus
}
#endif