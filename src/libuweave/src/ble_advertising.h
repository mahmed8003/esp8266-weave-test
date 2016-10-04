// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBUWEAVE_SRC_BLE_ADVERTISING_H_
#define LIBUWEAVE_SRC_BLE_ADVERTISING_H_

#include <stdint.h>

#include "src/device.h"

typedef struct UwBleAdvertisingData_ {
  uint8_t bytes[20];
} UwBleAdvertisingData;

// Setup state flags.
static const uint8_t kUwBleAdvertisingFlagNeedsWiFiSetup = (1 << 0);
static const uint8_t kUwBleAdvertisingFlagNeedsWeaveRegistration = (1 << 1);

// Capability flags.
static const uint8_t kUwBleAdvertisingFlagWiFi24Ghz = (1 << 0);
static const uint8_t kUwBleAdvertisingFlagWiFi50Ghz = (1 << 1);
static const uint8_t kUwBleAdvertisingFlagBle40 = (1 << 2);

/**
 * Force an update of the advertising payload to match the current device
 * state.
 */
bool uw_ble_advertising_update_data_(UwDevice* device);

/**
 * Get the raw advertising payload for the current device state, exposed for
 * testing.
 */
void uw_ble_advertising_get_data_(UwDevice* device, UwBleAdvertisingData* data);

#endif  // LIBUWEAVE_SRC_BLE_ADVERTISING_H_
