# components/web_cfg/CMakeLists.txt
idf_component_register(
    SRCS
        "web_cfg.c"
        "web_cfg_handlers.c"
    INCLUDE_DIRS
        "include"
    REQUIRES
        cfg
        esp_wifi
        esp_netif
        esp_event
        esp_http_server
        nvs_flash
        app_core
    WHOLE_ARCHIVE
)

# Embebidos (genera _binary_<path>_start/_end)
target_add_binary_data(${COMPONENT_LIB} "web_cfg_ui.html" TEXT)
target_add_binary_data(${COMPONENT_LIB} "login.html" TEXT)
target_add_binary_data(${COMPONENT_LIB} "assets/favicon.svg" BINARY)
target_add_binary_data(${COMPONENT_LIB} "assets/iot_logo.svg" BINARY)

