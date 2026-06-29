#include "sdkconfig.h"

#include "app_wifi.h"
#include "app_config.h"

#include <string.h>

#include "esp_log.h"
#include "esp_err.h"

#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

#define WIFI_MAX_RETRY 5
#define WIFI_WAIT_MS   20000

static const char *TAG = "APP_WIFI";

static EventGroupHandle_t s_wifi_event_group;
static int s_retry_count = 0;
static esp_netif_t *s_wifi_netif = NULL;

static void app_wifi_event_handler(
    void *arg,
    esp_event_base_t event_base,
    int32_t event_id,
    void *event_data
)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "Wi-Fi station started");
        ESP_LOGI(TAG, "Connecting to SSID: %s", APP_WIFI_SSID);
        esp_wifi_connect();
    }

    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_count < WIFI_MAX_RETRY) {
            s_retry_count++;

            ESP_LOGW(
                TAG,
                "Disconnected. Retry %d/%d",
                s_retry_count,
                WIFI_MAX_RETRY
            );

            esp_wifi_connect();
        } else {
            ESP_LOGE(TAG, "Wi-Fi failed after max retries");
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
    }

    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;

        ESP_LOGI(TAG, "Wi-Fi connected");
        ESP_LOGI(TAG, "IP address: " IPSTR, IP2STR(&event->ip_info.ip));

        /*
            Fix DNS problem:
            getaddrinfo() returns 202
        */
        esp_netif_dns_info_t dns = {0};

        dns.ip.type = ESP_IPADDR_TYPE_V4;
        esp_netif_set_ip4_addr(&dns.ip.u_addr.ip4, 8, 8, 8, 8);

        ESP_ERROR_CHECK(
            esp_netif_set_dns_info(
                s_wifi_netif,
                ESP_NETIF_DNS_FALLBACK,
                &dns
            )
        );

        ESP_LOGI(TAG, "Fallback DNS set to 8.8.8.8");

        s_retry_count = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

esp_err_t app_wifi_connect(void)
{
    ESP_LOGI(TAG, "Initializing Wi-Fi");

    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS problem found. Erasing NVS...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);

    s_wifi_event_group = xEventGroupCreate();

    if (s_wifi_event_group == NULL) {
        ESP_LOGE(TAG, "Failed to create Wi-Fi event group");
        return ESP_FAIL;
    }

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    s_wifi_netif = esp_netif_create_default_wifi_sta();

    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));

    ESP_ERROR_CHECK(
        esp_event_handler_register(
            WIFI_EVENT,
            ESP_EVENT_ANY_ID,
            &app_wifi_event_handler,
            NULL
        )
    );

    ESP_ERROR_CHECK(
        esp_event_handler_register(
            IP_EVENT,
            IP_EVENT_STA_GOT_IP,
            &app_wifi_event_handler,
            NULL
        )
    );

    wifi_config_t wifi_config = {0};

    strncpy(
        (char *) wifi_config.sta.ssid,
        APP_WIFI_SSID,
        sizeof(wifi_config.sta.ssid) - 1
    );

    strncpy(
        (char *) wifi_config.sta.password,
        APP_WIFI_PASS,
        sizeof(wifi_config.sta.password) - 1
    );

    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    /*
        Optional but good for HTTP tests.
        It keeps Wi-Fi awake.
    */
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));

    EventBits_t bits = xEventGroupWaitBits(
        s_wifi_event_group,
        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
        pdFALSE,
        pdFALSE,
        pdMS_TO_TICKS(WIFI_WAIT_MS)
    );

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Wi-Fi ready for HTTP");
        return ESP_OK;
    }

    if (bits & WIFI_FAIL_BIT) {
        ESP_LOGE(TAG, "Wi-Fi failed");
        return ESP_FAIL;
    }

    ESP_LOGE(TAG, "Wi-Fi connection timeout");
    return ESP_ERR_TIMEOUT;
}