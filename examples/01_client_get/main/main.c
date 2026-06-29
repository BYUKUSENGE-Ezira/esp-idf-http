#include "esp_log.h"
#include "esp_err.h"

#include "app_wifi.h"
#include "app_http_client.h"

static const char *TAG = "MAIN";

void app_main(void)
{
    ESP_LOGI(TAG, "Example 01: HTTP Client GET");

   
    esp_err_t wifi_result = app_wifi_connect();

    if (wifi_result != ESP_OK) {
        ESP_LOGE(TAG, "Wi-Fi failed: %s", esp_err_to_name(wifi_result));
        return;
    }

    esp_err_t http_result = app_http_client_get();

    if (http_result == ESP_OK) {
        ESP_LOGI(TAG, "HTTP GET example finished successfully");
    } else {
        ESP_LOGE(TAG, "HTTP GET example failed: %s", esp_err_to_name(http_result));
    }
}