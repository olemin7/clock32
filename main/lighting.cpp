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
#include <chrono>
#include "sdkconfig.h"

const static char *TAG = "LIGHTING";
using namespace std::chrono_literals;
class presistor
{
private:
    static presistor *ptr_;
    adc_oneshot_unit_handle_t adc1_handle;
    presistor()
    {
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
    }

public:
    presistor(presistor &other) = delete;
    void operator=(const presistor &) = delete;
    uint16_t get() const
    {
        int adc_raw;
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC_CHANNEL_0, &adc_raw));
        ESP_LOGD(TAG, "ADC Raw Data: %d", adc_raw);
        return adc_raw;
    }
    static presistor &instance();
};

presistor *presistor::ptr_ = nullptr;
presistor &presistor::instance()
{
    if (!ptr_)
    {
        ptr_ = new presistor();
    }
    return *ptr_;
}

namespace lighting
{

    lighting::lighting() : timer_([this]()
                                  { adc_val_ = presistor::instance().get(); })
    {
        timer_.start_periodic(std::chrono::milliseconds(CONFIG_LIGHTING_REFRESH));
    }
}
