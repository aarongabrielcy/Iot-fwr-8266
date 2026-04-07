COMPONENT_ADD_INCLUDEDIRS := include
# Añadimos la raíz del componente para que encuentre los archivos locales
COMPONENT_SRCDIRS := .

COMPONENT_DEPENDS := cfg esp8266 tcpip_adapter esp_event esp_http_server nvs_flash app_core scheduler

# Embebidos
COMPONENT_EMBED_TXTFILES := web_cfg_ui.html
