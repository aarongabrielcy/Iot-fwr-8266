#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

bool telemetry_builder_build_tracking(char *buf, size_t len);
bool telemetry_builder_build_boot(char *buf, size_t len);
bool telemetry_builder_build_output_status(char *buf, size_t len);
bool telemetry_builder_build_input_status(char *buf, size_t len);
bool telemetry_builder_build_sensor_status(char *buf, size_t len);
bool telemetry_builder_build_alert(char *buf, size_t len);

#ifdef __cplusplus
}
#endif