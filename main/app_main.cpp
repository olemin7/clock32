#include <memory>
#include <stdio.h>
#include <inttypes.h>
#include <chrono>
#include <iostream>
#include <math.h>

#include "rom/rtc.h"
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <esp_log.h>
#include <esp_wifi.h>

#include <nvs_flash.h>
#include <string>
#include "freertos/queue.h"

#include "esp_exception.hpp"
#include "esp_wifi.h"
#include "esp_err.h"
#include "esp_timer_cxx.hpp"
#include <esp_event.h>

#include "json_helper.hpp"
#include "provision.h"
#include "mqtt_wrapper.hpp"
#include "blink.hpp"
#include "utils.hpp"
#include "sntp.hpp"
#include "iot_button.h"
#include "sensors/sensor_event.hpp"
#include "sensors/htu2x.hpp"
#include "sensors/lighting.hpp"

using namespace std::chrono_literals;

std::unique_ptr<mqtt::CMQTTWrapper>       mqtt_mng;
static const char *TAG = "main";

static EventGroupHandle_t app_main_event_group;
constexpr int             SENSORS_DONE         = BIT0;
constexpr int MQTT_CONNECTED_EVENT = BIT1;

static void event_got_ip_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
    ESP_LOGI(TAG, "Connected with IP Address: %s", utils::to_Str(event->ip_info.ip).c_str());
    /* Signal main application to continue execution */
    sntp::init();

    mqtt_mng =
        std::make_unique<mqtt::CMQTTWrapper>([]() { xEventGroupSetBits(app_main_event_group, MQTT_CONNECTED_EVENT); });
    auto json_obj = json::CreateObject();
    cJSON_AddStringToObject(json_obj.get(), "app_name", CONFIG_APP_NAME);
    cJSON_AddStringToObject(json_obj.get(), "ip", utils::to_Str(event->ip_info.ip).c_str());
    int rssi;
    ESP_ERROR_CHECK(esp_wifi_sta_get_rssi(&rssi));
    cJSON_AddNumberToObject(json_obj.get(), "rssi", rssi);

    cJSON_AddStringToObject(json_obj.get(), "mac", utils::get_mac().c_str());

    mqtt_mng->publish(CONFIG_MQTT_TOPIC_ADVERTISEMENT, PrintUnformatted(json_obj));
}

#define BOOT_BUTTON_NUM 9
#define BUTTON_ACTIVE_LEVEL 0
static void button_event_cb(void *arg, void *data)
{
    ESP_LOGW(TAG, "REQ REPROVISION");
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    esp_restart();
}

void init() {
    // Create a default event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    app_main_event_group = xEventGroupCreate();
    /* Initialize NVS partition */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        /* NVS partition was truncated
         * and needs to be erased */
        ESP_ERROR_CHECK(nvs_flash_erase());

        /* Retry nvs_flash_init */
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    /* Initialize TCP/IP */
    ESP_ERROR_CHECK(esp_netif_init());

    /* Initialize the event loop */

    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_got_ip_handler, NULL));
    blink::init();

    button_config_t btn_cfg = {
        .type = BUTTON_TYPE_GPIO,
        .long_press_time = 0,
        .short_press_time = 0,
        .gpio_button_config = {
            .gpio_num = BOOT_BUTTON_NUM,
            .active_level = BUTTON_ACTIVE_LEVEL,
        },
    };
    button_handle_t btn = iot_button_create(&btn_cfg);
    assert(btn);
    ESP_ERROR_CHECK(iot_button_register_cb(btn, BUTTON_LONG_PRESS_START, button_event_cb, NULL));

    htu2x::init();
    lighting::init();
}

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "[APP] Startup..");
    utils::print_info();
    init();
    blink::start(blink::BLINK_PROVISIONING);
    provision_main();
    ESP_LOGI(TAG, "started");

    const auto uxBits = xEventGroupWaitBits(app_main_event_group, SENSORS_DONE | MQTT_CONNECTED_EVENT, pdTRUE, pdTRUE,
                                            pdMS_TO_TICKS(10000));
    blink::start(blink::BLINK_PROVISIONED);
    ESP_LOGI(TAG, "wrapping");
    if (uxBits & MQTT_CONNECTED_EVENT)
    {
        ESP_LOGI(TAG, "flush %d", mqtt_mng->flush(5s));
    }
    else
    {
        ESP_LOGW(TAG, "no MQTT_CONNECTED_EVENT");
    }
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    //  mqtt_mng.reset();some error
}
