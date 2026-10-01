// Result sink that sends results to MobileDetect as BLE GATT notifications.
// The only component that knows about NimBLE.
#pragma once
#include "result_sink.h"

// Initializes NVS and NimBLE and starts advertising. Returns NULL on failure.
result_sink_t *ble_mobiledetect_create(void);
