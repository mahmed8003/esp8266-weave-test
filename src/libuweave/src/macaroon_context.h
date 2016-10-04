// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBUWEAVE_SRC_MACAROON_CONTEXT_
#define LIBUWEAVE_SRC_MACAROON_CONTEXT_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

#include "src/macaroon_caveat.h"

typedef struct {
  uint32_t current_time;  // In number of seconds since Jan 1st 2000 00:00:00
  const uint8_t* ble_session_id;  // Only for BLE
  size_t ble_session_id_len;
  const uint8_t* auth_challenge_str;
  size_t auth_challenge_str_len;
} UwMacaroonContext;

// If current_j2000_time is negative, the value of current_time in the context
// will be set to 0.
bool uw_macaroon_context_create_(time_t current_j2000_time,
                                 const uint8_t* ble_session_id,
                                 size_t ble_session_id_len,
                                 const uint8_t* auth_challenge_str,
                                 size_t auth_challenge_str_len,
                                 UwMacaroonContext* new_context);

// If current_j2000_time is negative, the value of current_time in the context
// will be set to 0.
static inline bool uw_macaroon_context_create_with_timestamp_(
    time_t current_j2000_time,
    UwMacaroonContext* new_context) {
  return uw_macaroon_context_create_(
      current_j2000_time,
      /* ble_session_id */ NULL, /* ble_session_id_len */ 0,
      /* auth_challeng_str */ NULL,
      /* auth_challenge_str_len */ 0, new_context);
}

#endif  // LIBUWEAVE_SRC_MACAROON_CONTEXT_
