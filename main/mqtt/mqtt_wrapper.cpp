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

    CMQTTWrapper::CMQTTWrapper(device_info_t &device_info)
        : imqtt::Client(imqtt::BrokerConfiguration{.address = {imqtt::URI{std::string{CONFIG_BROKER_URL}}},
                                                   .security = imqtt::Insecure{}},
                        {}, {.connection = {.disable_auto_reconnect = true}}),
          device_info_(device_info)
    {
        ESP_LOGI(TAG, "CONFIG_BROKER_URL %s", CONFIG_BROKER_URL);
    };

    CMQTTWrapper::~CMQTTWrapper()
    {
        ESP_LOGD(TAG, "mqtt_wrapper dtor");
        esp_mqtt_client_destroy(handler.get());
    }

    void CMQTTWrapper::on_connected(esp_mqtt_event_handle_t const /*event*/)
    {
        ESP_LOGI(TAG, "connected");
        std::string info;

        info = "{ \"sw\":" + device_info_.sw + ",\"mac\":" + device_info_.mac + ",\"ip\":" + device_info_.ip + ",\"rssi\":" + std::to_string(device_info_.rssi) + "}";

        publish(CONFIG_MQTT_TOPIC_ADVERTISEMENT, info);
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
