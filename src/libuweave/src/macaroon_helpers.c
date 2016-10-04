// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/macaroon_helpers.h"

#include "src/macaroon_caveat.h"
#include "src/macaroon_context.h"

/**
 * Allocate two lists in the given buffer: a list of Caveats, and the list of
 * pointers pointing to the Caveats. The pointer list is needed for creating a
 * Macaroon. This function will also advance the buffer pointer.
 */
static UwMacaroonCaveat** allocate_caveat_list_(size_t num_caveats,
                                                uint8_t** buffer_ptr,
                                                size_t* buffer_size_ptr) {
  // Create the Caveat list
  size_t needed_buffer_size = num_caveats * sizeof(UwMacaroonCaveat);
  if (needed_buffer_size > *buffer_size_ptr) {
    return NULL;
  }
  UwMacaroonCaveat* caveats = (UwMacaroonCaveat*)(*buffer_ptr);
  *buffer_ptr += needed_buffer_size;
  *buffer_size_ptr -= needed_buffer_size;

  needed_buffer_size = num_caveats * sizeof(UwMacaroonCaveat*);
  if (needed_buffer_size > *buffer_size_ptr) {
    return NULL;
  }
  UwMacaroonCaveat** caveat_ptrs = (UwMacaroonCaveat**)(*buffer_ptr);
  *buffer_ptr += needed_buffer_size;
  *buffer_size_ptr -= needed_buffer_size;

  for (size_t i = 0; i < num_caveats; i++) {
    caveat_ptrs[i] = &(caveats[i]);
  }

  return caveat_ptrs;
}

bool uw_macaroon_mint_server_authentication_token_(
    const uint8_t* root_key,
    size_t root_key_len,
    const uint8_t* token_str,
    size_t token_str_len,
    const uint8_t nonce[UW_MACAROON_INIT_DELEGATION_NONCE_SIZE],
    uint8_t* buffer,
    size_t buffer_size,
    UwMacaroon* new_macaroon) {
  if (root_key == NULL || root_key_len == 0 || nonce == NULL ||
      buffer == NULL || buffer_size == 0 || new_macaroon == NULL) {
    return false;
  }
  if (token_str == NULL && token_str_len != 0) {
    return false;
  }

  *new_macaroon = (UwMacaroon){};

  // This creates a Macaroon with two caveats:
  // 1. SATv1
  // 2. Nonce
  const size_t num_caveats = 2;
  UwMacaroonCaveat** caveat_list =
      allocate_caveat_list_(num_caveats, &buffer, &buffer_size);
  if (caveat_list == NULL) {
    return false;
  }

  // The SATv1 Caveat
  size_t needed_buffer_size = uw_macaroon_caveat_creation_get_buffsize_(
      kUwMacaroonCaveatTypeServerAuthenticationTokenV1, token_str_len);
  if ((needed_buffer_size == 0) || (needed_buffer_size > buffer_size) ||
      !uw_macaroon_caveat_create_server_authentication_token_(
          token_str, token_str_len, buffer, needed_buffer_size,
          caveat_list[0])) {
    return false;
  }
  buffer += needed_buffer_size;
  buffer_size -= needed_buffer_size;

  // The Nonce Caveat
  needed_buffer_size = uw_macaroon_caveat_creation_get_buffsize_(
      kUwMacaroonCaveatTypeNonce, UW_MACAROON_INIT_DELEGATION_NONCE_SIZE);
  if ((needed_buffer_size == 0) || (needed_buffer_size > buffer_size) ||
      !uw_macaroon_caveat_create_nonce_(
          nonce, UW_MACAROON_INIT_DELEGATION_NONCE_SIZE, buffer,
          needed_buffer_size, caveat_list[1])) {
    return false;
  }

  // Fill the Macaroon with the two Caveats
  UwMacaroonContext context = {};  // Context values are not needed in this case
  return uw_macaroon_create_from_root_key_(
      new_macaroon, root_key, root_key_len, &context,
      (const UwMacaroonCaveat* const*)caveat_list, num_caveats);
}

bool uw_macaroon_mint_client_authorization_token_(
    const uint8_t* root_key,
    size_t root_key_len,
    const uint8_t* token_str,
    size_t token_str_len,
    uint32_t current_time,
    UwMacaroonCaveatCloudServiceId service_id,
    uint8_t* buffer,
    size_t buffer_size,
    UwMacaroon* new_macaroon) {
  if (root_key == NULL || root_key_len == 0 || buffer == NULL ||
      buffer_size == 0 || new_macaroon == NULL) {
    return false;
  }
  if (token_str == NULL && token_str_len != 0) {
    return false;
  }

  *new_macaroon = (UwMacaroon){};

  // This creates a Macaroon with 3 Caveats:
  // 1. CATv1
  // 2. Timestamp
  // 3. Service Delegate ID
  const size_t num_caveats = 3;
  UwMacaroonCaveat** caveat_list =
      allocate_caveat_list_(num_caveats, &buffer, &buffer_size);
  if (caveat_list == NULL) {
    return false;
  }

  // The CATv1 Caveat
  size_t needed_buffer_size = uw_macaroon_caveat_creation_get_buffsize_(
      kUwMacaroonCaveatTypeClientAuthorizationTokenV1, token_str_len);
  if ((needed_buffer_size == 0) || (needed_buffer_size > buffer_size) ||
      !uw_macaroon_caveat_create_client_authorization_token_(
          token_str, token_str_len, buffer, needed_buffer_size,
          caveat_list[0])) {
    return false;
  }
  buffer += needed_buffer_size;
  buffer_size -= needed_buffer_size;

  // The Timestamp Caveat
  needed_buffer_size = uw_macaroon_caveat_creation_get_buffsize_(
      kUwMacaroonCaveatTypeDelegationTimestamp, 0);
  if ((needed_buffer_size == 0) || (needed_buffer_size > buffer_size) ||
      !uw_macaroon_caveat_create_delegation_timestamp_(
          current_time, buffer, needed_buffer_size, caveat_list[1])) {
    return false;
  }
  buffer += needed_buffer_size;
  buffer_size -= needed_buffer_size;

  // The Service Delegatee Caveat
  needed_buffer_size = uw_macaroon_caveat_creation_get_buffsize_(
      kUwMacaroonCaveatTypeDelegateeService, 0);
  if ((needed_buffer_size == 0) || (needed_buffer_size > buffer_size) ||
      !uw_macaroon_caveat_create_delegatee_service_(
          service_id, buffer, needed_buffer_size, caveat_list[2])) {
    return false;
  }

  // Fill the Macaroon with the three Caveats
  UwMacaroonContext context = {};  // Context values are not needed in this case
  return uw_macaroon_create_from_root_key_(
      new_macaroon, root_key, root_key_len, &context,
      (const UwMacaroonCaveat* const*)caveat_list, num_caveats);
}
