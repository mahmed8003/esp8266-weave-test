// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/crypto_eax.h"

#include <string.h>
#include "src/buffer.h"
#include "src/crypto_cmac.h"
#include "src/crypto_utils.h"
#include "src/log.h"
#include "src/uw_assert.h"
#include "uweave/provider/crypto.h"

#define CHECK_ERROR_(x) \
  do {                  \
    if (!(x))           \
      return false;     \
  } while (0)

/** Increments an arbitrary long integer, with most-significant byte first. */
static void increment_msb_(uint8_t* buf, size_t length) {
  UW_ASSERT(buf != NULL && length > 0, "Nothing to increment");
  while (length-- > 0) {
    if (buf[length] < 0xFF) {
      ++buf[length];
      return;
    } else {
      buf[length] = 0;
    }
  }
}

static void xor_buffers_(uint8_t* dst,
                         const uint8_t* a,
                         const uint8_t* b,
                         size_t length) {
  while (length--) {
    *dst++ = *a++ ^ *b++;
  }
}

/** Common state used in both encrypt and decrypt. */
typedef struct {
  UwCmacState cmac_state;
  uint8_t ctr[UWP_CRYPTO_AES128_BLOCK_SIZE];
  uint8_t ad_nonce_mac[UWP_CRYPTO_AES128_BLOCK_SIZE];
} EaxState_;

static bool eax_init_(const uint8_t* key,
                      size_t tag_length,
                      const uint8_t* nonce,
                      size_t nonce_length,
                      const uint8_t* ad,
                      size_t ad_length,
                      EaxState_* state) {
  if (key == NULL || nonce == NULL || nonce_length == 0) {
    return false;
  }
  if (0 == tag_length || tag_length > UWP_CRYPTO_AES128_BLOCK_SIZE) {
    return false;
  }

  CHECK_ERROR_(uw_cmac_init_(&state->cmac_state, key));

  uint8_t tweak[UWP_CRYPTO_AES128_BLOCK_SIZE] = {0};  // "tweak" for IV's CMAC
  CHECK_ERROR_(
      uw_cmac_update_(&state->cmac_state, tweak, UWP_CRYPTO_AES128_BLOCK_SIZE));
  CHECK_ERROR_(uw_cmac_update_(&state->cmac_state, nonce, nonce_length));
  CHECK_ERROR_(uw_cmac_final_(&state->cmac_state, state->ctr));

  // If zero tag length is ever allowed, the code from here ...
  tweak[UWP_CRYPTO_AES128_BLOCK_SIZE - 1] = 0x01;  // The "tweak" for AD's CMAC
  CHECK_ERROR_(uw_cmac_reset_(&state->cmac_state));
  CHECK_ERROR_(
      uw_cmac_update_(&state->cmac_state, tweak, UWP_CRYPTO_AES128_BLOCK_SIZE));
  if (ad != NULL && ad_length > 0) {
    CHECK_ERROR_(uw_cmac_update_(&state->cmac_state, ad, ad_length));
  }
  CHECK_ERROR_(uw_cmac_final_(&state->cmac_state, state->ad_nonce_mac));
  xor_buffers_(state->ad_nonce_mac, state->ad_nonce_mac, state->ctr,
               UWP_CRYPTO_AES128_BLOCK_SIZE);

  tweak[UWP_CRYPTO_AES128_BLOCK_SIZE - 1] = 0x02;  // "tweak" for ciphertext MAC
  CHECK_ERROR_(uw_cmac_reset_(&state->cmac_state));
  CHECK_ERROR_(
      uw_cmac_update_(&state->cmac_state, tweak, UWP_CRYPTO_AES128_BLOCK_SIZE));

  return true;
}

