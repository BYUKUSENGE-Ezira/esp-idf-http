#include "app_http_client.h"
#include "app_config.h"
#include "app_wifi.h"

#include <inttypes.h>

#include "esp_log.h"
#include "esp_err.h"
#include "esp_http_client.h"

// OPTIMIZATION: Made static const to prevent cross-module link warnings
static const char* TAG = "APP_HTTP_CLIENT";

static esp_err_t app_http_event_handler(esp_http_client_event_t *event){
    switch(event->event_id){
        case HTTP_EVENT_ERROR:
            ESP_LOGE(TAG,"HTTP_EVENT_ERROR");
            break;

        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGI(TAG,"HTTP_ON_CONNECTED");
            break;

        case HTTP_EVENT_HEADERS_SENT:
            ESP_LOGI(TAG,"HTTP_EVENT_HEADERS_SENT");
            break;

        case HTTP_EVENT_ON_HEADER:
            ESP_LOGI(TAG,"HTTP_EVENT_ON_HEADER");
            break;

        case HTTP_EVENT_ON_DATA:
            ESP_LOGI(TAG,"HTTP_EVENT_ON_DATA");

            if(event->data != NULL && event->data_len > 0){
                printf("%.*s",event->data_len, (char*)event->data);
            }
            break;

        case HTTP_EVENT_ON_FINISH:
            ESP_LOGI(TAG,"HTTP_EVENT_ON_FINISH");
            break;
            
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGW(TAG,"HTTP_EVENT_DISCONNECTED");
            break;

        case HTTP_EVENT_REDIRECT:
            // FIX 2: Corrected copy-paste log description text
            ESP_LOGI(TAG,"HTTP_EVENT_REDIRECT");
            break;
            
        // FIX 1: Added mandatory v6.0.1 enums to stop -Werror=switch compile crashes
        case HTTP_EVENT_ON_STATUS_CODE:
            ESP_LOGI(TAG,"HTTP_EVENT_ON_STATUS_CODE");
            break;

        case HTTP_EVENT_ON_HEADERS_COMPLETE:
            ESP_LOGI(TAG,"HTTP_EVENT_ON_HEADERS_COMPLETE");
            break;

        default:
            break;
    }

    return ESP_OK;
}

esp_err_t app_http_client_get(void){
    esp_http_client_config_t config = {
        .url = APP_HTTP_GET_URL,
        .method = HTTP_METHOD_GET,
        .timeout_ms = 15000,
        .event_handler = app_http_event_handler,
        .keep_alive_enable = false
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);

    if (client == NULL){
        ESP_LOGE(TAG, "Failed to create http client");
        return ESP_FAIL;
    }

    esp_err_t err = esp_http_client_perform(client);

    if(err == ESP_OK){
        int status_code = esp_http_client_get_status_code(client);
        int64_t content_length = esp_http_client_get_content_length(client);

        ESP_LOGI(TAG, "HTTP status code: %d", status_code);
        ESP_LOGI(TAG,"HTTP Content length: %" PRId64, content_length);

        if(status_code < 200 || status_code >= 300){
            ESP_LOGW(TAG,"Server return error somehow");
            err = ESP_FAIL;
        }
    }
    else{
            ESP_LOGE(TAG,"Failed");
    }

    esp_http_client_cleanup(client);
    ESP_LOGI(TAG,"Cleaned up");

    return err;
}
