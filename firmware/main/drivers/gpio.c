#include "gpio.h"

esp_err_t gpio_init(void) {
    gpio_config_t gate1_btn_config = {
        .pin_bit_mask = 1ULL << CONFIG_GATE1_PIN,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE,
    };
    ESP_ERROR_CHECK(gpio_config(&gate1_btn_config));

    gpio_config_t gate2_btn_config = {
        .pin_bit_mask = 1ULL << CONFIG_GATE2_PIN,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE,
    };
    ESP_ERROR_CHECK(gpio_config(&gate2_btn_config));

    gpio_config_t stop_btn_config = {
        .pin_bit_mask = 1ULL << CONFIG_STOP_PIN,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE,
    };
    ESP_ERROR_CHECK(gpio_config(&stop_btn_config));

    gpio_config_t wifi_btn_config = {
        .pin_bit_mask = 1ULL << CONFIG_WIFI_PIN,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE,
    };
    ESP_ERROR_CHECK(gpio_config(&wifi_btn_config));

#ifdef CONFIG_TWO_GATE_WIRELESS_MASTER

    gpio_config_t doo_btn_config = {
        .pin_bit_mask = 1ULL << CONFIG_DOO_PIN,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE,
    };
    ESP_ERROR_CHECK(gpio_config(&doo_btn_config));

    gpio_config_t oc_btn_config = {
        .pin_bit_mask = 1ULL << CONFIG_OC_PIN,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE,
    };
    ESP_ERROR_CHECK(gpio_config(&oc_btn_config));

    gpio_config_t driver_select_btn_config = {
        .pin_bit_mask = 1ULL << CONFIG_DRIVER_SELECT_PIN,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE,
    };
    ESP_ERROR_CHECK(gpio_config(&driver_select_btn_config));

    gpio_config_t sd_cd_config = {
        .pin_bit_mask = 1ULL << CONFIG_SD_CD,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&sd_cd_config));

#endif

    ESP_ERROR_CHECK(gpio_install_isr_service(ESP_INTR_FLAG_IRAM));
    ESP_LOGI("GPIO", "INIT OK");
    return ESP_OK;
}
