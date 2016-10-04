// Copyright 2016 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBUWEAVE_INCLUDE_UWEAVE_PAIRING_CALLBACK_H_
#define LIBUWEAVE_INCLUDE_UWEAVE_PAIRING_CALLBACK_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "uweave/pairing_type.h"

/**
 * Handles the beginning of the pairing process. This function will be called at
 * the start of the pairing process and is responsible for displaying the
 * pairing PIN to the user, if applicable.
 */
typedef bool (*UwPairingBegin)(uint32_t session_id,
                               UwPairingType pairing_type,
                               const char* code);

/**
 * Handles the end of the pairing process. This function will be called at
 * the end of the pairing process. If there is a pairing PIN displayed to the
 * user, this function is responsible for taking it down.
 */
typedef bool (*UwPairingEnd)(uint32_t session_id);

typedef struct {
  UwPairingBegin begin;
  UwPairingEnd end;
} UwPairingCallback;

#endif  // LIBUWEAVE_INCLUDE_UWEAVE_PAIRING_CALLBACK_H_
