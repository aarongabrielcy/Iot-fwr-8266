#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RECONNECTION_TIME  600

bool scheduler_init(void);
bool scheduler_start(void);
bool scheduler_stop(void);
bool scheduler_reload(void);

void start_reconnection_timer(void);
void stop_reconnection_timer(void);


#ifdef __cplusplus
}
#endif