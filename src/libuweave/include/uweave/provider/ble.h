// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBUWEAVE_INCLUDE_UWEAVE_PROVIDER_BLE_H_
#define LIBUWEAVE_INCLUDE_UWEAVE_PROVIDER_BLE_H_

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

#include "uweave/ble_transport.h"
#include "uweave/device.h"
#include "uweave/gatt.h"

/**
 * This defines the platform-specific interface to board. To support a new
 * board, you would implement these methods and link your provider with your app
 * and libuweave.a.
 */

/**
 * Initializes any global state for the ble library at startup.  No other uw
 * or uwp functions may be invoked.  Functionality that is restricted to when
 * the service is enabled should be kept in uwp_ble_create_service().
 */
bool uwp_ble_init();

/** Sets advertising data payload for Weave clients to discovery. */
bool uwp_ble_set_advertising_data(const char* device_name,
                                  const uint16_t manufacturer_id,
                                  const uint8_t* manufacturer_data,
                                  const uint8_t manufacturer_data_length);

/** Starts BLE advertising. */
bool uwp_ble_advertising_start();

/** Stops BLE advertising. */
bool uwp_ble_advertising_stop();

/** Creates service required for Weave BlE communication. */
bool uwp_ble_create_service(const UwBleService* service,
                            UwBleTransport* transport);

/**
 * If an event is pending, copies the data into the event pointer and returns
 * true.  Otherwise returns false.
 *
 * Each event includes an opaque connection_handle.  Over the lifetime of the
 * session, the uWeave library expects to read a sequence of a
 * kUwBleEventTypeConnected, kUwBleEventTypeData, kUwBleEventTypeDisconnected
 * events with the same connection_handle.
 */
bool uwp_ble_read_event(UwBleEvent* packet);

/** Returns true if you can write a BLE packet. */
bool uwp_ble_can_write_packet();

/** Writes a data packet. */
bool uwp_ble_write_packet(UwBleEvent* packet);

/** Disconnects the session associated with the connection_handle. */
void uwp_ble_disconnect(UwBleOpaqueConnectionHandle connection_handle);

/** Updates the device name. */
bool uwp_ble_set_device_name(const char* device_name);

#endif  // LIBUWEAVE_INCLUDE_UWEAVE_PROVIDER_BLE_H_
