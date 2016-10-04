// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBUWEAVE_SRC_MACAROON_HELPERS_H_
#define LIBUWEAVE_SRC_MACAROON_HELPERS_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "src/macaroon.h"

#define UW_MACAROON_INIT_DELEGATION_NONCE_SIZE 8
#define UW_MACAROON_ADDL_DELEGATION_NONCE_SIZE 16

/**
 * Mint an initial server authentication token (SAT).
 *
 * @param root_key the root key for signing the token.
 * @param root_key_len the number of bytes of the root key.
 * @param token_str the arbitrary string to be contained in the version caveat
 *        (can be empty).
 * @param token_str_len the number of bytes of the token string (0 indicates an
 *        empty string).
 * @param nonce the initial nonce, which should be a value not used before.
 * @param buffer the buffer used to create the data structure of the macaroon
 * @param buffer_size the size of the given buffer.
 * @param new_macaroon the created caveats and other data will be filled into
 *        this macaroon struct.
 *
 * @return false if anything goes wrong; true otherwise.
 */
bool uw_macaroon_mint_server_authentication_token_(
    const uint8_t* root_key,
    size_t root_key_len,
    const uint8_t* token_str,
    size_t token_str_len,
    const uint8_t nonce[UW_MACAROON_INIT_DELEGATION_NONCE_SIZE],
    uint8_t* buffer,
    size_t buffer_size,
    UwMacaroon* new_macaroon);

/**
 * Mint an initial client authorization token (CAT).
 *
 * @param root_key the root key for signing the token.
 * @param root_key_len the number of bytes of the root key.
 * @param token_str the arbitrary string to be contained in the version caveat
 *        (can be empty).
 * @param token_str_len the number of bytes of the token string (0 indicates an
 *        empty string).
 * @param current_time the initial timestamp to be included in the macaroon.
 * @param service_id the initial delegatee of the token, for example, the owner
 *        or a cloud service
 * @param buffer the buffer used to create the data structure of the macaroon
 * @param buffer_size the size of the given buffer.
 * @param new_macaroon the created caveats and other data will be filled into
 *        this macaroon struct.
 *
 * @return false if anything goes wrong; true otherwise.
 */
bool uw_macaroon_mint_client_authorization_token_(
    const uint8_t* root_key,
    size_t root_key_len,
    const uint8_t* token_str,
    size_t token_str_len,
    uint32_t current_time,
    UwMacaroonCaveatCloudServiceId service_id,
    uint8_t* buffer,
    size_t buffer_size,
    UwMacaroon* new_macaroon);

#endif  // LIBUWEAVE_SRC_MACAROON_HELPERS_
