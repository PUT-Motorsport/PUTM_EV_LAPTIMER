#include "spi.h"

#include "esp_log.h"

esp_err_t spi_init(void) {
  spi_bus_config_t sdcard_spi_config = {
      .mosi_io_num = SD_SPI_MOSI,
      .miso_io_num = SD_SPI_MISO,
      .sclk_io_num = SD_SPI_CLK,
      .quadwp_io_num = -1,
      .quadhd_io_num = -1,
      .max_transfer_sz = 4 * 1000,
  };
  //   spi_bus_config_t lcd_spi_config = {
  //       .mosi_io_num = LCD_SPI_MOSI,
  //       .miso_io_num = -1,
  //       .sclk_io_num = LCD_SPI_CLK,
  //       .quadwp_io_num = -1,
  //       .quadhd_io_num = -1,
  //       .max_transfer_sz = 1 * 1000 * 1000,

  //   };
  ESP_ERROR_CHECK(
      spi_bus_initialize(SPI2_HOST, &sdcard_spi_config, SPI_DMA_CH1));
  //   ESP_ERROR_CHECK(spi_bus_initialize(SPI3_HOST, &lcd_spi_config,
  //   SPI_DMA_CH2));
  ESP_LOGI("SPI", "INIT OK");
  return ESP_OK;
}