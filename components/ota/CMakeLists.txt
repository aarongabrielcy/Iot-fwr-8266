idf_component_register(
    SRCS "ota_fw.c"
    INCLUDE_DIRS "include"
    REQUIRES nvs_flash esp_http_client esp_https_ota esp-tls
)
