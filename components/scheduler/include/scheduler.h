#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

bool scheduler_init(void);
bool scheduler_start(void);
bool scheduler_stop(void);
bool scheduler_reload(void);

#ifdef __cplusplus
}
#endif