#include "gpio.h"

esp_err_t gpio_init(void)
{
    gpio_config_t gate1_btn_config = {
        .pin_bit_mask = 1ULL << CONFIG_GATE1_PIN,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = false,
        .pull_down_en = true,
        .intr_type = GPIO_INTR_POSEDGE,
    };

    gpio_config_t gate2_btn_config = {
        .pin_bit_mask = 1ULL << CONFIG_GATE2_PIN,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = false,
        .pull_down_en = true,
        .intr_type = GPIO_INTR_POSEDGE,
    };

    gpio_config_t stop_btn_config = {
        .pin_bit_mask = 1ULL << CONFIG_STOP_PIN,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = true,
        .pull_down_en = false,
        .intr_type = GPIO_INTR_NEGEDGE,
    };

    gpio_config_t wifi_btn_config = {
        .pin_bit_mask = 1ULL << CONFIG_WIFI_PIN,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = true,
        .pull_down_en = false,
        .intr_type = GPIO_INTR_NEGEDGE,
    };

    gpio_config_t doo_btn_config = {
        .pin_bit_mask = 1ULL << CONFIG_DOO_PIN,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = true,
        .pull_down_en = false,
        .intr_type = GPIO_INTR_NEGEDGE,
    };

    gpio_config_t oc_btn_config = {
        .pin_bit_mask = 1ULL << CONFIG_OC_PIN,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = true,
        .pull_down_en = false,
        .intr_type = GPIO_INTR_NEGEDGE,
    };

    gpio_config_t driver_select_btn_config = {
        .pin_bit_mask = 1ULL << CONFIG_DRIVER_SELECT_PIN,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = true,
        .pull_down_en = false,
        .intr_type = GPIO_INTR_NEGEDGE,
    };

    gpio_config_t sd_cd_config = {
        .pin_bit_mask = 1ULL << CONFIG_SD_CD,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = false,
        .pull_down_en = false,
    };

    const gpio_config_t *configs[] = {&gate1_btn_config, &gate2_btn_config, &stop_btn_config,
                                      &wifi_btn_config, &doo_btn_config, &oc_btn_config, &driver_select_btn_config, &sd_cd_config};

    for (int i = 0; i < sizeof(configs) / sizeof(configs[0]); i++)
    {
        ESP_ERROR_CHECK(gpio_config(configs[i]));
    }
    ESP_ERROR_CHECK(gpio_install_isr_service(ESP_INTR_FLAG_IRAM));
    ESP_LOGI("GPIO", "INIT OK");
    return ESP_OK;
}
