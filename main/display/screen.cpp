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
#include "sdkconfig.h"
#include "kvs.hpp"

namespace screen
{
    constexpr auto TAG = "SCREEN";

    constexpr auto kvs_segment_rotation = "s_rotation";
    constexpr auto kvs_segment_upsidedown = "s_upsidedown";
    constexpr auto kvs_mirrored = "mirrored";

    max7219_t dev;

    uint8_t display_segment_rotation;
    bool display_segment_upsidedown;

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

        const auto brightness = lighting_to_brightness(update->val);
        if (brightness != pre_level)
        {
            pre_level = brightness;
            max7219_set_brightness(&dev, pre_level);
            ESP_LOGD(TAG, "brightness=%u", pre_level);
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
        auto transformed = transformation::buffer_by_segment_rotate(buffer, display_segment_rotation);
        if (display_segment_upsidedown)
        {
            for (auto &line : transformed)
            {
                line = transformation::reverse_bits(line);
            }
        }
        return max7219_buffer_raw(transformed);
    }

    void test_rotation()
    {
        auto buffer = transformation::get_test_buffer();
        for (int i = 0; i < 4; i++)
        {

            max7219_buffer_raw(transformation::buffer_by_segment_rotate(buffer, i));
            vTaskDelay(pdMS_TO_TICKS(2000));
        }
    }

    void startup_screen()
    {
        auto buffer = transformation::get_test_buffer();
        print(buffer);
    }

    void init()
    {
        auto kvss = kvs::handler(TAG);
        bool display_mirrored;

        kvss.get_item_or(kvs_segment_rotation, display_segment_rotation, CONFIG_DISPLAY_SEGMENT_ROTATION);
        kvss.get_item_or(kvs_segment_upsidedown, display_segment_upsidedown, CONFIG_DISPLAY_SEGMENT_UPSIDEDOWN);
        kvss.get_item_or(kvs_mirrored, display_mirrored, CONFIG_DISPLAY_MIRRORED);

        // kvss.get_item_or("segment_rotation", display_segment_rotation, 3);
        // kvss.get_item_or("segment_upsidedown", display_segment_upsidedown, CONFIG_DISPLAY_SEGMENT_UPSIDEDOWN);
        // kvss.get_item_or("mirrored", display_mirrored, true);

        ESP_LOGI(TAG, "segment_rotation=%d segment_upsidedown=%d mirrored=%d", display_segment_rotation, display_segment_upsidedown, display_mirrored);

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
            .cascade_size = SEGMENTS,
            .mirrored = display_mirrored,
        };
        ESP_ERROR_CHECK(max7219_init_desc(&dev, SPI2_HOST, MAX7219_MAX_CLOCK_SPEED_HZ, GPIO_NUM_5));
        ESP_ERROR_CHECK(max7219_init(&dev));
        ESP_ERROR_CHECK(max7219_clear(&dev));

        ESP_ERROR_CHECK(esp_event_handler_register(sensor_event::event, sensor_event::lighting, &echo, NULL));
        //    test_rotation();
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

    esp_err_t set_config(uint8_t segment_rotation,
                         bool segment_upsidedown,
                         bool mirrored)
    {
        auto kvss = kvs::handler(TAG);

        ESP_LOGI(TAG, "rotation %d, upsidedown %d, mirrored %d", segment_rotation, segment_upsidedown, mirrored);

        bool error = false;

        if (ESP_OK != kvss.set_item(kvs_segment_rotation, segment_rotation))
        {
            error = true;
        }

        if (ESP_OK != kvss.set_item(kvs_segment_upsidedown, segment_upsidedown))
        {
            error = true;
        }

        if (ESP_OK != kvss.set_item(kvs_mirrored, mirrored))
        {
            error = true;
        }

        if (error)
        {
            ESP_LOGE(TAG, "error wr");
        }

        return ESP_OK;
    }
}
