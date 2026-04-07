#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "time_sync.h"
//#include "app_state.h"
#include "esp_sntp.h"
#include "esp_timer.h"
#include "esp_log.h"

#define TAG "TIMESTAMP"

static TaskHandle_t timestamp_sync_handle = NULL;
static time_t now = 0;
static struct tm timeinfo = {0};

// Prototipos locales
static void timestamp_sync_task(void *pvParameters);
static void print_time();

void timestamp_sync_init() {
    if (timestamp_sync_handle == NULL) {
        // En ESP8266, 2048 o 3072 es usualmente suficiente para SNTP
        xTaskCreate(timestamp_sync_task, "ts_sync_task", 3072, NULL, 5, &timestamp_sync_handle);
    }
}

static void timestamp_sync_task(void *pvParameters) {
    int retry = 0;
    const int retry_count = 20;

    ESP_LOGI(TAG, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();

    while (timeinfo.tm_year < (2020 - 1900) && ++retry < retry_count) {
        ESP_LOGI(TAG, "Waiting for system time... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        time(&now);
        localtime_r(&now, &timeinfo);
    }

    if (timeinfo.tm_year + 1900 < 2020) {
        ESP_LOGW(TAG, "System time NOT set");
        //app_state_set_time_synced(false);
    } else {
        ESP_LOGI(TAG, "System time set successfully");
        //app_state_set_time_synced(true);
        //app_state_set_last_sync_epoch_time((uint64_t)now);
        print_time();
    }

    // Limpieza CRÍTICA
    timestamp_sync_handle = NULL; 
    vTaskDelete(NULL); // La tarea se elimina a sí misma de forma segura
}

void get_time() {
    // Si ya está sincronizado, actualizamos estados
    //if (app_state_get_time_synced()) {
    if (true) {
        time(&now);
        //app_state_set_last_sync_epoch_time((uint64_t)now);
        
        // El horómetro en segundos
        uint32_t orometer = (uint32_t)(esp_timer_get_time() / 1000000);
        //app_state_set_orometer_seconds(orometer);
    } else {
        // Si no está sincronizado y la tarea no está corriendo, la lanzamos
        timestamp_sync_init();
    }
}

static void print_time() {
    time(&now);
    localtime_r(&now, &timeinfo);
    ESP_LOGI(TAG, "Current time: %04d-%02d-%02d %02d:%02d:%02d",
             timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
             timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
}