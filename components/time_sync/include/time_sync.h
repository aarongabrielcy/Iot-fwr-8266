#pragma once
// Esta cabecera va en Tracking, uart 00, app_controller
#include "stdbool.h"
#include "stdint.h"
#include "time.h"

void timestamp_sync_init();
void get_time();
