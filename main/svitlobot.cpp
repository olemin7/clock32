/*
 * Copyright (c) 2026
 */

#include "svitlobot.hpp"

#include <inttypes.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include <esp_http_client.h>
#include <esp_log.h>
#include <esp_timer.h>

#include "sdkconfig.h"

#if __has_include("svitlobot_secret.h")
#include "svitlobot_secret.h"
#endif
#ifndef SVITLOBOT_CHANNEL_KEY
#define SVITLOBOT_CHANNEL_KEY ""
#endif

namespace svitlobot
{
    namespace
    {
        constexpr auto *TAG = "SvitloBot";
        constexpr auto *QUERY_KEY = "?channel_key=";
        constexpr size_t MASK_VISIBLE_CHARS = 2;
        constexpr int HTTP_OK = 200;
        constexpr int MS_PER_S = 1000;
        constexpr int64_t US_PER_MS = 1000;
        constexpr uint32_t TASK_STACK_SIZE = 6144;
        constexpr UBaseType_t TASK_PRIORITY = tskIDLE_PRIORITY + 1;
        constexpr EventBits_t ONLINE = BIT0;

        EventGroupHandle_t event_group = nullptr;

        void task(void * /*arg*/)
        {
            for (;;)
            {
                xEventGroupWaitBits(event_group, ONLINE, pdFALSE, pdTRUE, portMAX_DELAY);
                ping();
                ESP_LOGD(TAG, "stack high water mark %u", (unsigned)uxTaskGetStackHighWaterMark(nullptr));
                vTaskDelay(pdMS_TO_TICKS(CONFIG_SVITLOBOT_INTERVAL_S * MS_PER_S));
            }
        }
    } // namespace

    std::string make_url(const std::string &base, const std::string &key)
    {
        return base + QUERY_KEY + key;
    }

    std::string mask_key(const std::string &key)
    {
        if (key.size() <= 2 * MASK_VISIBLE_CHARS)
        {
            return std::string(key.size(), '*');
        }
        return key.substr(0, MASK_VISIBLE_CHARS) +
               std::string(key.size() - 2 * MASK_VISIBLE_CHARS, '*') +
               key.substr(key.size() - MASK_VISIBLE_CHARS);
    }

    const std::string &key()
    {
        static const std::string channel_key = SVITLOBOT_CHANNEL_KEY;
        return channel_key;
    }

    bool ping()
    {
        const auto url = make_url(CONFIG_SVITLOBOT_URL, key());

        // Zero-init + field assignment: the struct has many members and
        // designated initializers would trigger -Wmissing-field-initializers.
        esp_http_client_config_t config = {};
        config.url = url.c_str();
        config.method = HTTP_METHOD_GET;
        config.timeout_ms = CONFIG_SVITLOBOT_TIMEOUT_S * MS_PER_S;

        esp_http_client_handle_t client = esp_http_client_init(&config);
        if (!client)
        {
            ESP_LOGE(TAG, "esp_http_client_init failed");
            return false;
        }

        const int64_t start_us = esp_timer_get_time();
        const esp_err_t err = esp_http_client_perform(client);
        const int64_t duration_ms = (esp_timer_get_time() - start_us) / US_PER_MS;
        const int status = esp_http_client_get_status_code(client);
        esp_http_client_cleanup(client);

        const auto masked = mask_key(key());
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "ping key=%s failed: %s (%" PRId64 " ms)", masked.c_str(), esp_err_to_name(err), duration_ms);
            return false;
        }
        if (status != HTTP_OK)
        {
            ESP_LOGE(TAG, "ping key=%s HTTP %d (%" PRId64 " ms)", masked.c_str(), status, duration_ms);
            return false;
        }
        ESP_LOGI(TAG, "ping key=%s HTTP %d (%" PRId64 " ms)", masked.c_str(), status, duration_ms);
        return true;
    }

    void init()
    {
        if (key().empty())
        {
            ESP_LOGW(TAG, "channel key not set (main/svitlobot_secret.h), disabled");
            return;
        }
        event_group = xEventGroupCreate();
        if (!event_group)
        {
            ESP_LOGE(TAG, "xEventGroupCreate failed, disabled");
            return;
        }
        if (xTaskCreate(task, "svitlobot", TASK_STACK_SIZE, nullptr, TASK_PRIORITY, nullptr) != pdPASS)
        {
            ESP_LOGE(TAG, "xTaskCreate failed, disabled");
            vEventGroupDelete(event_group);
            event_group = nullptr;
            return;
        }
        ESP_LOGI(TAG, "started, key=%s interval=%ds", mask_key(key()).c_str(), CONFIG_SVITLOBOT_INTERVAL_S);
    }

    void set_online(bool online)
    {
        if (!event_group)
        {
            return;
        }
        if (online)
        {
            xEventGroupSetBits(event_group, ONLINE);
        }
        else
        {
            xEventGroupClearBits(event_group, ONLINE);
        }
    }
} // namespace svitlobot
