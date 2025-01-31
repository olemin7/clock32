/*
 *
 *  Created on: Jun 14, 2024
 *      Author: oleksandr
 */

#pragma once

#include <nvs_flash.h>
#include <nvs.h>
#include <nvs_handle.hpp>
#include <string>
#include <memory>

namespace kvs
{
    class handler
    {
    private:
        std::unique_ptr<nvs::NVSHandle> handle_;

    public:
        handler(const std::string &ns_name);
        template <typename T>
        esp_err_t set_item(const std::string &key, T value)
        {
            if (!handle_)
            {
                return ESP_ERR_INVALID_STATE;
            }
            return handle_->set_item(key.c_str(), value);
        }

        // do not update value if error
        template <typename T>
        esp_err_t get_item(const std::string &key, T &value)
        {
            if (!handle_)
            {
                return ESP_ERR_INVALID_STATE;
            }
            return handle_->get_item(key.c_str(), value);
        }

        template <typename T, typename D>
        void get_item_or(const std::string &key, T &value, const D def_val)
        {
            if (ESP_OK != get_item(key, value))
            {
                value = def_val;
            }
        }
    };

    void init();

} // namespace utils
