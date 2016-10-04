// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBUWEAVE_INCLUDE_UWEAVE_CHANNEL_ENCRYPTION_H_
#define LIBUWEAVE_INCLUDE_UWEAVE_CHANNEL_ENCRYPTION_H_

#include <stdbool.h>
#include <stdint.h>

#include "src/device_crypto.h"
#include "uweave/buffer.h"
#include "uweave/status.h"

#define UW_BLE_SESSION_ID_LEN 16

typedef enum {
  kUwChannelEncryptionPhasePassthrough,

  // TODO(arnarb): Asymmetric handshake b/26748660

  // Symmetric handshake
  kUwChannelEncryptionPhaseSATReceived,

  // Running session (i.e. handshake done)
  kUwChannelEncryptionPhaseInSession,
} UwChannelEncryptionPhase;

typedef enum {
  kUwChannelEncryptionRoleDevice = 0,
  kUwChannelEncryptionRoleClient
} UwChannelEncryptionRole;

typedef struct {
  UwChannelEncryptionPhase phase;
  UwChannelEncryptionRole encryption_role;

  // Common state for handshakes
  uint8_t client_random[12];
  uint8_t server_random[12];

  // Session state
  uint8_t session_key[16];
  // session id (16), sender (1), counter (3)
  uint8_t nonce_base[UW_BLE_SESSION_ID_LEN + 4];
  uint32_t our_counter;
  uint32_t their_counter;
} UwChannelEncryptionState;

static inline const uint8_t* uw_channel_encryption_session_id_(
    const UwChannelEncryptionState* state) {
  return state->nonce_base;
}

bool uw_channel_encryption_init_(UwChannelEncryptionState* state,
                                 UwBuffer* message_in,
                                 UwBuffer* message_out);

void uw_channel_encryption_build_token_sha256_session_key_(
    UwChannelEncryptionState* state,
    const uint8_t mac_tag[UW_MACAROON_MAC_LEN]);

/**
 * Processes an incoming message. This may either:
 * 1. Modify message_in in place.
 *    In this case a message was decrypted/passed through and it should
 *    be forwarded to the next step (message dispatching).
 * 2. Set data in message_out.
 *    In this case the encryption layer is doing its own signalling, such
 *    as a setup handshake. Do not pass message_in further, and instead reply
 *    to the client with the contents of message_out.
 * A caller should check if the length of message_out changed as a result
 * of calling this method. If not, case 1 applies, otherwise case 2.
 */
UwStatus uw_channel_encryption_process_in_(UwChannelEncryptionState* state,
                                           UwDeviceCrypto* device_crypto,
                                           UwBuffer* message_in,
                                           UwBuffer* message_out);

UwStatus uw_channel_encryption_process_out_(UwChannelEncryptionState* state,
                                            UwBuffer* message_out);

static inline bool uw_channel_encryption_is_encrypted_(
    UwChannelEncryptionState* state) {
  return state->phase == kUwChannelEncryptionPhaseInSession;
}

#endif  // LIBUWEAVE_INCLUDE_UWEAVE_CHANNEL_ENCRYPTION_H_
