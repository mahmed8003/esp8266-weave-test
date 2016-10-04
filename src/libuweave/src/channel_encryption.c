// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string.h>
#include <stddef.h>

#include "src/channel_encryption.h"
#include "src/crypto_defines.h"
#include "src/crypto_eax.h"
#include "src/crypto_hkdf.h"
#include "src/log.h"
#include "src/macaroon.h"
#include "src/macaroon_caveat.h"
#include "src/macaroon_context.h"
#include "src/macaroon_caveat_internal.h"
#include "uweave/provider/crypto.h"

#define SESSION_TAG_LENGTH 12
#define SESSION_NONCE_LENGTH 20
#define SESSION_CLIENT_SENDER 0x01
#define SESSION_SERVER_SENDER 0x03

#define TOKEN_SHA256_KEY_DATA_LEN 41

#define MAX_FIRST_RESPONSE_SIZE 15
#define MAX_DECODED_SAT_SIZE 256

#ifdef VERBOSE_ENCRYPTION
/** Utility to print binary message payloads in hex. */
static inline void dump(const char* label,
                        const uint8_t* buffer,
                        size_t length) {
#if UW_LOG_LEVEL >= UW_LOG_LEVEL_INFO
  UW_LOG_INFO("%s: 0x", label);
  for (unsigned int i = 0; i < length; i++) {
    uw_log_simple_("%02X", buffer[i]);
  }
  uw_log_simple_("\n");
#endif
}
#endif

static const uint8_t kModeSaltTokenSha256[32] = {
    0x00, 0x8a, 0x39, 0x36, 0x22, 0x04, 0x1f, 0x5f, 0x0f, 0xc7, 0x5d,
    0x97, 0xda, 0xee, 0x6e, 0x81, 0xcb, 0xbb, 0x2b, 0xc7, 0x4f, 0x9c,
    0xcc, 0x91, 0xe7, 0x5e, 0x77, 0xa5, 0x6b, 0x4a, 0x4b, 0x05};

static const uint8_t kHkdfContextSessionKey[11] = {
    0x73, 0x65, 0x73, 0x73, 0x69, 0x6f,
    0x6e, 0x20, 0x6b, 0x65, 0x79  // ascii "session key"
};

bool uw_channel_encryption_init_(UwChannelEncryptionState* state,
                                 UwBuffer* message_in,
                                 UwBuffer* message_out) {
  const uint8_t* buf_in;
  size_t buf_in_length;

  uw_buffer_get_const_bytes(message_in, &buf_in, &buf_in_length);
  if (buf_in_length == 0) {
    UW_LOG_ERROR("Initial message is empty\n");
    return false;
  }

  switch (buf_in[0]) {
    case UW_CRYPTO_MODE_PASSTHROUGH:
      state->phase = kUwChannelEncryptionPhasePassthrough;
      // TODO(arnarb): set encrypted=false in session ctxt
      return true;

    case UW_CRYPTO_MODE_ED25519_HKDF_SHA256:
      // TODO(arnarb): b/26748660
      return false;

    case UW_CRYPTO_MODE_TOKEN_SHA256:
      if (buf_in_length != 13) {
        UW_LOG_ERROR("Client asked for token auth with wrong random\n");
        return false;
      }
      state->phase = kUwChannelEncryptionPhaseSATReceived;
      memcpy(state->client_random, buf_in + 1, 12);
      uwp_crypto_getrandom(state->server_random, 12);
      uw_buffer_append(message_out, state->server_random, 12);
      return true;

    default:
      UW_LOG_ERROR("Client requested unknown crypto mode: %d\n", buf_in[0]);
      return false;
  }
}

void uw_channel_encryption_build_token_sha256_session_key_(
    UwChannelEncryptionState* state,
    const uint8_t mac_tag[UW_MACAROON_MAC_LEN]) {
  uint8_t key_material[TOKEN_SHA256_KEY_DATA_LEN];
  key_material[0] = 0x02;
  memcpy(key_material + 1, state->client_random, 12);
  memcpy(key_material + 13, state->server_random, 12);
  memcpy(key_material + 25, mac_tag, UW_MACAROON_MAC_LEN);

  uint8_t hkdf_output[32];
  uw_crypto_hkdf_(key_material, sizeof(key_material), kHkdfContextSessionKey,
                  sizeof(kHkdfContextSessionKey), kModeSaltTokenSha256,
                  hkdf_output);
  memcpy(state->session_key, hkdf_output, 16);
  memcpy(state->nonce_base, hkdf_output + 16, 16);
  state->our_counter = 0;
  state->their_counter = 0;
#ifdef VERBOSE_ENCRYPTION
  // Only for debugging. Never check in code with this enabled.
  dump("mac_tag", mac_tag, UW_MACAROON_MAC_LEN);
  dump("session key", state->session_key, 16);
  dump("session id", state->nonce_base, 16);
  dump("client random", state->client_random, 12);
  dump("server random", state->server_random, 12);
#endif
}

