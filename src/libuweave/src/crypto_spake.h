// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBUWEAVE_SRC_CRYPTO_SPAKE_H_
#define LIBUWEAVE_SRC_CRYPTO_SPAKE_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "uweave/buffer.h"
#include "uweave/provider/crypto.h"

#define UW_SPAKE_P224_SCALAR_SIZE 28
#define UW_SPAKE_P224_POINT_SIZE (2 * UW_SPAKE_P224_SCALAR_SIZE)

typedef struct UwSpakeState_ {
  bool is_server;                             // Protocol role
  uint8_t x[UW_SPAKE_P224_SCALAR_SIZE];       // Ephemeral private key
  uint8_t Xmasked[UW_SPAKE_P224_POINT_SIZE];  // Local commitment
  uint8_t Ymasked[UW_SPAKE_P224_POINT_SIZE];  // Remote commitment
  uint8_t pw[UW_SPAKE_P224_SCALAR_SIZE];      // Hashed password
  uint8_t key[UW_SPAKE_P224_POINT_SIZE];      // DH Secret
} UwSpakeState;

/**
 * The spake exchange enables two remote parties to securely negotiate a shared
 * key from a shared secret.
 *
 * If A and B are the two parties, each initializes the spake state with the
 * shared secret that is exchanged out of band (e.g. a display).
 *
 * From the initial state, party A computes commitment C_a and party B computes
 * commitment C_b and they are exchanged over the wire. Party A then updates
 * its state with C_b and party B updates its state with C_a. With a successful
 * exchange, party A and party B will have the same key in the spake state and
 * the key can be used to bootstrap further secure exchanges.
 */

/**
 * Initializes the spake state based on the shared secret, `password` described
 * in the header comment above.
 */
bool uw_spake_init_(UwSpakeState* state,
                    bool is_server,
                    const uint8_t* password,
                    size_t password_length);

/**
 * Computes this party's commitment, i.e., C_a, descrbied in the header comment
 * above.
 */
bool uw_spake_compute_commitment_(UwSpakeState* state, UwBuffer* commitment);

/**
 * Completes the spake exchange by updating the spake state with the remote
 * commitment, i.e., C_b, described in the header comment above, and writing the
 * shared key to `key`.
 */
bool uw_spake_finalize_(UwSpakeState* state,
                        const UwBuffer* remote_commitment,
                        uint8_t* key,
                        size_t key_size);

#endif  // LIBUWEAVE_SRC_CRYPTO_SPAKE_H_
