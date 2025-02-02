#include <memory>
#include <stdio.h>
#include <inttypes.h>
#include <chrono>
#include <iostream>
#include <math.h>
#include <string>

#include "rom/rtc.h"
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <esp_log.h>
#include <esp_wifi.h>
#include "freertos/queue.h"

#include "esp_exception.hpp"
#include "esp_wifi.h"
#include "esp_err.h"
#include "esp_timer_cxx.hpp"
#include <esp_event.h>

#include "json_helper.hpp"
#include "provision.h"
#include "mqtt/mqtt_wrapper.hpp"
#include "display/blink.hpp"
#include "utils.hpp"
#include "time/sntp.hpp"
#include "time/clock_tm.hpp"
#include "iot_button.h"
#include "sensors/sensor_event.hpp"
#include "sensors/htu2x.hpp"
#include "sensors/lighting.hpp"
#include "display/screen.hpp"
#include "display/layers.hpp"
#include "display/tests.hpp"
#include "kvs.hpp"
#include "proto/defines.hpp"

using namespace std::chrono_literals;
static const char *TAG = "main";

constexpr auto DEVICE_SW = CONFIG_APP_NAME " " __DATE__ " " __TIME__;
layers::layers diplay;

std::unique_ptr<mqtt::CMQTTWrapper> mqtt_mng = nullptr;
std::unique_ptr<clock_tm::clock> clock_ptr = nullptr;
button_handle_t btn_ptr = nullptr;

proto::handler commands;

static EventGroupHandle_t app_main_event_group;
constexpr int GOT_IP = BIT0;

static void event_got_ip_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    mqtt::device_info_t device_info;
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    device_info.ip = utils::to_Str(event->ip_info.ip);

    ESP_LOGI(TAG, "Connected with IP Address: %s", device_info.ip.c_str());
    diplay.show(4, device_info.ip, screen::js_right);

    /* Signal main application to continue execution */
    xEventGroupSetBits(app_main_event_group, GOT_IP);

    ESP_ERROR_CHECK(esp_wifi_sta_get_rssi(&device_info.rssi));

    device_info.sw = DEVICE_SW;
    device_info.mac = utils::get_mac();

    mqtt_mng = std::make_unique<mqtt::CMQTTWrapper>(device_info, [](auto msg)
                                                    { commands.on_command(msg); });

    blink::stop(blink::BLINK_CONNECTING);
}

static void event_lost_ip_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    blink::start(blink::BLINK_CONNECTING);
    ESP_LOGI(TAG, "event_lost_ip_handler");
    mqtt_mng.reset();
}

#define BOOT_BUTTON_NUM 9
#define BUTTON_ACTIVE_LEVEL 0
static void button_event_cb(void *arg, void *data)
{
    ESP_LOGW(TAG, "REQ REPROVISION");
    diplay.show(10, "***");
    ESP_ERROR_CHECK(provision_reset());
    vTaskDelay(pdMS_TO_TICKS(1000));
    esp_restart();
}

template <typename T>
void mqtt_send_sensor(const std::string &field, T value)
{
    if (mqtt_mng)
    {
        mqtt_mng->publish_device_brunch("sensors", field, value);
    }
}

void mqtt_temperature(void * /*arg*/, esp_event_base_t /*event_base*/, int32_t /*event_id*/, void *event_data)
{
    const auto event = (sensor_event::temperature_t *)event_data;
    mqtt_send_sensor("temperature", event->val);
}

void mqtt_humidity(void * /*arg*/, esp_event_base_t /*event_base*/, int32_t /*event_id*/, void *event_data)
{
    const auto event = (sensor_event::humidity_t *)event_data;
    mqtt_send_sensor("humidity", event->val);
}

void mqtt_lighting(void * /*arg*/, esp_event_base_t /*event_base*/, int32_t /*event_id*/, void *event_data)
{
    const auto event = (sensor_event::lighting_t *)event_data;
    mqtt_send_sensor("lighting", event->val);
    mqtt_send_sensor("adc", event->raw);
}

void init()
{
    utils::print_info();
    // Create a default event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    app_main_event_group = xEventGroupCreate();
    kvs::init();

    /* Initialize TCP/IP */
    ESP_ERROR_CHECK(esp_netif_init());

    /* Initialize the event loop */
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_got_ip_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_LOST_IP, &event_lost_ip_handler, NULL));
    blink::init();
    screen::init();

    button_config_t btn_cfg = {
        .type = BUTTON_TYPE_GPIO,
        .long_press_time = 5 * 1000,
        .short_press_time = 0,
        .gpio_button_config = {
            .gpio_num = BOOT_BUTTON_NUM,
            .active_level = BUTTON_ACTIVE_LEVEL,
        },
    };
    button_handle_t btn_ptr = iot_button_create(&btn_cfg);
    assert(btn_ptr);
    ESP_ERROR_CHECK(iot_button_register_cb(btn_ptr, BUTTON_LONG_PRESS_START, button_event_cb, NULL));
    lighting::init();
    commands.add("ldr", [](auto payload)
                 {
                     proto::ldr_t data;
                     if (proto::get(payload,data))
                     {
                         lighting::update_adc_range(data.min, data.max);
                     } });
    commands.add("display", [](auto payload)
                 {
                     proto::display_t data;
                     if (proto::get(payload,data))
                     {
                        screen::set_config(data.segment_rotation,data.segment_upsidedown,data.mirrored);
                     } });

    commands.add("brightness", [](auto payload)
                 {
                    proto::brightness_t data;
                    if (proto::get(payload,data))
                    {
                        screen::set_config_brightness(data.min, data.max);
                    } });

    commands.add("restart", [](auto)
                 { 
                    ESP_LOGI(TAG, "esp_restart");
                    vTaskDelay(pdMS_TO_TICKS(500));
                    esp_restart(); });

    commands.add("factory_reset", [](auto)
                 { 
                    ESP_LOGI(TAG, "factory_reset");
                    ESP_ERROR_CHECK(nvs_flash_erase()); 
                    vTaskDelay(pdMS_TO_TICKS(500));
                    esp_restart(); });

    commands.add("timezone", [](auto payload)
                 {
                    proto::timezone_t data;
                    if (proto::get(payload,data))
                    {
                        clock_tm::update_time_zone(data.tz);
                    } });
}

/************************************
 *
 */

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "[APP] Startup..");
    init();
    //  screen::tests();

    //------------------------------
    blink::start(blink::BLINK_PROVISIONING);
    provision_main();
    ESP_LOGI(TAG, "started");
    blink::stop(blink::BLINK_PROVISIONING);
    //------------------------------

    blink::start(blink::BLINK_CONNECTING);
    xEventGroupWaitBits(app_main_event_group, GOT_IP, pdTRUE, pdTRUE, portMAX_DELAY);

    htu2x::init();
    sntp::init([]()
               {
        if (!clock_ptr)
        {
            clock_ptr = std::make_unique<clock_tm::clock>([](auto timeinfo)
                                                          {
            char buffMin[6];
            sprintf(buffMin, "%u:%02u", timeinfo.tm_hour, timeinfo.tm_min);
            diplay.show(5, buffMin, screen::js_center);
            ESP_LOGI(TAG, "clock %s",buffMin); });
        } });
    //--------------------------------
    ESP_ERROR_CHECK(esp_event_handler_register(sensor_event::event, sensor_event::internall_temperature, &mqtt_temperature, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(sensor_event::event, sensor_event::internall_humidity, &mqtt_humidity, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(sensor_event::event, sensor_event::lighting, &mqtt_lighting, NULL));

    ESP_LOGI(TAG, "done");
}
