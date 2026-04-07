#include "freertos/FreeRTOS.h" // <--- DEBE SER EL PRIMERO
#include "freertos/task.h"
#include "freertos/queue.h"

#include "uart_drv.h"

#include "driver/uart.h"
#include "esp_err.h"
#include "esp_log.h"

#define TAG "UART_DRV"

esp_err_t uart_drv_init(uart_port_t uart_num, uint32_t baud_rate, int rx_pin, int tx_pin, int buf_size);

esp_err_t uart_drv_init(uart_port_t uart_num, uint32_t baud_rate, int rx_pin, int tx_pin, int buf_size){
    uart_config_t uart_config = {
        .baud_rate = baud_rate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        };
    
    esp_err_t err = uart_param_config(uart_num, &uart_config);
    if(err != ESP_OK){
        ESP_LOGI(TAG, "PARAM CONFIG UART DRV FAILED: %s", esp_err_to_name(err));;
        return ESP_FAIL;
    }
    err = uart_driver_install(uart_num, buf_size * 2, buf_size * 2, 0, NULL, 0);
    if(err != ESP_OK){
        ESP_LOGI(TAG, "INSTALL UART DRV FAILED: %s", esp_err_to_name(err));
        return ESP_FAIL;
    }
    return ESP_OK; 
}

bool send_data_uart(uart_port_t uart_num, const uint8_t *data, size_t len){
    if (data == NULL || len == 0) {
        ESP_LOGE(TAG, "Invalid TX data");
        return false;
    }
    uart_flush_input(uart_num);
    int written = uart_write_bytes(uart_num, (const char *)data, len);
    if (written < 0) {
        ESP_LOGE(TAG, "uart_write_bytes failed");
        return false;
    }
    esp_err_t err = uart_wait_tx_done(uart_num, pdMS_TO_TICKS(100));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "uart_wait_tx_done failed: %s", esp_err_to_name(err));
        return false;
    }
    return true;
}

int read_data_uart(uart_port_t uart_num, uint8_t *buffer, int max_length, int timeout_ms){
    int len = uart_read_bytes(uart_num, buffer, max_length, pdMS_TO_TICKS(timeout_ms));
    if(len < 0){
        ESP_LOGE(TAG, "uart_read_bytes failed");
        return -1;
    }
    return len;
}
