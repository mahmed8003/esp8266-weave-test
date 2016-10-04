// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/crypto_hmac.h"
#include "uweave/provider/crypto.h"

void uw_crypto_hkdf_(const uint8_t* key_material,
                     size_t key_material_length,
                     const uint8_t* context_data,
                     size_t context_data_length,
                     const uint8_t salt[UWP_CRYPTO_SHA256_DIGEST_LEN],
                     uint8_t output[UWP_CRYPTO_SHA256_DIGEST_LEN]) {
  uint8_t intermediate_key[UWP_CRYPTO_SHA256_DIGEST_LEN];

  // Extract
  UwCryptoHmacMsg extract_msg = {.bytes = key_material,
                                 .num_bytes = key_material_length};
  uw_crypto_hmac_(salt, UWP_CRYPTO_SHA256_DIGEST_LEN, &extract_msg, 1,
                  intermediate_key, UWP_CRYPTO_SHA256_DIGEST_LEN);

  // Expand
  UwCryptoHmacMsg expand_msgs[2] = {
      {.bytes = context_data, .num_bytes = context_data_length},
      {.bytes = (const uint8_t*)"\x01", .num_bytes = 1}};
  uw_crypto_hmac_(intermediate_key, UWP_CRYPTO_SHA256_DIGEST_LEN, expand_msgs,
                  2, output, UWP_CRYPTO_SHA256_DIGEST_LEN);
}
