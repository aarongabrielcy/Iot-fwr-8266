#pragma once

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

bool mqtt_topics_build_cmd(char *buf, size_t len);
bool mqtt_topics_build_status(char *buf, size_t len);
bool mqtt_topics_build_telemetry(char *buf, size_t len);
bool mqtt_topics_build_ack(char *buf, size_t len);

#ifdef __cplusplus
}
#endif