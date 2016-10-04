// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBUWEAVE_INCLUDE_UWEAVE_PROVIDER_CRYPTO_H_
#define LIBUWEAVE_INCLUDE_UWEAVE_PROVIDER_CRYPTO_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define UWP_CRYPTO_AES128_BLOCK_SIZE 16
#define UWP_CRYPTO_SHA256_BLOCK_SIZE 64
#define UWP_CRYPTO_SHA256_DIGEST_LEN 32

/**
 * Initializes any global state for the crypto library at startup.  No other uw
 * or uwp functions may be invoked.
 */
bool uwp_crypto_init();

/**
 * Encrypts a single 16-byte block with a 128-bit key with AES-128 ECB mode.
 * Writes a 16-byte ciphertext block out to cipher text, caller must ensure
 * this is a valid buffer of sufficient size.
 * Provider implementation *must* support the case where the plaintext and
 * ciphertext pointers point to the same block.
 */
bool uwp_crypto_aes128_ecb_encrypt(const uint8_t* key,
                                   const uint8_t* plaintext,
                                   uint8_t* ciphertext);

typedef struct {
  uint8_t data[64];
  uint8_t datalen;
  uint64_t bitlen;
  uint32_t state[8];
} UwpCryptoSha256State;

/**
 * The SHA256 implementation should expect to handle sensitive data for
 * cryptographic use.
 */
void uwp_crypto_sha256_init(UwpCryptoSha256State* state);
void uwp_crypto_sha256_update(UwpCryptoSha256State* state,
                              const uint8_t* data,
                              size_t data_len);
void uwp_crypto_sha256_final(UwpCryptoSha256State* state, uint8_t* digest);

/**
 * Sources random data suitable for cryptographic purposes. The provider
 * will write length bytes of random data into the buffer given and return true.
 * If random data is not available, the provider will return false.
 */
bool uwp_crypto_getrandom(uint8_t* buffer, size_t length);

#endif  // LIBUWEAVE_INCLUDE_UWEAVE_PROVIDER_CRYPTO_H_
