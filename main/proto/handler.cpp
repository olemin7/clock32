/*
 *
 *  Created on: Jun 14, 2024
 *      Author: oleksandr
 */

#include "handler.hpp"
#include <esp_log.h>

namespace proto
{
    static const char *TAG = "proto_cmd_handler";

    cJSON_opt_t get_field(cJSON_opt_t data, const std::string &name)
    {
        if (data)
        {
            const auto cmd_j = cJSON_GetObjectItem(data.value(), name.c_str());
            if (cmd_j != nullptr)
            {
                return cmd_j;
            }
        }
        return {};
    }

    std::optional<std::string> get_field_string(cJSON_opt_t data, const std::string &name)
    {
        const auto field = get_field(data, name);
        if (!field || !cJSON_IsString(field.value()))
        {
            return {};
        }
        return field.value()->valuestring;
    }

    std::optional<double> get_field_number(cJSON_opt_t data, const std::string &name)
    {
        const auto field = get_field(data, name);
        if (!field || !cJSON_IsNumber(field.value()))
        {
            return {};
        }
        return field.value()->valuedouble;
    }

    std::optional<bool> get_field_bool(cJSON_opt_t data, const std::string &name)
    {
        const auto field = get_field(data, name);
        if (!field || !cJSON_IsBool(field.value()))
        {
            return {};
        }
        return cJSON_IsTrue(field.value());
    }

    handler::handler() = default;

    void handler::on_command(const std::string &msg)
    {
        ESP_LOGI(TAG, "got msg =%s", msg.c_str());
        cJSON *root = cJSON_Parse(msg.c_str());
        if (nullptr == root)
        {
            ESP_LOGE(TAG, "no root");
            return;
        }

        auto cmd = get_field_string(root, "cmd").value_or("");

        if (cmd != "")
        {
            auto it = handler_.find(cmd);
            if (it != handler_.end())
            {
                auto payload = get_field(root, "payload");

                it->second(payload);
            }
            else
            {
                ESP_LOGE(TAG, "no handler for cmd");
            }
        }
        else
        {
            ESP_LOGE(TAG, "!cmd");
        }
        cJSON_Delete(root);
    }

    void handler::add(const std::string cmd, command_t &&handler)
    {
        handler_[cmd] = handler;
    }
}
