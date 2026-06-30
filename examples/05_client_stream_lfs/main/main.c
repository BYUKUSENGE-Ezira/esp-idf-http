#include "app_wifi.h"
#include "app_lfs.h"
#include "app_http_client.h"
#include "app_config.h"

#include "esp_log.h"
#include "esp_err.h"

static const char *TAG = "APP_MAIN";

void app_main(void)
{
    ESP_LOGI(TAG, "Starting 05_client_stream_lfs");

    ESP_ERROR_CHECK(app_lfs_mount());

    ESP_ERROR_CHECK(app_wifi_connect());

    esp_err_t err = app_http_client_stream_to_lfs();

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Download saved successfully");
        ESP_LOGI(TAG, "Reading file back from LittleFS");

        ESP_ERROR_CHECK(app_lfs_read_file(APP_LFS_DOWNLOAD_FILE));
    } else {
        ESP_LOGE(TAG, "Download failed");
    }

    app_lfs_unmount();
}