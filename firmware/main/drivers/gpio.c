#include "gpio.h"
#include "esp_log.h"
#include "main.h"

esp_err_t gpio_init(void)
{
    gpio_config_t gate1_btn_config = {
        .pin_bit_mask = 1ULL << 32,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = false,
        .pull_down_en = true,
        .intr_type = GPIO_INTR_NEGEDGE,
    };

    gpio_config_t gate2_btn_config = {
        .pin_bit_mask = 1ULL << 33,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = false,
        .pull_down_en = true,
        .intr_type = GPIO_INTR_NEGEDGE,
    };

    gpio_config_t reset_btn_config = {
        .pin_bit_mask = 1ULL << 35,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = false,
        .pull_down_en = false,
        .intr_type = GPIO_INTR_NEGEDGE,
    };

    gpio_config_t lap_mode_btn_config = {
        .pin_bit_mask = 1ULL << 25,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = false,
        .pull_down_en = true,
        .intr_type = GPIO_INTR_DISABLE,
    };

    const gpio_config_t *configs[] = {&gate1_btn_config, &gate2_btn_config,
                                      &reset_btn_config,
                                      &lap_mode_btn_config};

    for (int i = 0; i < sizeof(configs) / sizeof(configs[0]); i++)
    {
        ESP_ERROR_CHECK(gpio_config(configs[i]));
    }
    ESP_ERROR_CHECK(gpio_install_isr_service(ESP_INTR_FLAG_IRAM));
    ESP_LOGI("GPIO", "INIT OK");
    return ESP_OK;
}
