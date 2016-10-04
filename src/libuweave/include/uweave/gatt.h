// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBUWEAVE_INCLUDE_UWEAVE_GATT_H_
#define LIBUWEAVE_INCLUDE_UWEAVE_GATT_H_

#include <stdint.h>

#include "uweave/config.h"

#define UW_BLE_LONG_UUID_SIZE 16

/** Type to hold a UUID in long and short form. */
typedef struct {
  uint8_t long_uuid[UW_BLE_LONG_UUID_SIZE];
  uint16_t short_uuid;
} UwBleUuid;

/** Configuration options for BLE characteristics. */
typedef enum {
  kUwBlePropertyOptionRead = 1 << 0,
  kUwBlePropertyOptionWrite = 1 << 1,
  kUwBlePropertyOptionWriteNoResponse = 1 << 2,
  kUwBlePropertyOptionNotify = 1 << 3,
  kUwBlePropertyOptionIndicate = 1 << 4
} UwBlePropertyOptions;

/** Definiton for a BLE characterstic. */
typedef struct {
  UwBleUuid uuid;
  UwBlePropertyOptions properties;
  uint16_t value_length;
} UwBleCharacteristic;

/** Definition of a BLE service. */
typedef struct {
  UwBleUuid uuid;
  UwBleCharacteristic* characteristics;
  uint16_t characteristics_count;
} UwBleService;

typedef enum {
  kUwBleEventTypeData,
  kUwBleEventTypeConnection,
  kUwBleEventTypeDisconnection,
} UwBleEventType;

/**
 * An opaque handle given byt he provider to specify which connection data was
 * received on or should be sent to.
 */
typedef uint16_t UwBleOpaqueConnectionHandle;

/** Data for 1 MTU sized packet. */
typedef struct {
  uint16_t data_length;
  uint8_t data[UW_BLE_PACKET_SIZE];
} UwBlePacket;

typedef struct {
  UwBleEventType event_type;
  UwBleOpaqueConnectionHandle connection_handle;
  UwBlePacket packet;
} UwBleEvent;

/**
 * Weave base UUID for BLE UUID's in big endian format.
 * 00000000-0004-1000-8000-001A11000100
 */
extern const UwBleUuid UwBaseUuid;

/**
 * Weave service UUID in big endian format.
 * 00000100-0004-1000-8000-001A11000100
 */
extern const UwBleUuid UwServiceUuid;

/**
 * RX Characteristic UUID in big endian format.
 * 00000102-0004-1000-8000-001a11000100
 *
 * A client reads from this characteristic to receive data.
 */
extern const UwBleUuid UwClientRxCharacteristicUuid;

/**
 * TX Characteristic UUID in big endian format.
 * 00000101-0004-1000-8000-001a11000100
 *
 * A client writes to this characteristic to transmit data.
 */
extern const UwBleUuid UwClientTxCharacteristicUuid;

#endif  // LIBUWEAVE_INCLUDE_UWEAVE_GATT_H_
