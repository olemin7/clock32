#include "screen.hpp"
#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_idf_version.h>
#include <max7219.h>
#include <array>
#include "esp_log.h"
#include "sensors/sensor_event.hpp"
#include "font.hpp"
#include "transformation.hpp"

namespace screen
{
    static const char *TAG = "SCREEN";
    constexpr auto SEGMENT_ROTATION = 3;
    constexpr auto SEGMENT_UPSIDEDOWN = false;
    constexpr auto DISPLAYS_MIRRORED = false;

    max7219_t dev;

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

    esp_err_t max7219_buffer_raw(const buffer_t &buffer)
    {
        uint8_t pos = 0;
        for (auto &line : buffer)
        {

            const auto ret = max7219_set_digit(&dev, pos, line);
            if (ret != ESP_OK)
            {
                return ret;
            }
            pos++;
        }
        return ESP_OK;
    }

    esp_err_t print(const buffer_t &buffer)
    {
        auto transformed = transformation::buffer_by_segment_rotate(buffer, SEGMENT_ROTATION);
        if (SEGMENT_UPSIDEDOWN)
        {
            for (auto &line : transformed)
            {
                line = transformation::reverse_bits(line);
            }
        }
        return max7219_buffer_raw(transformed);
    }

    void get_test_buffer(buffer_t &buffer)
    {
        auto pos = sizeof(buffer_t);
        while (pos--)
        {
            buffer[pos] = 1;
        }
        buffer[0] = 0xff;
    }

    void test1()
    {
        auto buffer = transformation::get_test_buffer();
        for (int i = 0; i < 4; i++)
        {

            max7219_buffer_raw(transformation::buffer_by_segment_rotate(buffer, i));
            vTaskDelay(pdMS_TO_TICKS(2000));
        }
    }

    void test2()
    {
        auto image = font::get("123");
        for (int justify = 0; justify <= js_right; justify++)
        {
            print(transformation::image2buff(image, static_cast<justify_t>(justify), 0));
            vTaskDelay(pdMS_TO_TICKS(2000));
        }
    }

    void test3()
    {
        auto image = font::get("24:55");

        print(transformation::image2buff(image, js_center, 0));
    }

    void startup_screen()
    {
        auto buffer = transformation::get_test_buffer();
        print(buffer);
    }

    void init()
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
            .mirrored = DISPLAYS_MIRRORED,
        };
        ESP_ERROR_CHECK(max7219_init_desc(&dev, SPI2_HOST, MAX7219_MAX_CLOCK_SPEED_HZ, GPIO_NUM_5));
        ESP_ERROR_CHECK(max7219_init(&dev));
        ESP_ERROR_CHECK(max7219_clear(&dev));

        ESP_ERROR_CHECK(esp_event_handler_register(sensor_event::event, sensor_event::lighting, &echo, NULL));
        startup_screen();
    }

    esp_err_t print(const std::vector<uint8_t> &image, const justify_t justify, const uint8_t offset)
    {
        const auto buffer = transformation::image2buff(image, justify, offset);
        return print(buffer);
    }

    esp_err_t print(const std::string str, const justify_t justify, const uint8_t offset)
    {
        const auto image = font::get(str);
        return print(image, justify, offset);
    }
}