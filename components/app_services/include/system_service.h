#pragma once

#include <stdint.h>
#include "cfg.h"

#ifdef __cplusplus
extern "C" {
#endif

void system_service_handle(const app_cfg_t *cfg, int32_t event_id, void *event_data);

#ifdef __cplusplus
}
#endif