// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/crypto_spake.h"

#include <string.h>
#include "src/buffer.h"
#include "src/crypto_utils.h"
#include "src/log.h"
#include "src/uw_assert.h"
#include "omaha-crypto/p224.h"

/*
 * Implements SPAKE2 as defined here:
 * http://www.di.ens.fr/~mabdalla/papers/AbPo05a-letter.pdf
 *
 * The constants N and M are the same as in Chromium's implementation at
 * https://code.google.com/p/chromium/codesearch#chromium/src/crypto/p224_spake.h
 *
 * See that file for details of their generation.
 */

static const p224_point kM = {
    {174237515, 77186811, 235213682, 33849492, 33188520, 48266885, 177021753,
     81038478},
    {104523827, 245682244, 266509668, 236196369, 28372046, 145351378, 198520366,
     113345994},
    {1, 0, 0, 0, 0, 0, 0, 0},
};

static const p224_point kN = {
    {136176322, 263523628, 251628795, 229292285, 5034302, 185981975, 171998428,
     11653062},
    {197567436, 51226044, 60372156, 175772188, 42075930, 8083165, 160827401,
     65097570},
    {1, 0, 0, 0, 0, 0, 0, 0},
};

bool uw_spake_init_(UwSpakeState* state,
                    bool is_server,
                    const uint8_t* password,
                    size_t password_length) {
  state->is_server = is_server;

  uint8_t buf[UWP_CRYPTO_SHA256_DIGEST_LEN];
  UwpCryptoSha256State sha;
  uwp_crypto_sha256_init(&sha);
  uwp_crypto_sha256_update(&sha, password, password_length);
  uwp_crypto_sha256_final(&sha, buf);
  memcpy(state->pw, buf, UW_SPAKE_P224_SCALAR_SIZE);

  if (!uwp_crypto_getrandom(state->x, sizeof(state->x))) {
    UW_LOG_ERROR("Could not get random data");
    return false;
  }

  return true;
}

bool uw_spake_compute_commitment_(UwSpakeState* state, UwBuffer* commitment) {
  p224_point X, mask, Xmasked;

  p224_base_point_mul(state->x, &X);
  p224_point_mul(state->is_server ? &kN : &kM, state->pw, &mask);
  p224_point_add(&mask, &X, &Xmasked);
  p224_point_to_bin(&Xmasked, state->Xmasked);

  if (!uw_buffer_append(commitment, state->Xmasked, sizeof(state->Xmasked))) {
    UW_LOG_ERROR("Could not write commitment. Buffer too small?\n");
    return false;
  }

  return true;
}

bool uw_spake_finalize_(UwSpakeState* state,
                        const UwBuffer* remote_commitment,
                        uint8_t* key,
                        size_t key_size) {
  p224_point p1, p2, Y;

  const uint8_t* in_bytes;
  size_t in_length;
  uw_buffer_get_const_bytes(remote_commitment, &in_bytes, &in_length);

  if (in_length != UW_SPAKE_P224_POINT_SIZE) {
    UW_LOG_ERROR("Received spake commitment has incorrect size %d != %d.\n",
                 (int)in_length, (int)UW_SPAKE_P224_POINT_SIZE);
    return false;
  }
  memcpy(state->Ymasked, in_bytes, UW_SPAKE_P224_POINT_SIZE);

  // p1 = mask point (pw * N or pw * M)
  p224_point_mul(state->is_server ? &kM : &kN, state->pw, &p1);

  // p2 = negative of mask point
  p224_point_negate(&p1, &p2);

  // p1 = Ymasked, remote's public key + mask point
  if (0 == p224_point_from_bin(state->Ymasked, sizeof(state->Ymasked), &p1)) {
    UW_LOG_ERROR("Could not parse a P224 public key from remote.");
    return false;
  }

  // Y = p1 + p2 = Ymasked - mask point
  p224_point_add(&p1, &p2, &Y);

  // p1 = DH secret
  p224_point_mul(&Y, state->x, &p1);
  p224_point_to_bin(&p1, state->key);

  UW_ASSERT(key != NULL && key_size > 0, "Nowhere to put SPAKE key.\n");
  // TODO(arnarb): This should probably be limited to the x-coordinate.
  UW_ASSERT(key_size <= sizeof(state->key), "Requested key is too large.\n");
  memcpy(key, state->key, key_size);
  return true;
}
