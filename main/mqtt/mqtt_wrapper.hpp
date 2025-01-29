/*
 * mqtt.h
 *
 *  Created on: Jun 14, 2024
 *      Author: oleksandr
 */

#pragma once
#include <string.h>
#include <chrono>
#include <sstream>
#include "esp_mqtt.hpp"
#include "esp_mqtt_client_config.hpp"
#include "esp_timer_cxx.hpp"

namespace mqtt
{
   struct device_info_t
   {
      std::string sw;
      std::string ip;
      std::string mac;
      int rssi;
   };

   class CMQTTWrapper : public idf::mqtt::Client
   {
   private:
      const device_info_t &device_info_;
      // ESPTimer timer_
   public:
      CMQTTWrapper(device_info_t &device_info);
      virtual ~CMQTTWrapper();
      void publish(const std::string &topic, const std::string &message);
      template <typename T>
      void publish(const std::string &topic, const std::string &field, T value)
      {
         publish(topic, "{\"" + field + "\":" + std::to_string(value) + "}");
      }

      template <typename T>
      void publish_device_brunch(const std::string &topic, const std::string &field, T value)
      {
         publish("/devices/" + device_info_.mac + "/" + topic, field, value);
      }

   private:
      void on_connected(const esp_mqtt_event_handle_t event) final;
      void on_disconnected(const esp_mqtt_event_handle_t event) final;
      void on_published(const esp_mqtt_event_handle_t event) final {}
      void on_data(const esp_mqtt_event_handle_t event) final {};

      void send_queue();
   };

} // namespace mqtt
