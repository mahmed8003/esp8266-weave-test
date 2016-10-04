// Copyright 2016 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBUWEAVE_INCLUDE_UWEAVE_PROVIDER_TIME_H_
#define LIBUWEAVE_INCLUDE_UWEAVE_PROVIDER_TIME_H_

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

// TODO(jmccullough: A spiel about time.

/** Initialize any global state or hardware for tracking time. */
bool uwp_time_init();

/** Set the device time as seconds from the unix epoch. */
void uwp_time_set(time_t unix_timestamp_seconds);

/** Return the current time as seconds from the unix epoch. */
time_t uwp_time_get();

/**
 * Returns the number of seconds since an arbitrary epoch.
 *
 * The tick value should not be affected by uwp_time_set and must increase once
 * per second (subject to clock accuracy).
 */
time_t uwp_time_get_ticks();

/** Return a bound of the clock accuracy in parts per million. */
uint32_t uwp_time_get_accuracy_ppm();

/** Returns true if the time has not been set since the last power loss. */
bool uwp_time_is_time_set();

#endif  // LIBUWEAVE_INCLUDE_UWEAVE_PROVIDER_TIME_H_
