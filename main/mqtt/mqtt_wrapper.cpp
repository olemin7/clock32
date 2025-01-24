/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
/* C++ MQTT (over TCP) Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include "mqtt_wrapper.hpp"
#include "nvs_flash.h"

#include <esp_log.h>
#include <memory>

#include "sdkconfig.h"

namespace mqtt
{
    namespace imqtt = idf::mqtt;

    constexpr auto *TAG = "MQTT";
    constexpr int EMPTY_QUEUE = BIT0;

    CMQTTWrapper::CMQTTWrapper()
        : imqtt::Client(imqtt::BrokerConfiguration{.address = {imqtt::URI{std::string{CONFIG_BROKER_URL}}},
                                                   .security = imqtt::Insecure{}},
                        {}, {.connection = {.disable_auto_reconnect = true}})
    {
        ESP_LOGD(TAG, "mqtt_wrapper ctor");
        ESP_LOGI(TAG, "CONFIG_BROKER_URL %s", CONFIG_BROKER_URL);

        // ESP_ERROR_CHECK(esp_event_handler_register(sensor_event::event, sensor_event::internall_temperature, [this](void * /*arg*/, esp_event_base_t /*event_base*/, int32_t /*event_id*/, void *event_data)
        //                                            {
        //                                            if (is_connected_){
        //                                                 const auto temperature = (sensor_event::temperature_t *)event_data;
        //                                                     ESP_LOGI(TAG, "temperature=%f", temperature->val);
        //                                              std::stringstream ss;
        //                                              ss<<"{value:"<<temperature->val<<
        //                                             publish("temperature",)
        //                                            } }, NULL));
        // //  ESP_ERROR_CHECK(esp_event_handler_register(sensor_event::event, sensor_event::internall_humidity, &echo_humidity, NULL));
    };

    CMQTTWrapper::~CMQTTWrapper()
    {
        ESP_LOGD(TAG, "mqtt_wrapper dtor");
        esp_mqtt_client_destroy(handler.get());
    }

    void CMQTTWrapper::on_connected(esp_mqtt_event_handle_t const /*event*/)
    {
        ESP_LOGI(TAG, "connected");
    }

    void CMQTTWrapper::on_disconnected(const esp_mqtt_event_handle_t event)
    {
        ESP_LOGI(TAG, "disconnected");
    }

    void CMQTTWrapper::publish(const std::string &topic, const std::string &message)
    {
        ESP_LOGI(TAG, "add topic:%s, msg:%s", topic.c_str(), message.c_str());

        imqtt::Client::publish(topic, imqtt::StringMessage(message));
    }

} // namespace mqtt
