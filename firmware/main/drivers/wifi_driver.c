#include "wifi_driver.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_mac.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "mdns.h"

#include "lwip/err.h"
#include "lwip/sys.h"

static const char *TAG = "WIFI_AP";

static int s_retry_num = 0;
static EventGroupHandle_t s_wifi_event_group;
static esp_netif_t *s_wifi_netif = NULL;
static bool mdns_started = false;

static void start_mdns(void)
{
    esp_err_t err = mdns_init();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE)
    {
        ESP_LOGE(TAG, "mDNS init failed: %s", esp_err_to_name(err));
        return;
    }

    mdns_hostname_set("putm_laptimer");
    mdns_instance_name_set("PUTM EV Laptimer");

    mdns_service_add(NULL, "_http", "_tcp", 80, NULL, 0);

    ESP_LOGI(TAG, "mDNS started: http://putm_laptimer.local");
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED)
    {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
        ESP_LOGI(TAG, "Station " MACSTR " joined, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED)
    {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
        ESP_LOGI(TAG, "Station " MACSTR " left, AID=%d, reason:%d",
                 MAC2STR(event->mac), event->aid, event->reason);
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
        ESP_LOGI(TAG, "Station started");
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Got IP:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, BIT0);

        if (!mdns_started)
        {
            start_mdns();
            mdns_started = true;
        }
    }
    // else if (event_base == IP_EVENT && event_id == IP_EVENT_ASSIGNED_IP_TO_CLIENT)
    // {
    //     const ip_event_ap_staipassigned_t *e = (const ip_event_ap_staipassigned_t *)event_data;
    //     ESP_LOGI(TAG, "Assigned IP to client: " IPSTR ", MAC=" MACSTR ", hostname='%s'",
    //              IP2STR(&e->ip), MAC2STR(e->mac));
    // }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        mdns_started = false;
    }
}

esp_err_t wifi_init(wifi_mode_t wifi_mode, char wifi_ssid[WIFI_SSID_STR_LENGTH], char wifi_password[WIFI_PASSWORD_STR_LENGTH])
{
    if (wifi_ssid == NULL || wifi_password == NULL || ((wifi_mode != WIFI_MODE_AP) && (wifi_mode != WIFI_MODE_STA)))
        return ESP_FAIL;

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ret = esp_netif_init();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE)
    {
        ESP_ERROR_CHECK(ret);
    }

    ret = esp_event_loop_create_default();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE)
    {
        ESP_ERROR_CHECK(ret);
    }

    if (!s_wifi_event_group)
    {
        s_wifi_event_group = xEventGroupCreate();
    }

    if (s_wifi_netif)
    {
        esp_netif_destroy_default_wifi(s_wifi_netif);
        s_wifi_netif = NULL;
    }

    switch (wifi_mode)
    {
    case WIFI_MODE_AP:
    {
        s_wifi_netif = esp_netif_create_default_wifi_ap();

        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));

        ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                            ESP_EVENT_ANY_ID,
                                                            &wifi_event_handler,
                                                            NULL,
                                                            NULL));

        wifi_config_t wifi_ap_config = {
            .ap = {
                .ssid_len = strlen(wifi_ssid),
                .channel = 1,
                .max_connection = 3,
                .authmode = WIFI_AUTH_OPEN,
                .pmf_cfg = {
                    .required = false,
                },
            },
        };
        snprintf((char *)wifi_ap_config.ap.ssid, sizeof(wifi_ap_config.sta.ssid), wifi_ssid);

        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_ap_config));
        ESP_ERROR_CHECK(esp_wifi_start());

        ESP_LOGI(TAG, "WIFI AP INIT OK");
        break;
    }
    case WIFI_MODE_STA:
    {
        s_wifi_netif = esp_netif_create_default_wifi_sta();

        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));

        ESP_ERROR_CHECK(esp_event_handler_instance_register(
            WIFI_EVENT,
            ESP_EVENT_ANY_ID,
            &wifi_event_handler,
            NULL,
            NULL));

        ESP_ERROR_CHECK(esp_event_handler_instance_register(
            IP_EVENT,
            ESP_EVENT_ANY_ID,
            &wifi_event_handler,
            NULL,
            NULL));
        wifi_config_t wifi_sta_config = {
            .sta = {
                .scan_method = WIFI_ALL_CHANNEL_SCAN,
                .failure_retry_cnt = 5,
                /* Authmode threshold resets to WPA2 as default if password matches WPA2 standards (password len => 8).
                 * If you want to connect the device to deprecated WEP/WPA networks, Please set the threshold value
                 * to WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK and set the password with length and format matching to
                 * WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK standards.
                 */
                .threshold.authmode = WIFI_AUTH_WPA2_PSK,
                .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
            },
        };
        snprintf((char *)wifi_sta_config.sta.ssid, sizeof(wifi_sta_config.sta.ssid), wifi_ssid);
        snprintf((char *)wifi_sta_config.sta.password, sizeof(wifi_sta_config.sta.password), wifi_password);

        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_sta_config));
        ESP_ERROR_CHECK(esp_wifi_start());
        ESP_LOGI(TAG, "WIFI STATION INIT OK");
        break;
    }
    default:
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t wifi_reinit(wifi_mode_t wifi_mode, char wifi_ssid[WIFI_SSID_STR_LENGTH], char wifi_password[WIFI_PASSWORD_STR_LENGTH])
{
    esp_wifi_stop();
    esp_wifi_deinit();
    mdns_started = false;
    ESP_LOGI(TAG, "WIFI DEINIT OK");
    return wifi_init(wifi_mode, wifi_ssid, wifi_password);
}

esp_err_t wifi_get_ip(char ip_string[WIFI_IP_LENGTH])
{
    esp_netif_ip_info_t ip_info;
    if (s_wifi_netif == NULL || ip_string == NULL)
        return ESP_FAIL;
    if (esp_netif_get_ip_info(s_wifi_netif, &ip_info) != ESP_OK)
        return ESP_FAIL;
    if (ip_info.ip.addr == 0)
        return ESP_FAIL;
    snprintf(ip_string, WIFI_IP_LENGTH, "%3d.%3d.%3d.%3d", IP2STR(&ip_info.ip));
    return ESP_OK;
}

esp_err_t wifi_get_mode(wifi_mode_t *wifi_mode)
{
    if (esp_wifi_get_mode(wifi_mode) != ESP_OK)
        return ESP_FAIL;
    return ESP_OK;
}