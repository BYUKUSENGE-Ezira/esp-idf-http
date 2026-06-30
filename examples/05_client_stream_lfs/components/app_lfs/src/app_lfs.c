#include "app_lfs.h"
#include "app_config.h"

#include <stdio.h>

#include "esp_log.h"
#include "esp_littlefs.h"

static const char *TAG = "APP_LFS";

esp_err_t app_lfs_mount(void)
{
    esp_vfs_littlefs_conf_t conf = {
        .base_path = APP_LFS_BASE_PATH,
        .partition_label = APP_LFS_PARTITION_LABEL,
        .format_if_mount_failed = true,
        .dont_mount = false,
    };

    esp_err_t err = esp_vfs_littlefs_register(&conf);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount LittleFS: %s", esp_err_to_name(err));
        return err;
    }

    size_t total = 0;
    size_t used = 0;

    err = esp_littlefs_info(APP_LFS_PARTITION_LABEL, &total, &used);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "LittleFS mounted");
        ESP_LOGI(TAG, "Total: %d bytes, Used: %d bytes", total, used);
    } else {
        ESP_LOGW(TAG, "Failed to get LittleFS info: %s", esp_err_to_name(err));
    }

    return ESP_OK;
}

void app_lfs_unmount(void)
{
    esp_vfs_littlefs_unregister(APP_LFS_PARTITION_LABEL);
    ESP_LOGI(TAG, "LittleFS unmounted");
}

esp_err_t app_lfs_read_file(const char *path)
{
    FILE *file = fopen(path, "rb");

    if (file == NULL) {
        ESP_LOGE(TAG, "Failed to open file for reading: %s", path);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Reading saved file: %s", path);

    char buffer[128];
    size_t bytes_read = 0;

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        printf("%.*s", (int)bytes_read, buffer);
    }

    printf("\n");

    fclose(file);
    return ESP_OK;
}