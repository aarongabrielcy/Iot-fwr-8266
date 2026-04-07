#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

bool telemetry_service_publish_report(int32_t report_event_id, void *event_data);

#ifdef __cplusplus
}
#endif