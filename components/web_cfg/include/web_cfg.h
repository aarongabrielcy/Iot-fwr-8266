#pragma once
#include <stdbool.h>
#include "cfg.h"

#ifdef __cplusplus
extern "C" {
#endif

bool web_cfg_start(app_cfg_t *cfg);
void web_cfg_stop(void);

#ifdef __cplusplus
}
#endif