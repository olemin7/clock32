/*
 *
 *  Created on: Jun 14, 2024
 *      Author: oleksandr
 */

#pragma once

#include <string>
#include "cJSON.h"
#include <map>
#include <functional>
#include <optional>

namespace proto
{
    using cJSON_opt_t = std::optional<const cJSON *>;
    cJSON_opt_t get_field(cJSON_opt_t data, const std::string &name);
    std::optional<std::string> get_field_string(cJSON_opt_t data, const std::string &name);
    std::optional<double> get_field_number(cJSON_opt_t data, const std::string &name);
    std::optional<bool> get_field_bool(cJSON_opt_t data, const std::string &name);

    class handler
    {
    public:
        using command_t = std::function<void(cJSON_opt_t payload)>;

    private:
        std::map<std::string, command_t> handler_;

    public:
        handler();
        void on_command(const std::string &msg);
        void add(const std::string cmd, command_t &&handler);
    };

} // namespace utils
