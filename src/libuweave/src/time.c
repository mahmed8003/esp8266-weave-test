// Copyright 2016 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/time.h"

#include "uweave/provider/time.h"

time_t uw_time_get_timestamp_seconds_() {
  return uwp_time_get();
}

UwTimeStatus uw_time_get_status_() {
  // TODO(jmccullough): Implement the ppm-degradation measure.
  if (!uwp_time_is_time_set()) {
    return kUwTimeStatusInvalid;
  } else {
    return kUwTimeStatusOk;
  }
}

UwStatus uw_time_set_timestamp_seconds_(time_t timestamp, UwTimeSource source) {
  // TODO(jmccullough): Implement the sourced version.
  uwp_time_set(timestamp);
  return kUwStatusSuccess;
}

time_t uw_time_get_uptime_seconds_() {
  return uwp_time_get_ticks();
}
