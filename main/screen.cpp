#include "screen.hpp"
#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_idf_version.h>
#include <max7219.h>
#include "esp_log.h"
#include "sensors/sensor_event.hpp"
#include "sensors/sensor_event.hpp"

static const char *TAG = "SCREEN";

static const uint64_t symbols[] = {
    0x383838fe7c381000, // arrows
    0x10387cfe38383800,
    0x10307efe7e301000,
    0x1018fcfefc181000,
    0x10387cfefeee4400, // heart
    0x105438ee38541000, // sun

    0x7e1818181c181800, // digits
    0x7e060c3060663c00,
    0x3c66603860663c00,
    0x30307e3234383000,
    0x3c6660603e067e00,
    0x3c66663e06663c00,
    0x1818183030667e00,
    0x3c66663c66663c00,
    0x3c66607c66663c00,
    0x3c66666e76663c00};
static const size_t symbols_size = sizeof(symbols) - sizeof(uint64_t) * CONFIG_EXAMPLE_CASCADE_SIZE;

max7219_t dev;

void task(void *pvParameter)
{

    size_t offs = 0;
    while (1)
    {

        for (uint8_t c = 0; c < CONFIG_EXAMPLE_CASCADE_SIZE; c++)
            max7219_draw_image_8x8(&dev, c * 8, (uint8_t *)symbols + c * 8 + offs);
        vTaskDelay(pdMS_TO_TICKS(100));

        if (++offs == symbols_size)
            offs = 0;
    }
}

uint8_t lighting_to_brightness(uint16_t lux)
{
    if (lux <= sensor_event::LUX_MIN)
    {
        return 0;
    }
    if (lux >= sensor_event::LUX_MAX)
    {
        return MAX7219_MAX_BRIGHTNESS;
    }
    return (lux - sensor_event::LUX_MIN) * (MAX7219_MAX_BRIGHTNESS + 1) / (sensor_event::LUX_MAX - sensor_event::LUX_MIN);
}

void echo(void * /*arg*/, esp_event_base_t /*event_base*/, int32_t /*event_id*/, void *event_data)
{
    static auto pre_level = uint8_t{0};
    const auto update = (sensor_event::lighting_t *)event_data;

    const auto brightness = lighting_to_brightness(update->lux);
    if (brightness != pre_level)
    {
        pre_level = brightness;
        max7219_set_brightness(&dev, pre_level);
        ESP_LOGI(TAG, "brightness=%u", pre_level);
    }
}

void screen::init()
{

    // Configure SPI bus
    spi_bus_config_t cfg = {
        .mosi_io_num = GPIO_NUM_6,
        .miso_io_num = -1,
        .sclk_io_num = GPIO_NUM_4,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 0,
        .flags = 0};
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &cfg, SPI_DMA_CH_AUTO));

    // Configure device
    dev = max7219_t{
        .digits = 0,
        .cascade_size = CONFIG_EXAMPLE_CASCADE_SIZE,
        .mirrored = true,
    };
    ESP_ERROR_CHECK(max7219_init_desc(&dev, SPI2_HOST, MAX7219_MAX_CLOCK_SPEED_HZ, GPIO_NUM_5));
    ESP_ERROR_CHECK(max7219_init(&dev));
    ESP_ERROR_CHECK(max7219_clear(&dev));

    ESP_ERROR_CHECK(esp_event_handler_register(sensor_event::event, sensor_event::lighting, &echo, NULL));

    xTaskCreate(&task, TAG, configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL);
}