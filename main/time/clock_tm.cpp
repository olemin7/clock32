
#include "clock_tm.hpp"
#include <sys/time.h>
#include "esp_log.h"

namespace clock_tm
{
    static const char *TAG = "clock_tm";
    clock::clock(cb_t &&cb) : minute_timer_([this, cb_ = std::move(cb)]()
                                            {
    time_t now;
    struct tm timeinfo;
    time(&now);

    char strftime_buf[64];

    localtime_r(&now, &timeinfo);
    minute_timer_.start(std::chrono::seconds(60 - timeinfo.tm_sec));

    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(TAG, "Local time: %s", strftime_buf);    

    cb_(timeinfo); })
    {
        ESP_LOGI(TAG, "CONFIG_TIMEZONE=%s", CONFIG_TIMEZONE);
        setenv("TZ", CONFIG_TIMEZONE, 1);
        tzset();

        minute_timer_.start(std::chrono::seconds(1));
    }
}