#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void sensor_service_handle(int32_t event_id, void *event_data);

#ifdef __cplusplus
}
#endif