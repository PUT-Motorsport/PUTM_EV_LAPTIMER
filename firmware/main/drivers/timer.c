#include "driver/gptimer.h"
#include "esp_err.h"
#include "esp_log.h"
#include "soc/clk_tree_defs.h"

gptimer_handle_t laptime_timer;

esp_err_t timer_init() {
  gptimer_config_t timer_config = {.clk_src = GPTIMER_CLK_SRC_DEFAULT,
                                   .direction = GPTIMER_COUNT_UP,
                                   .resolution_hz = 1 * 10000};
  ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &laptime_timer));
  ESP_ERROR_CHECK(gptimer_enable(laptime_timer));
  ESP_ERROR_CHECK(gptimer_start(laptime_timer));
  ESP_LOGI("TIMER", "INIT OK");
  return ESP_OK;
}

uint64_t timer_get_time(gptimer_handle_t timer_handle) {
  uint64_t timer_value = 0;
  uint32_t timer_res = 0;
  uint64_t timer_time_10ms = 0;
  ESP_ERROR_CHECK(gptimer_get_raw_count(timer_handle, &timer_value));
  ESP_ERROR_CHECK(gptimer_get_resolution(timer_handle, &timer_res));
  timer_time_10ms = (timer_value * 100) / timer_res;
  return timer_time_10ms;
}

esp_err_t timer_reset(gptimer_handle_t timer_handle) {
  return gptimer_set_raw_count(timer_handle, 0);
}