idf_component_register(
    SRCS "src/app_wifi.c"
    INCLUDE_DIRS "include"
    REQUIRES
        app_config
        esp_wifi
        esp_netif
        nvs_flash
)