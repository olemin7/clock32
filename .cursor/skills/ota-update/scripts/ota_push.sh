#!/usr/bin/env bash
#
# OTA-push build/clock32.bin to a clock32 device over MQTT.
#
#   ota_push.sh <MAC> <BROKER> [PORT]
#   e.g. ota_push.sh 64E833885558 192.168.1.2
#
# Serves the image over HTTP, triggers {"cmd":"ota"}, then polls the status
# until the running slot flips. Exits non-zero if the update did not complete.
set -euo pipefail

usage() {
    echo "usage: $0 <MAC> <BROKER> [PORT]" >&2
    exit 2
}
[ $# -ge 2 ] || usage

mac=$1
broker=$2
port=${3:-8070}

readonly REPLY_TIMEOUT_S=15
readonly DOWNLOAD_POLL_S=10
# download ~35 s + reboot + Wi-Fi/MQTT rejoin ~40 s, with margin
readonly COMPLETE_TIMEOUT_S=240

script_dir=$(cd "$(dirname "$0")" && pwd)
repo_root=$(cd "$script_dir/../../../.." && pwd)
bin="$repo_root/build/clock32.bin"

for tool in mosquitto_sub mosquitto_pub python3 curl; do
    command -v "$tool" >/dev/null || { echo "error: $tool is not installed" >&2; exit 1; }
done
[ -f "$bin" ] || { echo "error: $bin not found - run idf.py build first" >&2; exit 1; }

# Host address the device can reach: the interface on the broker's /24.
subnet=$(echo "$broker" | cut -d. -f1-3)
host_ip=$(ip -4 addr show scope global | grep -oE "inet $subnet\.[0-9]+" | head -1 | cut -d' ' -f2)
[ -n "$host_ip" ] || { echo "error: no interface on subnet $subnet.0/24" >&2; exit 1; }

# One command/reply round-trip. The device answers on response/<MAC>, so the
# subscriber must be listening before the publish.
request() {
    local payload=$1 out
    out=$(mktemp)
    mosquitto_sub -h "$broker" -t "response/$mac" -C 1 -W "$REPLY_TIMEOUT_S" >"$out" 2>/dev/null &
    local sub=$!
    sleep 2
    mosquitto_pub -h "$broker" -t "cmd/$mac" -m "$payload"
    wait $sub 2>/dev/null || true
    cat "$out"
    rm -f "$out"
}

# The ota status payload is NOT valid JSON ({"payload":slot:ota_0 ...}): grep it.
slot_of()  { grep -oE "slot:[A-Za-z0-9_]+"  <<<"$1" | cut -d: -f2; }
state_of() { grep -oE "state:[A-Za-z]+"     <<<"$1" | cut -d: -f2; }

status=$(request '{"cmd":"ota"}')
slot_before=$(slot_of "$status")
[ -n "$slot_before" ] || { echo "error: no ota status from $mac - old firmware? Migrate over USB first (update-fw skill)." >&2; exit 1; }
echo "device $mac before: $status"

httpd_pid=""
cleanup() { [ -n "$httpd_pid" ] && kill "$httpd_pid" 2>/dev/null || true; }
trap cleanup EXIT

(cd "$repo_root/build" && exec python3 -m http.server "$port" --bind 0.0.0.0 >/dev/null 2>&1) &
httpd_pid=$!
sleep 1
curl -s -o /dev/null --max-time 5 "http://$host_ip:$port/clock32.bin" ||
    { echo "error: cannot fetch http://$host_ip:$port/clock32.bin myself - port in use?" >&2; exit 1; }

url="http://$host_ip:$port/clock32.bin"
echo "triggering update from $url"
reply=$(request "{\"cmd\":\"ota\",\"payload\":{\"url\":\"$url\"}}")
echo "device reply: $reply"
grep -q "state:updating" <<<"$reply" ||
    { echo "error: device did not start updating" >&2; exit 1; }

elapsed=0
while [ $elapsed -lt $COMPLETE_TIMEOUT_S ]; do
    sleep "$DOWNLOAD_POLL_S"; elapsed=$((elapsed + DOWNLOAD_POLL_S))
    # During download and reboot the device may not answer: that is expected.
    status=$(request '{"cmd":"ota"}' || true)
    [ -n "$status" ] || { echo "  ...no reply (downloading/rebooting), ${elapsed}s"; continue; }
    slot_now=$(slot_of "$status")
    state_now=$(state_of "$status")
    echo "  ...slot:$slot_now state:$state_now, ${elapsed}s"
    if [ "$slot_now" != "$slot_before" ] && [ "$state_now" = "idle" ]; then
        echo "OK: $mac updated, slot $slot_before -> $slot_now, back on MQTT (image confirmed)"
        echo "final status: $status"
        exit 0
    fi
    if grep -q "ESP_ERR_HTTP_CONNECT" <<<"$status"; then
        echo "error: device cannot reach $url - firewall? try:" >&2
        echo "  sudo ufw allow from $subnet.0/24 to any port $port proto tcp" >&2
        exit 1
    fi
done
echo "error: slot did not flip within ${COMPLETE_TIMEOUT_S}s; last status: $status" >&2
exit 1
