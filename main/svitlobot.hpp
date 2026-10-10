/*
 * Copyright (c) 2026
 *
 * SvitloBot keep-alive: periodic channelPing so the SvitloBot channel
 * reports power state of the place where the clock is plugged in.
 */

#pragma once

#include <string>

namespace svitlobot
{
    std::string make_url(const std::string &base, const std::string &key);

    // Channel key is a secret: only the masked form may appear in logs.
    std::string mask_key(const std::string &key);

    // Empty when main/svitlobot_secret.h is missing or defines an empty key.
    const std::string &key();

    // Blocking single request; returns true on HTTP 200.
    bool ping();

    // Starts the background task; no-op (with a warning) if the key is empty.
    void init();

    // Called from Wi-Fi/IP event handlers; safe before/without init().
    void set_online(bool online);
} // namespace svitlobot
