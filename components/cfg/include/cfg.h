#pragma once
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* tamaños */
#define CFG_WIFI_SSID_MAX   33
#define CFG_WIFI_PASS_MAX   65
#define CFG_MQTT_HOST_MAX   64
#define CFG_MQTT_USER_MAX   33
#define CFG_MQTT_PASS_MAX   65

#define CFG_DEVICE_ID_MAX   32
#define CFG_IP_STR_MAX      16

#define CFG_WEB_USER_MAX    32
#define CFG_WEB_PASS_MAX    16

#define CFG_LATITUDE    20   
#define CFG_LONGITUDE   20   

#define CFG_WIFI_BT_MAC 18

#define CFG_USER 10
#define CFG_PASS 10


#define CFG_USER 10
#define CFG_PASS 10

#define CFG_BLOB_VERSION    "1.0.1"

typedef enum {
    ULTRASONIC_SENSOR,
    ELECTRICITY_SENSOR
} sensor_selector_t;

typedef struct {
    int max_temp;
    int min_temp;
    int max_hum;
    int min_hum;
} temperature_cfg_t;

typedef struct {
    /* Identidad */
    char device_id[CFG_DEVICE_ID_MAX];

    /* WiFi STA */
    char wifi_ssid[CFG_WIFI_SSID_MAX];
    char wifi_pass[CFG_WIFI_PASS_MAX];

    /* MQTT */
    char mqtt_host[CFG_MQTT_HOST_MAX];
    int  mqtt_port;
    char mqtt_user[CFG_MQTT_USER_MAX];
    char mqtt_pass[CFG_MQTT_PASS_MAX];

    /* DHCP */
    char ip_dhcp[CFG_IP_STR_MAX];

    /* Web login */
    char web_user[CFG_WEB_USER_MAX];
    char web_pass[CFG_WEB_PASS_MAX];

    /* MAC */
    char wifi_mac[CFG_WIFI_BT_MAC];

    char login_user[CFG_USER];
    char login_pass[CFG_PASS];

    char longitude[CFG_LONGITUDE];
    char latitude[CFG_LATITUDE];

    /* Config extra */
    temperature_cfg_t temperature_cfg;
    sensor_selector_t sensor_selector;
    uint32_t telemetry_interval_s;
    char cfg_version[12];
    uint64_t datetime;
    uint64_t orometer_;
} app_cfg_t;

/* =========================
 * API actual
 * ========================= */
bool cfg_init(void);
bool cfg_load(app_cfg_t *out);
bool cfg_save(const app_cfg_t *cfg);

void cfg_ensure_device_id(app_cfg_t *cfg);
void cfg_ensure_web_credentials(app_cfg_t *cfg);
void cfg_ensure_telemetry_interval(app_cfg_t *cfg);
void cfg_ensure_login_credentials(app_cfg_t *cfg);
void cfg_ensure_coordinates(app_cfg_t *cfg);

bool cfg_has_wifi_sta(const app_cfg_t *cfg);
bool cfg_has_mqtt(const app_cfg_t *cfg);

bool cfg_set_ip_dhcp_and_save(app_cfg_t *cfg, const char *ip_str);
bool cfg_set_bt_and_wifi_mac(app_cfg_t *cfg);

void cfg_print_boot(const app_cfg_t *cfg, bool show_password);

/* =========================
 * Cache interno / getters nuevos
 * ========================= */

/* Devuelve puntero al cfg activo cacheado internamente.
 * Puede ser NULL si aún no se ha cargado nada.
 */
const app_cfg_t *cfg_get_active(void);

/* Copia el cfg activo al buffer de salida. */
bool cfg_get_active_copy(app_cfg_t *out);

/* Reemplaza el cfg activo en memoria (sin guardar en NVS). */
bool cfg_set_active(const app_cfg_t *cfg);

/* Guarda el cfg activo cacheado en NVS. */
bool cfg_save_active(void);

/* Getters simples */
const char *cfg_get_device_id(void);

const char *cfg_get_wifi_ssid(void);
const char *cfg_get_wifi_pass(void);

const char *cfg_get_mqtt_host(void);
int         cfg_get_mqtt_port(void);
const char *cfg_get_mqtt_user(void);
const char *cfg_get_mqtt_pass(void);

const char *cfg_get_ip_dhcp(void);

const char *cfg_get_web_user(void);
const char *cfg_get_web_pass(void);

const char *cfg_get_wifi_mac(void);

uint32_t cfg_get_telemetry_interval_s(void);
sensor_selector_t cfg_get_sensor_selector(void);

#ifdef __cplusplus
}
#endif