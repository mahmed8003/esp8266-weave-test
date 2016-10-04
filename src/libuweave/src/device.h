// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBUWEAVE_SRC_DEVICE_H_
#define LIBUWEAVE_SRC_DEVICE_H_

#include "src/device_crypto.h"
#include "src/trace.h"
#include "uweave/device.h"
#include "uweave/status.h"

// Forward declarations to avoid circular dependencies.
struct UwPrivetRequest_;
struct UwService_;
struct UwSession_;

struct UwDevice_ {
  UwSettings* settings;
  UwDeviceHandlers* device_handlers;
  UwDeviceWorkState work_state;
  UwDeviceCrypto device_crypto;
  UwCommandList* command_list;
  UwCounterSet* counter_set;
  struct UwService_* first_service;
  void* context;
  UwTraceLog trace_log;
};

/**
 * Registers a new service on this device.
 */
void uw_device_register_service_(UwDevice* device, struct UwService_* service);

/**
 * Conducts a message exchange with the device.
 *
 * Given the `session` and an inbound message in `request`, the device
 * will synchronously construct a reply in the provided `reply` buffer.
 *
 * Returns kUwStatusSuccess unless there was an error in dispatching that would
 * warrant breaking the session.
 */
UwStatus uw_device_message_exchange_(UwDevice* device,
                                     struct UwSession_* session,
                                     UwBuffer* request,
                                     UwBuffer* reply);

/**
 * Dispatches the request to the correct Privet API handler.
 *
 * Returns kUwStatusSuccess unless there was an error in dispatching that would
 * warrant breaking the session (e.g., an invalid Privet request or a
 * library-level marshalling error).
 */
UwStatus uw_device_dispatch_request_(UwDevice* device,
                                     struct UwPrivetRequest_* request);

void uw_device_increment_uw_counter_(UwDevice* device, UwCounterId id);
uint32_t uw_device_get_uw_counter_(UwDevice* device, UwCounterId id);

#endif  // LIBUWEAVE_SRC_DEVICE_H_
