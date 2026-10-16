/*
 * Copyright (c) 2026
 *
 * OTA update: pulls a firmware image from a LAN HTTP(S) URL into the inactive
 * app slot, triggered by the "ota" proto command (MQTT or the /cmd endpoint).
 */

#pragma once

#include <string>

namespace ota
{
    // Spawns the update task; returns false if an update is already running,
    // the URL is empty, or the task could not be created.
    bool start(const std::string &url);

    bool in_progress();

    // Running slot, app version and update state; reply for the proto command.
    std::string status();

    // Cancels the bootloader rollback once the app has proven itself (Wi-Fi up,
    // MQTT connected). Without this call the next reset boots the previous
    // image again. Safe to call repeatedly and when no rollback is pending.
    void mark_valid();
} // namespace ota
