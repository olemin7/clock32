#!/usr/bin/env bash
#
# Read or write the cumulative clock32 settings over MQTT.
#
#   device_cfg.sh get <MAC> <BROKER> <FILE>
#   device_cfg.sh set <MAC> <BROKER> <FILE>
#
# The payload is the response of {"cmd":"config"}, which covers every command
# registered with is_config=true (ldr, brightness, display, timezone, mqtt).
set -euo pipefail

usage() {
    echo "usage: $0 {get|set} <MAC> <BROKER> <FILE>" >&2
    echo "   e.g. $0 get 64E8338811DC 192.168.1.2 /tmp/cfg.json" >&2
    exit 2
}

[ $# -eq 4 ] || usage

action=$1
mac=$2
broker=$3
file=$4

# The device answers on response/<MAC> without echoing the request, so the
# subscriber has to be listening before the command is published.
readonly RESPONSE_TIMEOUT_S=15

for tool in mosquitto_sub mosquitto_pub jq; do
    command -v "$tool" >/dev/null || { echo "error: $tool is not installed" >&2; exit 1; }
done

# Fails fast on a down broker instead of silently producing an empty snapshot.
mosquitto_pub -h "$broker" -t "clock32/probe" -m "" >/dev/null 2>&1 ||
    { echo "error: broker $broker is unreachable" >&2; exit 1; }

request() {
    local payload=$1 out
    out=$(mktemp)
    mosquitto_sub -h "$broker" -t "response/$mac" -C 1 -W "$RESPONSE_TIMEOUT_S" >"$out" 2>/dev/null &
    local sub=$!
    sleep 2 # let the subscription settle, otherwise the reply is missed
    mosquitto_pub -h "$broker" -t "cmd/$mac" -m "$payload"
    wait $sub 2>/dev/null || true
    cat "$out"
    rm -f "$out"
}

case $action in
get)
    response=$(request '{"cmd":"config"}')
    [ -n "$response" ] || { echo "error: no response from $mac within ${RESPONSE_TIMEOUT_S}s" >&2; exit 1; }
    # Write via a temp file: a half-written or null snapshot must never end up
    # under the real name, because the next step is an irreversible flash.
    tmp=$(mktemp)
    trap 'rm -f "$tmp"' EXIT
    # Unwrap the {"payload":{...}} envelope so the file can be fed back verbatim.
    if ! echo "$response" | jq -e '.payload' >"$tmp"; then
        echo "error: unexpected response from $mac: $response" >&2
        exit 1
    fi
    mv "$tmp" "$file"
    trap - EXIT
    echo "saved settings of $mac to $file"
    jq -S . "$file"
    ;;
set)
    [ -f "$file" ] || { echo "error: $file does not exist" >&2; exit 1; }
    jq -e . "$file" >/dev/null 2>&1 || { echo "error: $file is not valid JSON" >&2; exit 1; }
    payload=$(jq -c --slurpfile cfg "$file" -n '{cmd:"config", payload:$cfg[0]}')
    response=$(request "$payload")
    [ -n "$response" ] || { echo "error: no response from $mac within ${RESPONSE_TIMEOUT_S}s" >&2; exit 1; }
    echo "restored settings of $mac from $file, device reports:"
    echo "$response" | jq -S '.payload // .'
    ;;
*)
    usage
    ;;
esac
