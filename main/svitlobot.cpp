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
#include <esp_netif.h>
#include <esp_random.h>
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
        constexpr int HTTP_TOO_MANY_REQUESTS = 429;
        constexpr int MS_PER_S = 1000;
        constexpr int64_t US_PER_MS = 1000;
        constexpr uint32_t TASK_STACK_SIZE = 6144;
        constexpr UBaseType_t TASK_PRIORITY = tskIDLE_PRIORITY + 1;
        constexpr EventBits_t ONLINE = BIT0;
        // Set on every transition to "online" so a sleeping task can abandon its
        // delay and ping right after a reconnect.
        constexpr EventBits_t RECONNECTED = BIT1;

        // Retry schedule. A failed ping used to cost a full interval, so a few
        // consecutive failures (DNS/TLS stalls on a weak link) added up to
        // multi-minute holes in the channel history. Retry quickly instead.
        constexpr uint32_t RETRY_MIN_MS = 5 * MS_PER_S;
        constexpr uint32_t RETRY_MAX_MS = 20 * MS_PER_S;
        constexpr uint32_t ONLINE_RECHECK_MS = 10 * MS_PER_S;
        // api.svitlobot.in.ua records one ping per minute per channel and
        // answers HTTP 429 to the rest.
        constexpr uint32_t API_MIN_PERIOD_MS = 65 * MS_PER_S;

        // A jitter window wider than the interval would delay the first ping
        // past the point where the channel is already reported as offline.
        static_assert(CONFIG_SVITLOBOT_STARTUP_JITTER_S <= CONFIG_SVITLOBOT_INTERVAL_S,
                      "SVITLOBOT_STARTUP_JITTER_S must not exceed SVITLOBOT_INTERVAL_S");
        // Pinging faster than the API allows gets every request thrown away with
        // HTTP 429, which looks exactly like a dead channel.
        static_assert(CONFIG_SVITLOBOT_INTERVAL_S * MS_PER_S >= API_MIN_PERIOD_MS,
                      "SVITLOBOT_INTERVAL_S must be above the API rate limit of one request per minute");

        EventGroupHandle_t event_group = nullptr;

        // The ONLINE bit is driven by IP/Wi-Fi events, and a missed
        // IP_EVENT_STA_GOT_IP would otherwise park the task on the event group
        // forever. Fall back to the real netif state so pings always recover.
        bool link_up()
        {
            esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
            if (!netif || !esp_netif_is_netif_up(netif))
            {
                return false;
            }
            esp_netif_ip_info_t ip_info = {};
            return esp_netif_get_ip_info(netif, &ip_info) == ESP_OK && ip_info.ip.addr != 0;
        }

        void wait_online()
        {
            for (;;)
            {
                if (xEventGroupWaitBits(event_group, ONLINE, pdFALSE, pdTRUE,
                                        pdMS_TO_TICKS(ONLINE_RECHECK_MS)) &
                    ONLINE)
                {
                    return;
                }
                if (link_up())
                {
                    ESP_LOGW(TAG, "offline flag stale (interface has an IP), resyncing");
                    xEventGroupSetBits(event_group, ONLINE);
                    return;
                }
            }
        }

        // Interruptible sleep: returns early when connectivity came back, so the
        // next ping is not postponed by the remainder of the delay.
        void sleep_or_reconnect(uint32_t delay_ms)
        {
            xEventGroupWaitBits(event_group, RECONNECTED, pdTRUE, pdTRUE, pdMS_TO_TICKS(delay_ms));
        }

        void task(void * /*arg*/)
        {
            uint32_t retry_ms = 0;
            bool first_ping = true;
            for (;;)
            {
                wait_online();
                // Only reconnects that happen after this point may cut the sleep
                // short, otherwise a stale flag would cause a back-to-back ping.
                xEventGroupClearBits(event_group, RECONNECTED);

                if (first_ping)
                {
                    first_ping = false;
                    // Spread the post-blackout stampede: every clock on the grid
                    // boots at the same moment, so without this they would all
                    // call the API in the same second.
                    const uint32_t jitter_ms = startup_jitter_ms(esp_random());
                    if (jitter_ms != 0)
                    {
                        ESP_LOGI(TAG, "startup jitter %" PRIu32 " ms", jitter_ms);
                        vTaskDelay(pdMS_TO_TICKS(jitter_ms));
                    }
                }

                uint32_t wait_ms = CONFIG_SVITLOBOT_INTERVAL_S * MS_PER_S;
                switch (ping())
                {
                case ping_result::ok:
                case ping_result::throttled:
                    // Throttled is not a failure: the channel already holds a
                    // fresh timestamp, so the location is reported as powered.
                    // Retrying would only earn another 429 and, worse, hide a
                    // sibling's ping behind our own rejected one.
                    retry_ms = 0;
                    break;
                case ping_result::failed:
                    retry_ms = retry_delay_ms(retry_ms);
                    wait_ms = retry_ms;
                    ESP_LOGW(TAG, "retry in %" PRIu32 " ms", retry_ms);
                    break;
                }
                ESP_LOGD(TAG, "stack high water mark %u", (unsigned)uxTaskGetStackHighWaterMark(nullptr));

                sleep_or_reconnect(wait_ms);
            }
        }
    } // namespace

    std::string make_url(const std::string &base, const std::string &key)
    {
        return base + QUERY_KEY + key;
    }

    uint32_t retry_delay_ms(uint32_t previous_ms)
    {
        if (previous_ms == 0)
        {
            return RETRY_MIN_MS;
        }
        if (previous_ms >= RETRY_MAX_MS)
        {
            return RETRY_MAX_MS;
        }
        const uint32_t doubled = previous_ms * 2;
        return (doubled > RETRY_MAX_MS) ? RETRY_MAX_MS : doubled;
    }

    uint32_t startup_jitter_ms(uint32_t random_value)
    {
        constexpr uint32_t window_ms = CONFIG_SVITLOBOT_STARTUP_JITTER_S * MS_PER_S;
        // if constexpr: with jitter disabled the modulo would be a compile-time
        // division by zero, so the branch must not even be instantiated.
        if constexpr (window_ms == 0)
        {
            return 0;
        }
        else
        {
            return random_value % window_ms;
        }
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

    ping_result ping()
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
            return ping_result::failed;
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
            return ping_result::failed;
        }
        if (status == HTTP_TOO_MANY_REQUESTS)
        {
            // The API records one ping per minute per channel and answers
            // "That's too fast bro" to the rest. Several clocks in the same
            // location intentionally share one channel key, so this normally
            // means a sibling device already reported for us: the channel is
            // fed and the location counts as powered either way.
            ESP_LOGI(TAG, "ping key=%s already reported, HTTP %d (%" PRId64 " ms)",
                     masked.c_str(), status, duration_ms);
            return ping_result::throttled;
        }
        if (status != HTTP_OK)
        {
            ESP_LOGE(TAG, "ping key=%s HTTP %d (%" PRId64 " ms)", masked.c_str(), status, duration_ms);
            return ping_result::failed;
        }
        ESP_LOGI(TAG, "ping key=%s HTTP %d (%" PRId64 " ms)", masked.c_str(), status, duration_ms);
        return ping_result::ok;
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
        ESP_LOGI(TAG, "started, key=%s interval=%ds jitter=%ds", mask_key(key()).c_str(),
                 CONFIG_SVITLOBOT_INTERVAL_S, CONFIG_SVITLOBOT_STARTUP_JITTER_S);
    }

    void set_online(bool online)
    {
        if (!event_group)
        {
            return;
        }
        if (online)
        {
            xEventGroupSetBits(event_group, ONLINE | RECONNECTED);
        }
        else
        {
            xEventGroupClearBits(event_group, ONLINE);
        }
    }
} // namespace svitlobot
