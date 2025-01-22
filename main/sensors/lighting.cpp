/*
 * blink.cpp
 *
 *  Created on: Jul 1, 2024
 *      Author: oleksandr
 */
#include "lighting.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/soc_caps.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_system.h"
#include "sdkconfig.h"
#include "sensor_event.hpp"

const static char *TAG = "LIGHTING";

namespace lighting
{
    using namespace sensor_event;
    void task(void *pvParameter)
    {
        ESP_LOGI(TAG, "init");

        adc_oneshot_unit_handle_t adc1_handle;
        //-------------ADC1 Init---------------//

        adc_oneshot_unit_init_cfg_t init_config1 = {
            .unit_id = ADC_UNIT_1,
        };
        ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

        //-------------ADC1 Config---------------//
        adc_oneshot_chan_cfg_t config = {
            .atten = ADC_ATTEN_DB_12,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_0, &config));
        auto pre_val = int{-1};
        while (1)
        {
            sensor_event::lighting_t val;
            ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC_CHANNEL_0, &val.raw));
            ESP_LOGD(TAG, "ADC Raw Data: %d", val.raw);
            if (std::abs(pre_val - val.raw) > CONFIG_LIGHTING_NOISE)
            {
                pre_val = val.raw;
                auto raw = val.raw;
                if (raw > CONFIG_LIGHTING_MAX_RAW)
                {
                    raw = CONFIG_LIGHTING_MAX_RAW;
                }
                else if (raw < CONFIG_LIGHTING_MIN_RAW)
                {
                    raw = CONFIG_LIGHTING_MIN_RAW;
                }

                val.lux = (LUX_MAX - LUX_MIN) * (CONFIG_LIGHTING_MAX_RAW - raw) / (CONFIG_LIGHTING_MAX_RAW - CONFIG_LIGHTING_MIN_RAW) + LUX_MIN;
                ESP_ERROR_CHECK(esp_event_post(sensor_event::event, sensor_event::lighting, &val, sizeof(val), portMAX_DELAY));
            }
            vTaskDelay(pdMS_TO_TICKS(CONFIG_LIGHTING_REFRESH));
        }
    }

    void echo(void * /*arg*/, esp_event_base_t /*event_base*/, int32_t /*event_id*/, void *event_data)
    {
        const auto update = (sensor_event::lighting_t *)event_data;

        ESP_LOGI(TAG, "adc=%d, lux=%u", update->raw, update->lux);
    }

    void init()
    {
        ESP_LOGI(TAG, "starting");
        //  ESP_ERROR_CHECK(esp_event_handler_register(sensor_event::event, sensor_event::lighting, &echo, NULL));
        xTaskCreate(&task, TAG, configMINIMAL_STACK_SIZE + 1024, NULL, 5, NULL);
    }
}