static UwStatus handshake_sat_helper(UwChannelEncryptionState* state,
                                     UwDeviceCrypto* device_crypto,
                                     UwBuffer* message_in,
                                     UwBuffer* message_out);

UwStatus uw_channel_encryption_process_in_(UwChannelEncryptionState* state,
                                           UwDeviceCrypto* device_crypto,
                                           UwBuffer* message_in,
                                           UwBuffer* message_out) {
  // This function is called for every incoming message, keep stack small
  const uint8_t* buf_in;
  size_t buf_in_length;

  switch (state->phase) {
    case kUwChannelEncryptionPhasePassthrough:
      return kUwStatusSuccess;

    case kUwChannelEncryptionPhaseSATReceived:
      return handshake_sat_helper(state, device_crypto, message_in,
                                  message_out);

    case kUwChannelEncryptionPhaseInSession:
      uw_buffer_get_const_bytes(message_in, &buf_in, &buf_in_length);
      // Decrypt in-place
      if (0 == (++(state->their_counter) & 0x00ffffff)) {
        UW_LOG_ERROR("Client message counter rolled over.\n");
        return kUwStatusCryptoIncomingMessageInvalid;
      }
      state->nonce_base[16] =
          (state->encryption_role == kUwChannelEncryptionRoleDevice
               ? SESSION_CLIENT_SENDER
               : SESSION_SERVER_SENDER);
      state->nonce_base[17] = (state->their_counter >> 16) & 0xff;
      state->nonce_base[18] = (state->their_counter >> 8) & 0xff;
      state->nonce_base[19] = state->their_counter & 0xff;
      if (!uw_eax_decrypt_(state->session_key, SESSION_TAG_LENGTH,
                           state->nonce_base, SESSION_NONCE_LENGTH, NULL, 0,
                           message_in, message_in)) {
        UW_LOG_ERROR("Could not decrypt session message.\n");
        return kUwStatusCryptoIncomingMessageInvalid;
      }
      return kUwStatusSuccess;
  }

  // Keep this point unreachable.
  return kUwStatusCryptoIncomingMessageInvalid;
};

UwStatus uw_channel_encryption_process_out_(UwChannelEncryptionState* state,
                                            UwBuffer* message_out) {
  // This function is called for every incoming message, keep stack small
  switch (state->phase) {
    case kUwChannelEncryptionPhasePassthrough:
      return kUwStatusSuccess;

    case kUwChannelEncryptionPhaseSATReceived:
      // This should not happen
      UW_LOG_ERROR(
          "Application tried to sent message but still in handshake.\n");
      return kUwStatusInvalidArgument;

    case kUwChannelEncryptionPhaseInSession:
      // Encrypt in-place
      if (uw_buffer_get_length(message_out) + SESSION_TAG_LENGTH >
          uw_buffer_get_size(message_out)) {
        // TODO(arnarb): This error is checked by the EAX implementation,
        // but this allows a clearer error. Fix the EAX to be more specific.
        UW_LOG_ERROR(
            "Output buffer too small to encrypt with tag. "
            "msg len=%d, buf size=%d\n",
            (int)uw_buffer_get_length(message_out),
            (int)uw_buffer_get_size(message_out));
        return kUwStatusTooLong;
      }

      // WARNING: Do not change this without a review of the full encryption
      // spec.
      // Allowing more than 2^24-1 messages per key requires increasing the tag
      // size accordingly.
      // REUSING A COUNTER WITH THE SAME SESSION KEY IS NEVER SAFE.
      if (0 == (++state->our_counter & 0x00ffffff)) {
        UW_LOG_ERROR("Maximum messages per session reached.\n");
        return kUwStatusCryptoEncryptionFailed;
      }
      state->nonce_base[16] =
          (state->encryption_role == kUwChannelEncryptionRoleDevice
               ? SESSION_SERVER_SENDER
               : SESSION_CLIENT_SENDER);
      state->nonce_base[17] = (state->our_counter >> 16) & 0xff;
      state->nonce_base[18] = (state->our_counter >> 8) & 0xff;
      state->nonce_base[19] = state->our_counter & 0xff;
      if (!uw_eax_encrypt_(state->session_key, SESSION_TAG_LENGTH,
                           state->nonce_base, SESSION_NONCE_LENGTH, NULL, 0,
                           message_out, message_out)) {
        UW_LOG_ERROR("Could not encrypt session message.\n");
        return kUwStatusCryptoEncryptionFailed;
      }
      return kUwStatusSuccess;
  }

  // Keep this point unreachable.
  return kUwStatusNotFound;
}

