#pragma once

#include "esp_event.h"

#ifdef __cplusplus
extern "C" {
#endif

void app_event_loop_init(void);
esp_event_loop_handle_t app_event_loop_get(void);

#ifdef __cplusplus
}
#endif