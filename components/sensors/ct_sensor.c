#include "ct_sensor.h"

#include "adc_drv.h"
#include "app_events.h"
#include "app_events_ids.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#define TAG "CT_SENSOR"

/*
 * Ajusta estos defines según tu hardware real.
 * Esta versión es básica, no RMS real.
 */
#define CT_SENSOR_TASK_STACK          3072
#define CT_SENSOR_TASK_PRIORITY       4
#define CT_SENSOR_POLL_MS             100
#define CT_SENSOR_AVG_SAMPLES         16

/* Umbral alto para disparar alerta */
#define CT_SENSOR_THRESHOLD_RAW       600

/* Histéresis: debe bajar por debajo de esto para "rearmar" */
#define CT_SENSOR_THRESHOLD_CLEAR     560

/* Debounce por muestras consecutivas */
#define CT_SENSOR_TRIGGER_COUNT       3
#define CT_SENSOR_CLEAR_COUNT         3

static TaskHandle_t s_ct_task = NULL;
static bool s_ct_initialized = false;
static bool s_ct_started = false;

static volatile uint16_t s_last_raw = 0;
static volatile bool s_alarm_active = false;

static void ct_sensor_task(void *arg)
{
    (void)arg;

    uint8_t over_count = 0;
    uint8_t clear_count = 0;

    while (1) {
        uint16_t raw_avg = 0;
        if (adc_drv_read_avg_raw(&raw_avg, CT_SENSOR_AVG_SAMPLES) == ESP_OK) {
            s_last_raw = raw_avg;

            if (!s_alarm_active) {
                if (raw_avg >= CT_SENSOR_THRESHOLD_RAW) {
                    if (over_count < 255) {
                        over_count++;
                    }

                    if (over_count >= CT_SENSOR_TRIGGER_COUNT) {
                        s_alarm_active = true;
                        clear_count = 0;
                        ESP_LOGW(TAG, "CT threshold exceeded, raw=%u", (unsigned)raw_avg);

                        /*
                         * Dispara reporte interno; MQTT sale por la tubería normal.
                         */
                        app_post_report_event(APP_RPT_CT_ALERT, NULL, 0);
                    }
                } else {
                    over_count = 0;
                }
            } else {
                if (raw_avg <= CT_SENSOR_THRESHOLD_CLEAR) {
                    if (clear_count < 255) {
                        clear_count++;
                    }

                    if (clear_count >= CT_SENSOR_CLEAR_COUNT) {
                        s_alarm_active = false;
                        over_count = 0;
                        ESP_LOGI(TAG, "CT alarm cleared, raw=%u", (unsigned)raw_avg);
                    }
                } else {
                    clear_count = 0;
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(CT_SENSOR_POLL_MS));
    }
}

bool ct_sensor_init(void)
{
    if (s_ct_initialized) {
        return true;
    }

    if (adc_drv_init() != ESP_OK) {
        ESP_LOGE(TAG, "adc_drv_init failed");
        return false;
    }

    s_last_raw = 0;
    s_alarm_active = false;
    s_ct_initialized = true;

    ESP_LOGI(TAG, "CT sensor initialized");
    return true;
}

bool ct_sensor_start(void)
{
    if (!s_ct_initialized) {
        ESP_LOGE(TAG, "ct_sensor_start called before init");
        return false;
    }

    if (s_ct_started) {
        return true;
    }

    BaseType_t ok = xTaskCreate(
        ct_sensor_task,
        "ct_sensor_task",
        CT_SENSOR_TASK_STACK,
        NULL,
        CT_SENSOR_TASK_PRIORITY,
        &s_ct_task
    );

    if (ok != pdPASS) {
        ESP_LOGE(TAG, "Failed to create ct_sensor_task");
        s_ct_task = NULL;
        return false;
    }

    s_ct_started = true;
    ESP_LOGI(TAG, "CT sensor task started");
    return true;
}

uint16_t ct_sensor_get_last_raw(void)
{
    return s_last_raw;
}

bool ct_sensor_is_alarm_active(void)
{
    return s_alarm_active;
}