// This helper processes an incoming SAT handshake message,
// and is kept separate since it requires a large amount of stack.
static UwStatus handshake_sat_helper(UwChannelEncryptionState* state,
                                     UwDeviceCrypto* device_crypto,
                                     UwBuffer* message_in,
                                     UwBuffer* message_out) {
  if (!device_crypto->has_device_auth_key) {
    return kUwStatusPairingRequired;
  }

  const uint8_t* buf_in;
  size_t buf_in_length;
  uw_buffer_get_const_bytes(message_in, &buf_in, &buf_in_length);

  uint8_t sat_buffer[MAX_DECODED_SAT_SIZE] = {};
  UwMacaroon sat;
  if (!uw_macaroon_deserialize_(buf_in, buf_in_length, sat_buffer,
                                sizeof(sat_buffer), &sat)) {
    UW_LOG_ERROR("Could not decode incoming SAT'\n");
    return kUwStatusCryptoIncomingMessageInvalid;
  }

  uint8_t sat_nonce[25];
  sat_nonce[0] = 0x01;
  memcpy(sat_nonce + 1, state->client_random, 12);
  memcpy(sat_nonce + 13, state->server_random, 12);

  UwMacaroonContext sat_context;
  uw_macaroon_context_create_(0, NULL, 0, sat_nonce, sizeof(sat_nonce),
                              &sat_context);

  UwMacaroonValidationResult sat_validation_result;
  if (!uw_macaroon_validate_(&sat, device_crypto->device_authentication_key,
                             sizeof(device_crypto->device_authentication_key),
                             &sat_context, &sat_validation_result)) {
    UW_LOG_ERROR("Incoming SAT' is invalid.\n");
    return kUwStatusVerificationFailed;
  }

  // (re)compute the tag of SAT (SAT' minus the last caveat)
  UwMacaroon sat2;
  if (!uw_macaroon_create_from_root_key_(
          &sat2, device_crypto->device_authentication_key,
          sizeof(device_crypto->device_authentication_key), &sat_context,
          sat.caveats, sat.num_caveats - 1)) {
    UW_LOG_ERROR("Could not recreate SAT\n");
    return kUwStatusCryptoIncomingMessageInvalid;
  }

  sat_nonce[0] = 0x02;

  UwMacaroonCaveat challenge_caveat;
  uint8_t challenge_caveat_buffer[16];
  if (!uw_macaroon_caveat_create_authentication_challenge_(
          challenge_caveat_buffer, sizeof(challenge_caveat_buffer),
          &challenge_caveat)) {
    UW_LOG_ERROR("Could not create nonce caveat\n");
    return kUwStatusCryptoIncomingMessageInvalid;
  }
  uint8_t sat_signature[UW_MACAROON_MAC_LEN];
  if (!uw_macaroon_caveat_sign_(sat2.mac_tag, UW_MACAROON_MAC_LEN, &sat_context,
                                &challenge_caveat, sat_signature,
                                sizeof(sat_signature))) {
    UW_LOG_ERROR("Could not sign caveat for server authentication\n");
    return kUwStatusCryptoIncomingMessageInvalid;
  }
  uw_buffer_append(message_out, sat_signature, sizeof(sat_signature));

  // sat2.mac_tag == original sat.mac_tag (sat.mac_tag above is sat').
  uw_channel_encryption_build_token_sha256_session_key_(state, sat2.mac_tag);
  state->phase = kUwChannelEncryptionPhaseInSession;
  UW_LOG_INFO("Starting session\n");
  return kUwStatusSuccess;
}
