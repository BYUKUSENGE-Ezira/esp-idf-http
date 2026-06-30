#include "app_http_client.h"
#include "app_config.h"

#include <stdio.h>
#include <inttypes.h>

#include "esp_log.h"
#include "esp_err.h"
#include "esp_http_client.h"

static const char *TAG = "APP_HTTP_CLIENT";

typedef struct {
    FILE *file;
    size_t total_bytes;
} app_http_stream_context_t;

static esp_err_t app_http_event_handler(esp_http_client_event_t *event)
{
    app_http_stream_context_t *stream_ctx =
        (app_http_stream_context_t *)event->user_data;

    switch (event->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGE(TAG, "HTTP_EVENT_ERROR");
            break;

        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;

        case HTTP_EVENT_HEADERS_SENT:
            ESP_LOGI(TAG, "HTTP_EVENT_HEADERS_SENT");
            break;

        case HTTP_EVENT_ON_HEADER:
            ESP_LOGI(TAG, "HEADER: %s: %s", event->header_key, event->header_value);
            break;

        case HTTP_EVENT_ON_DATA:
            if (stream_ctx != NULL &&
                stream_ctx->file != NULL &&
                event->data != NULL &&
                event->data_len > 0) {

                size_t written = fwrite(
                    event->data,
                    1,
                    event->data_len,
                    stream_ctx->file
                );

                if (written != event->data_len) {
                    ESP_LOGE(TAG, "File write failed");
                    return ESP_FAIL;
                }

                stream_ctx->total_bytes += written;

                ESP_LOGI(
                    TAG,
                    "Written chunk: %d bytes, total: %d bytes",
                    event->data_len,
                    (int)stream_ctx->total_bytes
                );
            }
            break;

        case HTTP_EVENT_ON_FINISH:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH");
            break;

        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "HTTP_EVENT_DISCONNECTED");
            break;

        case HTTP_EVENT_REDIRECT:
            ESP_LOGI(TAG, "HTTP_EVENT_REDIRECT");
            break;

        case HTTP_EVENT_ON_STATUS_CODE:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_STATUS_CODE");
            break;

        case HTTP_EVENT_ON_HEADERS_COMPLETE:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_HEADERS_COMPLETE");
            break;

        default:
            break;
    }

    return ESP_OK;
}

esp_err_t app_http_client_stream_to_lfs(void)
{
    app_http_stream_context_t stream_ctx = {
        .file = NULL,
        .total_bytes = 0,
    };

    stream_ctx.file = fopen(APP_LFS_DOWNLOAD_FILE, "wb");

    if (stream_ctx.file == NULL) {
        ESP_LOGE(
            TAG,
            "Failed to open file for writing: %s",
            APP_LFS_DOWNLOAD_FILE
        );

        return ESP_FAIL;
    }

    esp_http_client_config_t config = {
        .url = APP_HTTP_STREAM_URL,
        .method = HTTP_METHOD_GET,
        .timeout_ms = APP_HTTP_TIMEOUT_MS,
        .event_handler = app_http_event_handler,
        .user_data = &stream_ctx,
        .keep_alive_enable = false,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);

    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to create HTTP client");
        fclose(stream_ctx.file);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Downloading from: %s", APP_HTTP_STREAM_URL);
    ESP_LOGI(TAG, "Saving to: %s", APP_LFS_DOWNLOAD_FILE);

    esp_err_t err = esp_http_client_perform(client);

    fclose(stream_ctx.file);
    stream_ctx.file = NULL;

    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        int64_t content_length = esp_http_client_get_content_length(client);

        ESP_LOGI(TAG, "HTTP status code: %d", status_code);
        ESP_LOGI(TAG, "HTTP content length: %" PRId64, content_length);
        ESP_LOGI(TAG, "Total bytes saved: %d", (int)stream_ctx.total_bytes);

        if (status_code < 200 || status_code >= 300) {
            ESP_LOGW(TAG, "Server returned error status");
            err = ESP_FAIL;
        }
    } else {
        ESP_LOGE(TAG, "HTTP download failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    ESP_LOGI(TAG, "HTTP client cleaned up");

    return err;
}