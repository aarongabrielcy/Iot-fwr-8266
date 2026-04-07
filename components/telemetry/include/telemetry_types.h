#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    TELEMETRY_KIND_UNKNOWN = 0,
    TELEMETRY_KIND_TRACKING,
    TELEMETRY_KIND_BOOT,
    TELEMETRY_KIND_OUTPUT_STATUS,
    TELEMETRY_KIND_INPUT_STATUS,
    TELEMETRY_KIND_SENSOR_STATUS,
    TELEMETRY_KIND_ALERT
} telemetry_kind_t;

typedef struct {
    telemetry_kind_t kind;
    const char *type_str;
} telemetry_kind_info_t;

#ifdef __cplusplus
}
#endif