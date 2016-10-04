// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBUWEAVE_SRC_BLE_TRANSPORT_H_
#define LIBUWEAVE_SRC_BLE_TRANSPORT_H_

#include "uweave/ble_transport.h"
#include "src/session.h"

UwSession* uw_ble_transport_get_session_(UwBleTransport* ble_transport);

#endif  // LIBUWEAVE_SRC_BLE_TRANSPORT_H_
