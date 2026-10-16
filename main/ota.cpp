/*
 * Copyright (c) 2026
 */

#include "ota.hpp"

#include <atomic>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <esp_app_desc.h>
#include <esp_https_ota.h>
#include <esp_log.h>
#include <esp_ota_ops.h>
#include <esp_system.h>

#include "sdkconfig.h"

namespace ota
{
    namespace
    {
        constexpr auto *TAG = "ota";
        constexpr uint32_t TASK_STACK_SIZE = 8192;
        constexpr UBaseType_t TASK_PRIORITY = tskIDLE_PRIORITY + 2;
        constexpr int HTTP_TIMEOUT_MS = 30 * 1000;
        constexpr int LOG_EVERY_PERCENT = 10;
        // Delay before restart so the "done" log and the proto reply get out.
        constexpr int RESTART_DELAY_MS = 1500;

        std::atomic<bool> running{false};
        // Only ever written by the single update task.
        std::string last_result = "never ran";

        void report(const char *msg)
        {
            last_result = msg;
            ESP_LOGI(TAG, "%s", msg);
        }

        void task(void *arg)
        {
            // Owned here: allocated by start(), freed by this task.
            const std::string url = *static_cast<std::string *>(arg);
            delete static_cast<std::string *>(arg);

            ESP_LOGI(TAG, "update from %s", url.c_str());

            // Zero-init + field assignment, same reasoning as in svitlobot.cpp:
            // designated initializers would trip -Wmissing-field-initializers.
            esp_http_client_config_t http_config = {};
            http_config.url = url.c_str();
            http_config.timeout_ms = HTTP_TIMEOUT_MS;
            http_config.keep_alive_enable = true;

            esp_https_ota_config_t ota_config = {};
            ota_config.http_config = &http_config;

            esp_https_ota_handle_t handle = nullptr;
            esp_err_t err = esp_https_ota_begin(&ota_config, &handle);
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "begin failed: %s", esp_err_to_name(err));
                last_result = std::string("begin failed: ") + esp_err_to_name(err);
                running = false;
                vTaskDelete(nullptr);
                return;
            }

            const int total = esp_https_ota_get_image_size(handle);
            int last_logged_percent = -LOG_EVERY_PERCENT;
            do
            {
                err = esp_https_ota_perform(handle);
                if (total > 0)
                {
                    const int percent = 100 * esp_https_ota_get_image_len_read(handle) / total;
                    if (percent >= last_logged_percent + LOG_EVERY_PERCENT)
                    {
                        last_logged_percent = percent;
                        ESP_LOGI(TAG, "progress %d%% (%d/%d bytes)", percent,
                                 esp_https_ota_get_image_len_read(handle), total);
                    }
                }
            } while (err == ESP_ERR_HTTPS_OTA_IN_PROGRESS);

            if (err != ESP_OK || !esp_https_ota_is_complete_data_received(handle))
            {
                // abort() releases the slot; the running image stays untouched.
                esp_https_ota_abort(handle);
                ESP_LOGE(TAG, "download failed: %s", esp_err_to_name(err));
                last_result = std::string("download failed: ") + esp_err_to_name(err);
                running = false;
                vTaskDelete(nullptr);
                return;
            }

            err = esp_https_ota_finish(handle); // validates image, sets boot slot
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "finish failed: %s", esp_err_to_name(err));
                last_result = std::string("finish failed: ") + esp_err_to_name(err);
                running = false;
                vTaskDelete(nullptr);
                return;
            }

            report("update written, restarting");
            vTaskDelay(pdMS_TO_TICKS(RESTART_DELAY_MS));
            esp_restart();
        }
    } // namespace

    bool start(const std::string &url)
    {
        if (url.empty())
        {
            ESP_LOGE(TAG, "empty url");
            return false;
        }
        bool expected = false;
        // compare_exchange: two concurrent "ota" commands must not both pass
        // the check and spawn two tasks writing the same slot.
        if (!running.compare_exchange_strong(expected, true))
        {
            ESP_LOGW(TAG, "update already in progress");
            return false;
        }
        auto *arg = new std::string(url);
        if (xTaskCreate(task, "ota", TASK_STACK_SIZE, arg, TASK_PRIORITY, nullptr) != pdPASS)
        {
            ESP_LOGE(TAG, "xTaskCreate failed");
            delete arg;
            running = false;
            return false;
        }
        return true;
    }

    bool in_progress()
    {
        return running;
    }

    std::string status()
    {
        const esp_partition_t *part = esp_ota_get_running_partition();
        std::string s = "slot:";
        s += part ? part->label : "?";
        s += " built:";
        s += esp_app_get_description()->date;
        s += " ";
        s += esp_app_get_description()->time;
        s += running ? " state:updating" : (" state:idle last:" + last_result);
        return s;
    }

    void mark_valid()
    {
        // Only relevant while the image is pending verification after an OTA;
        // returns an error otherwise, which is fine to ignore.
        if (esp_ota_mark_app_valid_cancel_rollback() == ESP_OK)
        {
            ESP_LOGI(TAG, "image confirmed, rollback cancelled");
        }
    }
} // namespace ota
