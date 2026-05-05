#include "scheduler.h"

#include "app_events.h"
#include "app_events_ids.h"
#include "cfg.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "SCHEDULER";

static esp_timer_handle_t s_tracking_timer = NULL;
static bool s_scheduler_initialized = false;
static bool s_scheduler_started = false;
static esp_timer_handle_t reconnection_timer = NULL;
static uint64_t reconnection_interval = RECONNECTION_TIME * 1000000;

static void tracking_timer_cb(void *arg)
{
    (void)arg;
    app_post_report_event(APP_RPT_TRACKING, NULL, 0);
}

static void reconnection_timer_cb(void *arg)
{
    app_post_system_event(WIFI_RECONNECTION_TRY, NULL, 0);
}

static bool scheduler_create_tracking_timer(void)
{
    if (s_tracking_timer != NULL) {
        return true;
    }

    const esp_timer_create_args_t timer_args = {
        .callback = &tracking_timer_cb,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "tracking_tmr"
    };

    esp_err_t err = esp_timer_create(&timer_args, &s_tracking_timer);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create tracking timer: %s", esp_err_to_name(err));
        s_tracking_timer = NULL;
        return false;
    }

    return true;
}

void start_reconnection_timer(void)
{
    if (reconnection_timer != NULL) {
        ESP_LOGW(TAG, "reconnection_timer ya está en ejecución");
        return;
    }

    const esp_timer_create_args_t timer_args = {
        .callback = &reconnection_timer_cb,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "reconnection_tmr"
    };

    esp_err_t err = esp_timer_create(&timer_args, &reconnection_timer);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create reconnection timer: %s", esp_err_to_name(err));
        reconnection_timer = NULL;
        return;
    }

    err = esp_timer_start_periodic(reconnection_timer, reconnection_interval);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start reconnection timer: %s", esp_err_to_name(err));
        esp_timer_delete(reconnection_timer);
        reconnection_timer = NULL;
    }
}

void stop_reconnection_timer(void)
{
    if (reconnection_timer == NULL) {
        ESP_LOGW(TAG, "reconnection_timer no está en ejecución");
        return;
    }

    esp_timer_stop(reconnection_timer);
    esp_timer_delete(reconnection_timer);
    reconnection_timer = NULL;
}

bool scheduler_init(void)
{
    if (s_scheduler_initialized) {
        ESP_LOGW(TAG, "Scheduler already initialized");
        return true;
    }

    if (!scheduler_create_tracking_timer()) {
        return false;
    }

    s_scheduler_initialized = true;
    ESP_LOGI(TAG, "Scheduler initialized");
    return true;
}

bool scheduler_start(void)
{
    if (!s_scheduler_initialized) {
        ESP_LOGE(TAG, "Scheduler not initialized");
        return false;
    }

    if (s_scheduler_started) {
        ESP_LOGW(TAG, "Scheduler already started");
        return true;
    }

    uint32_t tracking_interval_s = cfg_get_telemetry_interval_s();
    if (tracking_interval_s == 0U) {
        tracking_interval_s = 10U;
    }

    esp_err_t err = esp_timer_start_periodic(
        s_tracking_timer,
        (uint64_t)tracking_interval_s * 1000000ULL
    );
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start tracking timer: %s", esp_err_to_name(err));
        return false;
    }

    s_scheduler_started = true;

    ESP_LOGI(TAG, "Scheduler started: tracking_interval=%u s", (unsigned)tracking_interval_s);
    return true;
}

bool scheduler_stop(void)
{
    if (!s_scheduler_initialized) {
        ESP_LOGW(TAG, "Scheduler not initialized");
        return false;
    }

    if (!s_scheduler_started) {
        ESP_LOGW(TAG, "Scheduler not started");
        return true;
    }

    if (s_tracking_timer != NULL) {
        esp_timer_stop(s_tracking_timer);
    }

    s_scheduler_started = false;
    ESP_LOGI(TAG, "Scheduler stopped");
    return true;
}