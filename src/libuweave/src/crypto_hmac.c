// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/crypto_hmac.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "uweave/provider/crypto.h"

#define IPAD_BYTE 0x36
#define OPAD_BYTE 0x5C

bool uw_crypto_hmac_(const uint8_t* key,
                     size_t key_len,
                     const UwCryptoHmacMsg messages[],
                     size_t num_messages,
                     uint8_t* truncated_digest,
                     size_t truncated_digest_len) {
  if (key == NULL || key_len == 0 || truncated_digest == NULL ||
      truncated_digest_len == 0 ||
      truncated_digest_len > UWP_CRYPTO_SHA256_DIGEST_LEN ||
      (num_messages != 0 && messages == NULL)) {
    return false;
  }
  for (size_t i = 0; i < num_messages; i++) {
    if (messages[i].num_bytes > 0 && messages[i].bytes == NULL) {
      return false;
    }
  }

  UwpCryptoSha256State sha256_state;
  uint8_t digest[UWP_CRYPTO_SHA256_DIGEST_LEN] = {0};

  // Processing the key
  uint8_t key_state[UWP_CRYPTO_SHA256_BLOCK_SIZE] = {0};
  if (key_len <= UWP_CRYPTO_SHA256_BLOCK_SIZE) {
    memcpy(key_state, key, key_len);
  } else {
    uwp_crypto_sha256_init(&sha256_state);
    uwp_crypto_sha256_update(&sha256_state, key, key_len);
    uwp_crypto_sha256_final(&sha256_state, key_state);
  }

  // Inner hashing
  uwp_crypto_sha256_init(&sha256_state);
  for (size_t i = 0; i < UWP_CRYPTO_SHA256_BLOCK_SIZE; i++) {
    key_state[i] ^= IPAD_BYTE;
  }
  uwp_crypto_sha256_update(&sha256_state, key_state,
                           UWP_CRYPTO_SHA256_BLOCK_SIZE);
  for (size_t i = 0; i < num_messages; i++) {
    if (messages[i].num_bytes != 0) {
      uwp_crypto_sha256_update(&sha256_state, messages[i].bytes,
                               messages[i].num_bytes);
    }
  }
  uwp_crypto_sha256_final(&sha256_state, digest);

  // Outer hashing
  uwp_crypto_sha256_init(&sha256_state);
  for (size_t i = 0; i < UWP_CRYPTO_SHA256_BLOCK_SIZE; i++) {
    key_state[i] ^= IPAD_BYTE ^ OPAD_BYTE;
  }
  uwp_crypto_sha256_update(&sha256_state, key_state,
                           UWP_CRYPTO_SHA256_BLOCK_SIZE);
  uwp_crypto_sha256_update(&sha256_state, digest, UWP_CRYPTO_SHA256_DIGEST_LEN);
  uwp_crypto_sha256_final(&sha256_state, digest);

  memcpy(truncated_digest, digest, truncated_digest_len);
  return true;
}
