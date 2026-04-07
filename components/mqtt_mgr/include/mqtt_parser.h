#pragma once

#include <stdbool.h>
#include <stddef.h>
#include "app_event_data.h"

#ifdef __cplusplus
extern "C" {
#endif

bool mqtt_parser_parse_output_command(const char *topic,
                                      const char *payload,
                                      app_cmd_output_t *out_cmd);

#ifdef __cplusplus
}
#endif