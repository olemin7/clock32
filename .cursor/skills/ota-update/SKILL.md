---
name: ota-update
description: Push a clock32 firmware update over the air (no USB cable) via MQTT and a LAN HTTP image server. Use when asked to OTA, push, or deploy firmware to a clock that already runs the two-slot OTA layout, or to update all clocks remotely.
disable-model-invocation: true
---

# OTA update for clock32

Pushes `build/clock32.bin` to a device over Wi-Fi. Requires firmware with the `ota` command
(two-slot layout, slots `ota_0`/`ota_1`). Devices still on the old single-`factory` layout must be
migrated once over USB first - use the `update-fw` skill for that.

## Quick flow

```bash
source ~/esp/v5.5.1/esp-idf/export.sh
export MY_ESP_LIBRARIES_LOCAL=+ MY_ESP_LIBRARIES_PATH=/tmp/esp32_idf_libs
idf.py build
scripts/ota_push.sh <MAC> <BROKER>          # does everything below automatically
```

`ota_push.sh` serves the image, records the current slot, triggers the update, polls status until
the slot flips, and cleans up. Typical values: broker `192.168.1.2`, MACs under `# cmd list` in
`README.md`.

## What the script automates (manual equivalent)

```bash
(cd build && python3 -m http.server 8070 --bind 0.0.0.0 &)
mosquitto_pub -t cmd/<MAC> -h <BROKER> -m '{"cmd":"ota","payload":{"url":"http://<host-ip>:8070/clock32.bin"}}'
mosquitto_pub -t cmd/<MAC> -h <BROKER> -m '{"cmd":"ota"}'    # poll: slot/built/state
```

Download takes ~35 s, then the device restarts and reconnects in ~30 s. Success criteria:
`slot:` flipped (`ota_0` <-> `ota_1`), `state:idle`, and the device is back on MQTT (which also
means `ota: image confirmed, rollback cancelled` ran - a crash-looping image would have rolled
back to the previous slot instead).

## Gotchas

- **Firewall**: the device pulls the image from this host, so the port must be open:
  `sudo ufw allow from 192.168.1.0/24 to any port 8070 proto tcp`. The symptom of a blocked port
  is `last:begin failed: ESP_ERR_HTTP_CONNECT` in the status (and no GET in the server log);
  the running image stays untouched.
- **Status payload is not JSON**: the reply is `{"payload":slot:ota_0 built:... state:...}` - parse
  it with grep/sed, not jq.
- **Size limit**: the image must stay under one slot (0x1F0000, ~1.98 MB); `idf.py build` fails the
  size check otherwise. Current levers already applied: -Os, no cert bundle, no IPv6. Next lever:
  Bluedroid -> NimBLE.
- A second `ota` command while one runs is rejected (`ota rejected: ... state:updating`).
- Settings live in NVS and are not touched by OTA; a snapshot
  (`../update-fw/scripts/device_cfg.sh get ...`) is still cheap insurance.
- USB flash (`update-fw` skill) remains the recovery path for an image that boots but misbehaves.
