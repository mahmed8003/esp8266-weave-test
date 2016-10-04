// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBUWEAVE_SRC_CRYPTO_CMAC_H_
#define LIBUWEAVE_SRC_CRYPTO_CMAC_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "uweave/provider/crypto.h"

typedef struct {
  const uint8_t* key;
  uint8_t block[UWP_CRYPTO_AES128_BLOCK_SIZE];
  uint8_t k1[UWP_CRYPTO_AES128_BLOCK_SIZE];
  uint8_t k2[UWP_CRYPTO_AES128_BLOCK_SIZE];
  uint8_t partial_block[UWP_CRYPTO_AES128_BLOCK_SIZE];
  uint8_t partial_size;
} UwCmacState;

bool uw_cmac_init_(UwCmacState* state, const uint8_t* key);
bool uw_cmac_reset_(UwCmacState* state);
bool uw_cmac_clone_(UwCmacState* dst, const UwCmacState* src);
bool uw_cmac_update_(UwCmacState* state, const uint8_t* data, size_t length);
bool uw_cmac_final_(UwCmacState* state, uint8_t* mac);

#endif  // LIBUWEAVE_SRC_CRYPTO_CMAC_H_
