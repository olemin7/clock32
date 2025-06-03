#pragma once
#include <string>
#include <functional>
#include <map>
namespace http_server
{
    class server
    {
    public:
        using uri_handler_t = std::function<std::string(const std::string &)>;

    private:
        std::map<std::string, uri_handler_t> uri_handlers_;
        server();

    public:
        static server &get_instance();

        server(const server &) = delete;
        server &operator=(const server &) = delete;

        void set_uri(const std::string &uri, uri_handler_t &&handler);
    };
}