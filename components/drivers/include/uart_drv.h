#pragma once
#include "esp_err.h"
#include "stdbool.h"
#include "freertos/FreeRTOS.h" // <--- DEBE SER EL PRIMERO
#include "driver/uart.h"
#define MONITOR_SERIAL UART_NUM_0
#define SENSOR_RS485_1 UART_NUM_1 

esp_err_t uart_drv_init(uart_port_t uart_num, uint32_t baud_rate, int rx_pin, int tx_pin, int buf_size);
bool send_data_uart(uart_port_t uart_num, const uint8_t *data, size_t len);
int read_data_uart(uart_port_t uart_num, uint8_t *buffer, int max_length, int timeout_ms);