bool uw_eax_encrypt_(const uint8_t* key,
                     size_t tag_length,
                     const uint8_t* nonce,
                     size_t nonce_length,
                     const uint8_t* ad,
                     size_t ad_length,
                     UwBuffer* input,
                     UwBuffer* output) {
  if (input == NULL || output == NULL) {
    return false;
  }
  EaxState_ state;
  CHECK_ERROR_(
      eax_init_(key, tag_length, nonce, nonce_length, ad, ad_length, &state));

  const uint8_t* in_p;
  size_t in_length;
  uw_buffer_get_const_bytes(input, &in_p, &in_length);

  uint8_t* out_p;
  size_t out_capacity;
  uw_buffer_get_bytes_(output, &out_p, &out_capacity);

  size_t out_length = in_length + tag_length;
  if (out_length > out_capacity) {
    return false;  // Output buffer too small
  }

  // While input and output memory may alias, there is one case of overlap that
  // is not safe: When the output buffer starts in the middle of the input
  // buffer. In this case, writing the first output block would spoil
  // yet-unprocessed blocks of the input.
  long gap = (long)(out_p - in_p);
  if (0 < gap && gap < in_length) {
    return false;
  }

  uint8_t key_block[UWP_CRYPTO_AES128_BLOCK_SIZE];
  while (in_length > 0) {
    // Get new key block
    CHECK_ERROR_(uwp_crypto_aes128_ecb_encrypt(key, state.ctr, key_block));
    increment_msb_(state.ctr, UWP_CRYPTO_AES128_BLOCK_SIZE);

    size_t chunk_size = in_length > UWP_CRYPTO_AES128_BLOCK_SIZE
                            ? UWP_CRYPTO_AES128_BLOCK_SIZE
                            : in_length;
    xor_buffers_(out_p, in_p, key_block, chunk_size);
    CHECK_ERROR_(uw_cmac_update_(&state.cmac_state, out_p, chunk_size));

    in_length -= chunk_size;
    in_p += chunk_size;
    out_p += chunk_size;
  }

  // Done with the key_block, lets use it to do mac calculation
  CHECK_ERROR_(uw_cmac_final_(&state.cmac_state, key_block));
  xor_buffers_(key_block, key_block, state.ad_nonce_mac,
               UWP_CRYPTO_AES128_BLOCK_SIZE);
  memcpy(out_p, key_block, tag_length);

  uw_buffer_set_length_(output, out_length);
  return true;
}

bool uw_eax_decrypt_(const uint8_t* key,
                     size_t tag_length,
                     const uint8_t* nonce,
                     size_t nonce_length,
                     const uint8_t* ad,
                     size_t ad_length,
                     UwBuffer* input,
                     UwBuffer* output) {
  if (input == NULL || output == NULL) {
    return false;
  }
  EaxState_ state;
  CHECK_ERROR_(
      eax_init_(key, tag_length, nonce, nonce_length, ad, ad_length, &state));

  const uint8_t* in_p;
  size_t in_length;
  uw_buffer_get_const_bytes(input, &in_p, &in_length);

  uint8_t* out_p;
  size_t out_capacity;
  uw_buffer_get_bytes_(output, &out_p, &out_capacity);

  size_t out_length = in_length - tag_length;
  if (out_length > out_capacity) {
    return false;  // Output buffer too small
  }

  long gap = (long)(out_p - in_p);
  if (0 < gap && gap < out_length) {
    return false;  // Output would overwrite tail of input before processing it
  }

  uint8_t key_block[UWP_CRYPTO_AES128_BLOCK_SIZE];

  // Before decrypting, we'll use the key block to do our mac check
  if (in_length < tag_length) {
    return false;
  }
  in_length -= tag_length;
  CHECK_ERROR_(uw_cmac_update_(&state.cmac_state, in_p, in_length));
  CHECK_ERROR_(uw_cmac_final_(&state.cmac_state, key_block));
  xor_buffers_(key_block, key_block, state.ad_nonce_mac,
               UWP_CRYPTO_AES128_BLOCK_SIZE);
  if (!uw_crypto_utils_equal_(key_block, in_p + in_length, tag_length)) {
    UW_LOG_ERROR("Signature check failed\n");
    return false;
  }

  while (in_length > 0) {
    // Get new key block
    CHECK_ERROR_(uwp_crypto_aes128_ecb_encrypt(key, state.ctr, key_block));
    increment_msb_(state.ctr, UWP_CRYPTO_AES128_BLOCK_SIZE);

    size_t chunk_size = in_length > UWP_CRYPTO_AES128_BLOCK_SIZE
                            ? UWP_CRYPTO_AES128_BLOCK_SIZE
                            : in_length;
    xor_buffers_(out_p, in_p, key_block, chunk_size);

    in_length -= chunk_size;
    in_p += chunk_size;
    out_p += chunk_size;
  }

  uw_buffer_set_length_(output, out_length);
  return true;
}
