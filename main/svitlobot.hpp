/*
 * Copyright (c) 2026
 *
 * SvitloBot keep-alive: periodic channelPing so the SvitloBot channel
 * reports power state of the place where the clock is plugged in.
 */

#pragma once

#include <stdint.h>
#include <string>

namespace svitlobot
{
    std::string make_url(const std::string &base, const std::string &key);

    // Delay before the next attempt after a failed ping; 0 means "first retry".
    // Doubles on every consecutive failure and is capped well below the ping
    // interval, so retries never push the channel into the offline state.
    uint32_t retry_delay_ms(uint32_t previous_ms);

    // Maps a random value onto the configured startup jitter window
    // [0, CONFIG_SVITLOBOT_STARTUP_JITTER_S), 0 when jitter is disabled.
    uint32_t startup_jitter_ms(uint32_t random_value);

    // Channel key is a secret: only the masked form may appear in logs.
    std::string mask_key(const std::string &key);

    // Empty when main/svitlobot_secret.h is missing or defines an empty key.
    const std::string &key();

    enum class ping_result
    {
        ok,        // HTTP 200: this device updated the channel timestamp.
        throttled, // HTTP 429: the channel was already fresh, typically because
                   // a sibling device sharing the key reported just before us.
                   // Not an error: the location still counts as powered.
        failed,    // Transport error, timeout or any other HTTP status.
    };

    // Blocking single request.
    ping_result ping();

    // Valid range for the runtime ping period. The floor is the API limit of
    // one recorded ping per minute per channel.
    constexpr uint32_t INTERVAL_MIN_S = 65;
    constexpr uint32_t INTERVAL_MAX_S = 600;

    bool interval_valid(uint32_t seconds);

    // Runtime settings, persisted in NVS. Several clocks share one channel key,
    // so pinging is enabled on one of them only (off by default) to keep the
    // total request rate of the fleet low.
    void set_enabled(bool enabled);
    bool is_enabled();
    // Returns false and keeps the old value when out of range.
    bool set_interval_s(uint32_t seconds);
    uint32_t interval_s();

    // Starts the background task; no-op (with a warning) if the key is empty.
    void init();

    // Called from Wi-Fi/IP event handlers; safe before/without init().
    void set_online(bool online);
} // namespace svitlobot
