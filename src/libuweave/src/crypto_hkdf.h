// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBUWEAVE_SRC_CRYPTO_HKDF_H_
#define LIBUWEAVE_SRC_CRYPTO_HKDF_H_

#include <stddef.h>
#include <stdint.h>

#include "uweave/provider/crypto.h"

/**
 * Derives keys with HKDF-SHA256.
 * See https://tools.ietf.org/html/rfc5869
 *
 * This differs in two ways from the RFC:
 *  - Only 32-byte values are accepted for salt.
 *  - Only the first 32 bytes are available from the expansion step.
 */
void uw_crypto_hkdf_(const uint8_t* key_material,
                     size_t key_material_length,
                     const uint8_t* context_data,
                     size_t context_data_length,
                     const uint8_t salt[UWP_CRYPTO_SHA256_DIGEST_LEN],
                     uint8_t output[UWP_CRYPTO_SHA256_DIGEST_LEN]);

#endif  // LIBUWEAVE_SRC_CRYPTO_HKDF_H_
