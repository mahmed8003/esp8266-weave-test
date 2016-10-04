// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBUWEAVE_SRC_MACAROON_H_
#define LIBUWEAVE_SRC_MACAROON_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

#include "src/macaroon_caveat.h"
#include "src/macaroon_context.h"

#define UW_MACAROON_MAC_LEN 16

// Jan 1st 2000 00:00:00 in unix epoch seconds.
#define J2000_EPOCH_OFFSET 946684800

// Note: If we are looking to make memory savings on MCUs,
// at the cost of a little extra processing, we can make
// the macaroon encoding the actual in-memory representation.
// This can save much copying of macaroon data if need be.
typedef struct {
  uint8_t mac_tag[UW_MACAROON_MAC_LEN];
  size_t num_caveats;
  const UwMacaroonCaveat* const* caveats;
} UwMacaroon;

// For the delegatee list in the validation result object
typedef enum {
  kUwMacaroonDelegateeTypeNone = 0,
  kUwMacaroonDelegateeTypeUser = 1,
  kUwMacaroonDelegateeTypeApp = 2,
  kUwMacaroonDelegateeTypeService = 3,
} UwMacaroonDelegateeType;

typedef struct {
  UwMacaroonDelegateeType type;
  const uint8_t* id;
  size_t id_len;
  UwMacaroonCaveatCloudServiceId service_id;  // Only for cloud services
  uint32_t timestamp;
} UwMacaroonDelegateeInfo;

#define MAX_NUM_DELEGATEES 10

typedef struct {
  UwMacaroonCaveatScopeType granted_scope;
  uint32_t expiration_time;  // In number of seconds since Jan 1st 2000 00:00:00
  bool app_commands_only;
  const uint8_t* lan_session_id;
  size_t lan_session_id_len;
  UwMacaroonDelegateeInfo delegatees[MAX_NUM_DELEGATEES];
  size_t num_delegatees;
} UwMacaroonValidationResult;

bool uw_macaroon_create_from_root_key_(UwMacaroon* new_macaroon,
                                       const uint8_t* root_key,
                                       size_t root_key_len,
                                       const UwMacaroonContext* context,
                                       const UwMacaroonCaveat* const caveats[],
                                       size_t num_caveats);

/**
 * Creates a new macaroon with a new caveat. The buffer must be large enough to
 * hold the count of caveats in the old_macaroon plus one.
 */
bool uw_macaroon_extend_(const UwMacaroon* old_macaroon,
                         UwMacaroon* new_macaroon,
                         const UwMacaroonContext* context,
                         const UwMacaroonCaveat* additional_caveat,
                         uint8_t* buffer,
                         size_t buffer_size);

/**
 * Verify and validate the Macaroon, and put relevant information into the
 * result object. Note that the resulting granted_scope will be the closest
 * valid scope type (to the narrower side) defined in macaroon_caveat.h.
 */
bool uw_macaroon_validate_(const UwMacaroon* macaroon,
                           const uint8_t* root_key,
                           size_t root_key_len,
                           const UwMacaroonContext* context,
                           UwMacaroonValidationResult* result);

/** Encode a Macaroon to a byte string. */
bool uw_macaroon_serialize_(const UwMacaroon* macaroon,
                            uint8_t* out,
                            size_t out_len,
                            size_t* resulting_str_len);

/**
 * Decodes a byte string to a Macaroon.
 *
 * One note is that the function doesn't copy string values to new buffers, so
 * the caller must maintain the input string around to make caveats with string
 * values to be usable.
 */
bool uw_macaroon_deserialize_(const uint8_t* in,
                              size_t in_len,
                              uint8_t* buffer,
                              size_t buffer_size,
                              UwMacaroon* new_macaroon);

/** Converts a j2000 timestamp to a unix timestamp. */
static inline time_t uw_macaroon_j2000_to_unix_epoch(time_t j2000) {
  return j2000 + J2000_EPOCH_OFFSET;
}

/** Converts a unix timestamp to a j2000 timestamp. */
static inline time_t uw_macaroon_unix_epoch_to_j2000(time_t unix_timestamp) {
  return unix_timestamp - J2000_EPOCH_OFFSET;
}

/**
 * Gets the expiration time of the macaroon as the number of seconds since the
 * unix epoch. A value of 0 means no expiration.
 */
time_t uw_macaroon_get_expiration_unix_epoch_time_(
    UwMacaroonValidationResult* result);

#endif  // LIBUWEAVE_SRC_MACAROON_H_
