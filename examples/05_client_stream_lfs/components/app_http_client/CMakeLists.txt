idf_component_register(
    SRCS "src/app_http_client.c"
    INCLUDE_DIRS "include"

    REQUIRES
        app_config
        esp_http_client
)