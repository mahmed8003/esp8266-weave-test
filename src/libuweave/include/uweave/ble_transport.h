// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBUWEAVE_INCLUDE_UWEAVE_BLE_TRANSPORT_H_
#define LIBUWEAVE_INCLUDE_UWEAVE_BLE_TRANSPORT_H_

#include <stdint.h>

#include "uweave/device.h"
#include "uweave/settings.h"

typedef struct UwBleTransport_ UwBleTransport;

/**
 * Initializes a BLE transport service to handle BLE input and registers it in
 * the runloop.
 */
bool uw_ble_transport_init(UwBleTransport* transport, UwDevice* device);

/** Gets the size of the UwBleTransport struct. */
size_t uw_ble_transport_sizeof();

/** Updates the BLE advertising data. */
bool uw_ble_update_ble_advertising_data(UwSettings* settings);

/**
 * Notify the transport and device that work is available.
 *
 * See uw_device_notify_work.
 */
void uw_ble_transport_notify_work(UwBleTransport* ble_transport);

/**
 * Notifies the transport that activity has occurred that should be considered
 * non-idle.
 */
void uw_ble_transport_notify_activity(UwBleTransport* ble_transport);

#endif  // LIBUWEAVE_INCLUDE_UWEAVE_BLE_TRANSPORT_H_
