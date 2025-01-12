#include "sntp.hpp"
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "esp_log.h"
#include "sdkconfig.h"
#include "esp_netif_sntp.h"
#include "lwip/ip_addr.h"
#include "esp_sntp.h"

#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN 48
#endif

namespace sntp
{
    static const char *TAG = "sntp";

    void time_sync_notification_cb(struct timeval *tv)
    {
            ESP_LOGI(TAG, "got Epoch = %lld", tv->tv_sec);
            time_t now;
            struct tm timeinfo;
            time(&now);

            char strftime_buf[64];

            // Set timezone to Eastern Standard Time and print local time

            localtime_r(&now, &timeinfo);
            strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
            ESP_LOGI(TAG, "Local time: %s", strftime_buf);
    }

    static void print_servers(void)
    {
            ESP_LOGI(TAG, "NTP servers:");

            for (uint8_t i = 0; i < SNTP_MAX_SERVERS; ++i)
            {
                    if (esp_sntp_getservername(i))
                    {
                            ESP_LOGI(TAG, "%d: %s", i, esp_sntp_getservername(i));
                    }
                    else
                    {
                            // we have either IPv4 or IPv6 address, let's print it
                            char buff[INET6_ADDRSTRLEN];
                            ip_addr_t const *ip = esp_sntp_getserver(i);
                            if (ipaddr_ntoa_r(ip, buff, INET6_ADDRSTRLEN) != NULL)
                                    ESP_LOGI(TAG, "server %d: %s", i, buff);
                    }
            }
    }

    void init()
    {
            ESP_LOGI(TAG, "init");
            ESP_LOGI(TAG, "CONFIG_TIMEZONE=%s", CONFIG_TIMEZONE);
            setenv("TZ", CONFIG_TIMEZONE, 1);
            tzset();
#if LWIP_DHCP_GET_NTP_SRV
        /**
         * NTP server address could be acquired via DHCP,
         * see following menuconfig options:
         * 'LWIP_DHCP_GET_NTP_SRV' - enable STNP over DHCP
         * 'LWIP_SNTP_DEBUG' - enable debugging messages
         *
         * NOTE: This call should be made BEFORE esp acquires IP address from DHCP,
         * otherwise NTP option would be rejected by default.
         */
        ESP_LOGI(TAG, "Initializing SNTP");
        esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG(CONFIG_SNTP_TIME_SERVER);
        config.start = false;                     // start SNTP service explicitly (after connecting)
        config.server_from_dhcp = true;           // accept NTP offers from DHCP server, if any (need to enable *before* connecting)
        config.renew_servers_after_new_IP = true; // let esp-netif update configured SNTP server(s) after receiving DHCP lease
        config.index_of_first_server = 1;         // updates from server num 1, leaving server 0 (from DHCP) intact
                                                  // configure the event on which we renew servers
#ifdef CONFIG_EXAMPLE_CONNECT_WIFI
        config.ip_event_to_renew = IP_EVENT_STA_GOT_IP;
#else
        config.ip_event_to_renew = IP_EVENT_ETH_GOT_IP;
#endif
        config.sync_cb = time_sync_notification_cb; // only if we need the notification function
        esp_netif_sntp_init(&config);

#endif /* LWIP_DHCP_GET_NTP_SRV */

#if LWIP_DHCP_GET_NTP_SRV
        ESP_LOGI(TAG, "Starting SNTP");
        esp_netif_sntp_start();
#if LWIP_IPV6 && SNTP_MAX_SERVERS > 2
        /* This demonstrates using IPv6 address as an additional SNTP server
         * (statically assigned IPv6 address is also possible)
         */
        ip_addr_t ip6;
        if (ipaddr_aton("2a01:3f7::1", &ip6))
        { // ipv6 ntp source "ntp.netnod.se"
            esp_sntp_setserver(2, &ip6);
        }
#endif /* LWIP_IPV6 */

#else
        ESP_LOGI(TAG, "Initializing and starting SNTP");
#if CONFIG_LWIP_SNTP_MAX_SERVERS > 1
        /* This demonstrates configuring more than one server
         */
        esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG_MULTIPLE(2,
                                                                          ESP_SNTP_SERVER_LIST(CONFIG_SNTP_TIME_SERVER, "pool.ntp.org"));
#else
        /*
         * This is the basic default config with one server and starting the service
         */
        esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG(CONFIG_SNTP_TIME_SERVER);
#endif
        config.sync_cb = time_sync_notification_cb; // Note: This is only needed if we want
        config.wait_for_sync = false;
        config.server_from_dhcp = true;
        ESP_ERROR_CHECK(esp_netif_sntp_init(&config));
#endif
        print_servers();
    }

}