#!/usr/bin/env bash
# Enforces the dependency rule (see docs/architecture.md):
#  1. Nothing under firmware/core/ includes hardware, RTOS, BLE, or adapter
#     headers. esp_err.h is the only ESP-IDF header the core may use.
#  2. No adapter includes another adapter's header (ble_mobiledetect may
#     include mobiledetect_protocol.h, the one allowed exception).
# Exits non-zero and prints file:line for every violation.
set -euo pipefail

cd "$(dirname "$0")/../firmware"
fail=0

report() {  # report <file:line:text> <reason>
    echo "VIOLATION: $1  ($2)"
    fail=1
}

# --- 1. core ---
core_forbidden='freertos/|driver/|esp_driver|esp_log|nimble|host/ble|nvs|i2c_bus|led_gpio|as7341|mock_sensor|ble_mobiledetect|log_sink|mobiledetect_protocol|console_input'
while IFS= read -r hit; do
    report "$hit" "core must not include this"
done < <(grep -rnE "^[[:space:]]*#[[:space:]]*include[[:space:]]*[<\"][^>\"]*($core_forbidden)" core || true)

# Any other ESP-IDF header (esp_*.h other than esp_err.h).
while IFS= read -r hit; do
    report "$hit" "core may only include esp_err.h from ESP-IDF"
done < <(grep -rnE "^[[:space:]]*#[[:space:]]*include[[:space:]]*[<\"]esp_" core | grep -vE "[<\"]esp_err\.h[>\"]" || true)

# --- 2. adapters ---
adapters=$(ls adapters)
for a in $adapters; do
    for other in $adapters; do
        [ "$a" = "$other" ] && continue
        [ "$a" = "ble_mobiledetect" ] && [ "$other" = "mobiledetect_protocol" ] && continue
        while IFS= read -r hit; do
            report "$hit" "adapter $a must not include adapter $other"
        done < <(grep -rnE "^[[:space:]]*#[[:space:]]*include[[:space:]]*[<\"]([^>\"]*/)?$other\.h[>\"]" "adapters/$a" || true)
    done
done

if [ "$fail" -ne 0 ]; then
    echo "Layer check FAILED."
    exit 1
fi
echo "Layer check passed."
