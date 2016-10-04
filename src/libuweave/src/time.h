// Copyright 2016 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
#ifndef LIBUWEAVE_SRC_TIME_H_
#define LIBUWEAVE_SRC_TIME_H_

#include <stdint.h>
#include <time.h>

#include "uweave/status.h"

typedef enum {
  kUwTimeSourceOwner,
  kUwTimeSourceVerified,
} UwTimeSource;

typedef enum {
  kUwTimeStatusOk = 0,
  kUwTimeStatusDegraded = 1,
  kUwTimeStatusInvalid = 2,
} UwTimeStatus;

/**
 * Returns the current time in unsigned seconds from the unix epoch.
 *
 * Note that this is an unsigned value.  If an application needs to store
 * timestamps for historical times before 1970-1-1, then a signed 64-bit integer
 * should be used rather than a signed 32-bit integer to avoid confusion with
 * the wraparound on that will occur in 2038.
 */
time_t uw_time_get_timestamp_seconds_();

/**
 * Returns the status of the time value.
 *
 * Returns:
 *   - kUwTimeStatusInvalid if the time is unset or has not been set for too
 *   long.  When invalid the device will reject any authentication that has a
 *   non-owner time-expiring token.
 *   - kUwTimeStatusDegraded if an intermediate period of time has elapsed since
 *   the time was last set and it should be refreshed to avoid the Invalid
 *   state.
 *   - kUwTimeStatusOk if the time has been set recently.
 */
UwTimeStatus uw_time_get_status_();

/**
 * Set the current time.  If the source is owner or verified, then the time is
 * accepted unconditionally and it will reset the error bound.  If the source is
 * user the function will return failure if the change is outside of the allowed
 * bound.
 */
UwStatus uw_time_set_timestamp_seconds_(time_t timestamp, UwTimeSource source);

/**
 * Return the uptime of the device.  Guaranteed not to change across
 * set_timestamp_seconds_ calls.
 */
time_t uw_time_get_uptime_seconds_();

#endif  // LIBUWEAVE_SRC_TIME_H_
