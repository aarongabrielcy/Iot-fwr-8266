#pragma once
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*wifi_mgr_cb_t)(void);
typedef void (*wifi_mgr_got_ip_cb_t)(const char *ip_str);

void wifi_mgr_set_on_connected(wifi_mgr_cb_t cb);
void wifi_mgr_set_on_failed(wifi_mgr_cb_t cb);
void wifi_mgr_set_on_got_ip(wifi_mgr_got_ip_cb_t cb);

bool wifi_mgr_start_sta(const char *ssid, const char *pass);
bool wifi_mgr_start_ap(const char *ap_ssid, const char *ap_pass);
bool wifi_mgr_try_sta_reconnect(const char *ssid, const char *pass);

/**
 * Inicia modo AP+STA:
 * - AP levanta para servir Web UI (192.168.4.1)
 * - STA queda lista para escanear y/o conectarse a la red que se guarde en NVS
 *
 * Importante: aquí NO conectamos a ninguna red automáticamente, salvo que luego
 * tú invoques wifi_mgr_start_sta o tu lógica de configuración lo haga.
 */
bool wifi_mgr_start_apsta(const char *ap_ssid, const char *ap_pass);

void get_rssi(void);

bool wifi_mgr_is_sta_connected(void);

#ifdef __cplusplus
}
#endif