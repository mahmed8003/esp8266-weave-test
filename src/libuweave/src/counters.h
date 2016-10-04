// Copyright 2016 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBUWEAVE_SRC_COUNTER_H_
#define LIBUWEAVE_SRC_COUNTER_H_

#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

#include "src/time.h"
#include "uweave/counters.h"
#include "uweave/provider/time.h"
#include "uweave/status.h"
#include "uweave/value.h"

/**
 * Determines how frequently uWeave tries to write dirty counters to storage.
 */
static const time_t kUwCounterCoalesceIntervalSeconds = 10;

/** Internal counter ids.  Expected to be continuous. */
typedef enum {
  kUwInternalCounterBleConnect = 0,
  kUwInternalCounterBleDisconnect = 1,
  kUwInternalCounterAuthPairing = 2,
  kUwInternalCounterAuthToken = 3,
  kUwInternalCounterAccessControlClaim = 4,
  kUwInternalCounterAccessControlConfirm = 5,
  kUwInternalCounterSetupTimeSet = 6,
  kUwInternalCounterSessionHandshakeFailure = 7,
  kUwInternalCounterSessionDecryptionFailure = 8,
  kUwInternalCounterSessionEncryptionFailure = 9,
  kUwInternalCounterPrivetDispatch = 10,
  kUwInternalCounterFactoryReset = 11,
  kUwInternalCounterLast
} UwInternalCounter;

typedef struct {
  UwCounterId id;
  UwCounterValue value;
} UwCounter;

struct UwCounterSet_ {
  // Nonce value to identify factory resets.
  uint32_t generation_id;
  // Set to the first instantiation of the counter set.  Used to distinguish a
  // factory reset for aggregation purposes.
  time_t generation_time;
  // Track the earliest un-persisted change for coalescing.
  time_t earliest_change_time;
  // uWeave counters.
  uint16_t uw_counter_count;
  UwCounter* uw_counters;
  // Application specific counters.
  uint16_t app_counter_count;
  UwCounter* app_counters;
};

static inline void uw_counter_set_increment_uw_counter_(
    UwCounterSet* counter_set,
    UwInternalCounter id) {
  if (counter_set->earliest_change_time == 0) {
    counter_set->earliest_change_time = uw_time_get_uptime_seconds_();
  }
  if (id >= kUwInternalCounterLast) {
    assert(false);
  }
  ++counter_set->uw_counters[id].value;
}

static inline UwCounterValue uw_counter_set_get_uw_counter_(
    UwCounterSet* counter_set,
    UwInternalCounter id) {
  if (id >= kUwInternalCounterLast) {
    assert(false);
  }
  return counter_set->uw_counters[id].value;
}

static inline void uw_counter_set_increment_app_counter_(
    UwCounterSet* counter_set,
    UwCounterId app_counter_id) {
  if (counter_set->earliest_change_time == 0) {
    counter_set->earliest_change_time = uw_time_get_uptime_seconds_();
  }
  for (int i = 0; i < counter_set->app_counter_count; ++i) {
    if (counter_set->app_counters[i].id == app_counter_id) {
      ++counter_set->app_counters[i].value;
      return;
    }
  }
  assert(false);
}

static inline UwCounterValue uw_counter_set_get_app_counter_(
    UwCounterSet* counter_set,
    UwCounterId app_counter_id) {
  for (int i = 0; i < counter_set->app_counter_count; ++i) {
    if (counter_set->app_counters[i].id == app_counter_id) {
      return counter_set->app_counters[i].value;
    }
  }
  assert(false);
  return kUwCounterValueInvalid;
}

typedef UwStatus (*UwCountersValueCallback)(void* data, const UwValue* value);

/**
 * Encodes the counter set as a UwValue and invokes the callback while the
 * value is alive on the stack.  Updates the generation_time on the counter_set
 * if it is unset.
 */
UwStatus uw_counter_set_encode_(UwCounterSet* counter_set,
                                UwCountersValueCallback callback,
                                void* callback_data);

UwStatus uw_counter_set_serialize_(UwCounterSet* counter_set,
                                   uint8_t* buf,
                                   size_t buf_len,
                                   size_t* result_len);

UwStatus uw_counter_set_deserialize_(UwCounterSet* counter_set,
                                     const uint8_t* buf,
                                     size_t buf_len);

UwStatus uw_counter_set_write_to_storage_(UwCounterSet* counter_set);

UwStatus uw_counter_set_read_from_storage_(UwCounterSet* counter_set);

UwStatus uw_counter_set_try_coalesce_(UwCounterSet* counter_set);

#endif  // LIBUWEAVE_SRC_COUNTER_H_
