/*
 * blink.cpp
 *
 *  Created on: Jul 1, 2024
 *      Author: oleksandr
 */
#include "http_server.hpp"
#include "esp_http_server.h"

#include <inttypes.h>
#include <stdio.h>
#include <chrono>
#include "esp_log.h"
#include <esp_wifi.h>
#include <memory>

using namespace std::chrono_literals;
static const char *TAG = "http_server";

static esp_err_t cmd_handler(httpd_req_t *req)
{
    // Limit payload size for safety
    const size_t max_len = 1024;
    int total_len = req->content_len;
    int received = 0;

    if (total_len >= max_len)
    {
        ESP_LOGW(TAG, "POST payload too large");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Payload too large");
        return ESP_FAIL;
    }

    std::string buf;
    buf.resize(total_len);

    while (received < total_len)
    {
        int ret = httpd_req_recv(req, &buf[received], total_len - received);
        if (ret <= 0)
        {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT)
            {
                httpd_resp_send_408(req);
            }
            return ESP_FAIL;
        }
        received += ret;
    }

    ESP_LOGI(TAG, "Received POST JSON: %s", buf.c_str());

    // TODO: Parse/process JSON here

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{}", 2); // Respond with empty JSON object
    return ESP_OK;
}

static const httpd_uri_t cmd = {
    .uri = "/cmd",
    .method = HTTP_POST,
    .handler = cmd_handler,
    .user_ctx = NULL};

httpd_handle_t start_webserver(void)
{
    // Generate default configuration
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    // Empty handle to http_server
    httpd_handle_t server = NULL;
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    // Start the httpd server
    if (httpd_start(&server, &config) == ESP_OK)
    {
        // Register URI handlers
        httpd_register_uri_handler(server, &cmd);
    }
    // If server failed to start, handle will be NULL
    return server;
}

// Function for stopping the webserver
static esp_err_t stop_webserver(httpd_handle_t server)
{
    // Ensure handle is non NULL
    if (server != NULL)
    {
        // Stop the httpd server
        return httpd_stop(server);
    }
    return ESP_OK;
}

static void disconnect_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    httpd_handle_t *server = (httpd_handle_t *)arg;
    if (*server)
    {
        ESP_LOGI(TAG, "Stopping webserver");
        if (stop_webserver(*server) == ESP_OK)
        {
            *server = NULL;
        }
        else
        {
            ESP_LOGE(TAG, "Failed to stop http server");
        }
    }
}

static void connect_handler(void *arg, esp_event_base_t event_base,
                            int32_t event_id, void *event_data)
{
    httpd_handle_t *server = (httpd_handle_t *)arg;
    if (*server == NULL)
    {
        ESP_LOGI(TAG, "Starting webserver");
        *server = start_webserver();
    }
}
namespace http_server
{
    server::server()
    {
        ESP_LOGI(TAG, "http_server::server()");
        // Initialize the HTTP server
        static httpd_handle_t server = NULL;
        // Register event handlers for connection and disconnection events
        ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &connect_handler, &server));
        ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disconnect_handler, &server));
    }

    server &server::get_instance()
    {
        static std::unique_ptr<server> instance_;
        if (!instance_)
        {
            instance_ = std::unique_ptr<server>(new server());
            ESP_LOGD(TAG, "http_server::server::get_instance() created");
        }
        return *instance_;
    }

    void server::set_uri(const std::string &uri, uri_handler_t &&handler)
    {
        ESP_LOGI(TAG, "http_server::server::set_uri() URI handler set for: %s", uri.c_str());
        uri_handlers_[uri] = std::move(handler);
    }
}