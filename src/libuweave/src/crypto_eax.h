// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBUWEAVE_SRC_CRYPTO_EAX_H_
#define LIBUWEAVE_SRC_CRYPTO_EAX_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "uweave/buffer.h"

/**
 * Encrypt. Input and output buffers may alias.
 * Capacity of output buffer must be state->tag_size more than the length of the
 * input buffer.
 */
bool uw_eax_encrypt_(const uint8_t* key,
                     size_t tag_length,
                     const uint8_t* nonce,
                     size_t nonce_length,
                     const uint8_t* ad,
                     size_t ad_length,
                     UwBuffer* input,
                     UwBuffer* output);

/**
 * Decrypt. Input and output buffers may alias. Returns false and overwrites
 * output with zeroes on signature error.
 */
bool uw_eax_decrypt_(const uint8_t* key,
                     size_t tag_length,
                     const uint8_t* nonce,
                     size_t nonce_length,
                     const uint8_t* ad,
                     size_t ad_length,
                     UwBuffer* input,
                     UwBuffer* output);

#endif /* LIBUWEAVE_SRC_CRYPTO_EAX_H_ */
