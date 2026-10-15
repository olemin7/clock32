---
name: update-fw
description: Update the clock32 firmware on an attached ESP32-C3 using the safe flow get settings -> flash -> restore settings. Use when asked to update, build, flash or reflash the firmware, or when settings must be preserved across a flash.
disable-model-invocation: true
---

# Update clock32 firmware

Settings (display orientation, brightness curve, timezone, LDR range, MQTT URL) live in NVS, not in
the app partition. A plain `idf.py flash` keeps them, so the restore step is normally a verification.
Snapshot anyway: the values are unrecoverable if NVS is ever erased, and a wrong `segment_rotation`
makes the display unreadable.

## Checklist

```
- [ ] Step 1: Identify device (port + MAC) and broker
- [ ] Step 2: Snapshot settings BEFORE flashing
- [ ] Step 3: Build and flash
- [ ] Step 4: Read settings back and compare
- [ ] Step 5: Restore only if they differ
- [ ] Step 6: Verify the device runs
```

## Step 1: Identify device

```bash
ls -l /dev/ttyACM*                      # port, usually /dev/ttyACM0
mosquitto_sub -h "$BROKER" -t 'devices/#' -v -W 70   # MAC appears as devices/<MAC>/<field>
```

Known MACs and the broker address are listed under `# cmd list` in `README.md`. The MAC is also
visible in the serial log inside the MQTT topic names.

## Step 2: Snapshot settings

**Never skip this and never flash first.** A snapshot taken after flashing is worthless.

```bash
scripts/device_cfg.sh get <MAC> <BROKER> /tmp/clock_cfg_<MAC>.json
```

Store the file outside the repo (`/tmp`) and show its contents to the user before continuing.
If the broker is unreachable, stop and ask the user how to proceed - do not flash blind.

## Step 3: Build and flash

The build needs the ESP-IDF env plus the path to `esp32_idf_libs`, otherwise kconfgen fails with
`'/provision/provision.projbuild' not found`:

```bash
source ~/esp/v5.5.1/esp-idf/export.sh
export MY_ESP_LIBRARIES_LOCAL=+ MY_ESP_LIBRARIES_PATH=/tmp/esp32_idf_libs
idf.py -p /dev/ttyACM0 flash
```

`MY_ESP_LIBRARIES_PATH` is appended to the project root. Use `/build/_deps/my_idf_libs-src` (the
FetchContent copy) when not working against a local checkout of the libs.

Use `flash`, never `erase-flash` or `erase_flash`: those wipe NVS, dropping both the settings and the
Wi-Fi provisioning, which then requires a re-provision over BLE.

## Step 4: Read back and compare

Wait for the device to rejoin Wi-Fi and MQTT (~30 s after reset), then:

```bash
scripts/device_cfg.sh get <MAC> <BROKER> /tmp/clock_cfg_<MAC>.after.json
diff <(jq -S . /tmp/clock_cfg_<MAC>.json) <(jq -S . /tmp/clock_cfg_<MAC>.after.json)
```

## Step 5: Restore if needed

Empty diff means NVS survived - report that and skip the write. Otherwise:

```bash
scripts/device_cfg.sh set <MAC> <BROKER> /tmp/clock_cfg_<MAC>.json
```

Then repeat step 4 to confirm the values stuck.

## Step 6: Verify

```bash
stty -F /dev/ttyACM0 115200 raw -echo    # MUST be re-run after every reset
timeout 120 cat /dev/ttyACM0 > /tmp/boot.txt
grep -aE "reset reason|SvitloBot|IP Address" /tmp/boot.txt
```

The USB CDC re-enumerates on reset, so a `cat` without a fresh `stty` returns 0 bytes immediately.
Only one reader may hold the port: a running `idf.py monitor` or `cat` blocks the others.

Report the reset reason, the obtained IP and the first SvitloBot ping result.

## Settings reference

`{"cmd":"config"}` returns every command registered with `is_config=true`, currently `ldr`,
`brightness`, `display`, `timezone` and `mqtt`, and the same payload shape restores all of them in
one request. Per-setting commands (`{"cmd":"display"}`, `{"cmd":"brightness"}`, ...) work as a
fallback; sending one with no payload reads it, sending one with a payload writes it.

Full command list and MQTT topic layout: `# cmd list` in `README.md`.
