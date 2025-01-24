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
   class CMQTTWrapper : public idf::mqtt::Client
   {
   private:
      // ESPTimer timer_
   public:
      CMQTTWrapper();
      virtual ~CMQTTWrapper();
      void publish(const std::string &topic, const std::string &message);
      template <typename T>
      void publish(const std::string &topic, const std::string &field, T value)
      {
         std::stringstream ss;
         ss << "{\"" << field << "\":" << value << "}";
         publish(topic, ss.str());
      }

   private:
      void on_connected(const esp_mqtt_event_handle_t event) final;
      void on_disconnected(const esp_mqtt_event_handle_t event) final;
      void on_published(const esp_mqtt_event_handle_t event) final {}
      void on_data(const esp_mqtt_event_handle_t event) final {};

      void send_queue();
   };

} // namespace mqtt
