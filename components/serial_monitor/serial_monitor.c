#include "serial_monitor.h"
#include "uart_drv.h"
#include "ota_fw.h"


#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "string.h"
#include "esp_system.h"

#define BUF_SIZE 1024
#define BAUD_RATE 115200

#define TAG "SERIAL_MONITOR"

TaskHandle_t monitor_uart_task = NULL;

static void serial_task_init();
static void serialConsole_task(void *pvParameters);

void serial_monitor_init(void){
    esp_err_t err = uart_drv_init(MONITOR_SERIAL, BAUD_RATE, 0, 0, BUF_SIZE);
    if(err != ESP_OK){
        ESP_LOGI(TAG, "SERIAL MONITOR INIT FAILED: %s", esp_err_to_name(err));
        return;
    }
    ESP_LOGI(TAG, "SERIAL MONITOR INIT");
    serial_task_init();
}

static void serialConsole_task(void *pvParameters){
    char data[512];
    while(1){
        int len = uart_read_bytes(UART_NUM_0, (uint8_t *)data, sizeof(data) - 1, pdMS_TO_TICKS(300));
        if (len > 0) {
            data[len] = '\0';
            if(strstr(data, "REBOOT") != NULL){
                ESP_LOGI(TAG, "REBOOTING...");
                esp_restart();
            }
            if(strstr(data, "OTA=") != NULL){
                ESP_LOGI(TAG, "UPDATING...");
                char *start_data_url = strstr(data, "OTA=");
                start_data_url += strlen("OTA=");
                ota_update(start_data_url);
            }
            if(strstr(data, "WINDOW=1") != NULL){
            }
            if(strstr(data, "WINDOW=2") != NULL){
            }
        }
    }
}

static void serial_task_init(){
    xTaskCreate(serialConsole_task, "serial_console_task", 8192, NULL, 5, &monitor_uart_task);
}