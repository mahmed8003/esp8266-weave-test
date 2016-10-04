// Copyright 2016 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "pairing_request.h"

#include <string.h>

#include "src/buffer.h"
#include "src/crypto_eax.h"
#include "src/crypto_spake.h"
#include "src/device.h"
#include "src/device_crypto.h"
#include "src/log.h"
#include "src/macaroon_helpers.h"
#include "src/privet_defines.h"
#include "src/privet_request.h"
#include "src/session.h"
#include "src/time.h"
#include "src/value.h"
#include "tinycbor/src/cbor.h"
#include "uweave/embedded_code.h"
#include "uweave/pairing_callback.h"
#include "uweave/pairing_type.h"
#include "uweave/provider/crypto.h"
#include "uweave/status.h"
#include "uweave/value_scan.h"

// The maximum length of a pairing code plus an extra byte for the terminating
// null character.
#define PAIRING_CODE_BUF_LEN 13

/**
 * Generates a new pairing passcode and displays it to the user.
 */
// STOPSHIP(mcolagrosso): Move responsibilty to a provider.
static void generate_pairing_passcode(UwBuffer* buffer) {
  UW_LOG_INFO("Using fixed pairing passcode for now.\n");
  // STOPSHIP(mcolagrosso): Move responsibilty of generating a random passcode
  // to a provider.
  static const char pairing_passcode[] = "7777";
  uw_buffer_append(buffer, (const uint8_t*)pairing_passcode,
                   strlen(pairing_passcode));
  UW_LOG_INFO("Pairing passcode: %s\n", pairing_passcode);
}

/**
 * Generates a new session ID for establishing the pairing process.
 */
static uint32_t generate_session_id() {
  uint32_t session_id;
  uwp_crypto_getrandom((uint8_t*)&session_id, sizeof(session_id));
  return session_id;
}

bool get_embedded_code_(UwSettings* settings, char** embedded_code_str) {
  UwEmbeddedCode embedded_code = settings->embedded_code;
  UwEmbeddedCodeSource source = embedded_code.source;
  switch (source) {
    case kUwEmbeddedCodeSourceNone:
      UW_LOG_WARN("No embedded code source\n");
      return false;
    case kUwEmbeddedCodeSourceSettings:
      *embedded_code_str = embedded_code.u.embedded_code_str;
      return true;
    case kUwEmbeddedCodeSourceCallback:
      return embedded_code.u.callback_func(embedded_code_str);
    default:
      UW_LOG_WARN("Unknown embedded code source: %d\n", source);
      return false;
  }
}

UwStatus uw_pairing_start_reply_(UwPrivetRequest* privet_request) {
  UwBuffer* privet_param_buffer =
      uw_privet_request_get_param_buffer_(privet_request);

  if (uw_buffer_is_null(privet_param_buffer)) {
    return kUwStatusPrivetInvalidParam;
  }

  if (!privet_request->session->device->settings->enable_multipairing &&
      uw_device_is_setup(privet_request->session->device)) {
    return kUwStatusPairingResetRequired;
  }

  UwValue pairing_param = uw_value_undefined();
  UwValue crypto_param = uw_value_undefined();

  UwMapFormat format[] = {
      {.key = uw_value_int(PRIVET_PAIRING_START_KEY_PAIRING),
       .type = kUwValueTypeInt,
       .value = &pairing_param},
      {.key = uw_value_int(PRIVET_PAIRING_START_KEY_CRYPTO),
       .type = kUwValueTypeInt,
       .value = &crypto_param},
  };

  if (!uw_status_is_success(
          uw_value_scan_map(privet_param_buffer, format,
                            uw_value_scan_map_count(sizeof(format))))) {
    UW_LOG_WARN("Error parsing /pairing/start parameters\n");
    return kUwStatusPrivetInvalidParam;
  }

  bool has_pairing = !uw_value_is_undefined(&pairing_param);
  bool has_crypto = !uw_value_is_undefined(&crypto_param);

  if (!has_pairing || !has_crypto) {
    UW_LOG_WARN("Privet /pairing/start required param missing:%s%s\n",
                (has_pairing ? "" : " pairing"), (has_crypto ? "" : " crypto"));
    return kUwStatusPrivetInvalidParam;
  }

  UwPairingType pairing_type = 0;
  uint8_t pairing_buf[PAIRING_CODE_BUF_LEN] = {0};
  UwBuffer pairing_passcode_buffer;
  // Initializing the size to one less than the size of pairing_buf to ensure
  // there is always a null terminator.
  uw_buffer_init(&pairing_passcode_buffer, pairing_buf,
                 sizeof(pairing_buf) - 1);

  uint8_t supported_pairing_types =
      privet_request->session->device->settings->supported_pairing_types;

  switch (pairing_param.value.int_value) {
    case PRIVET_INFO_AUTH_VALUE_PAIRING_PIN:
      pairing_type = kUwPairingTypePinCode;
      if (!(pairing_type & supported_pairing_types)) {
        UW_LOG_WARN("Pin code pairing unsupported\n");
        return kUwStatusPairingPinCodeTypeUnsupported;
      }
      generate_pairing_passcode(&pairing_passcode_buffer);
      if (uw_buffer_get_length(&pairing_passcode_buffer) == 0) {
        UW_LOG_WARN("Failed to generate valid pin\n");
        return kUwStatusPairingPinCodeGenerationFailed;
      }
      break;
    case PRIVET_INFO_AUTH_VALUE_PAIRING_EMBEDDED:
      pairing_type = kUwPairingTypeEmbeddedCode;
      if (!(pairing_type & supported_pairing_types)) {
        UW_LOG_WARN("Embedded code pairing unsupported\n");
        return kUwStatusPairingEmbeddedCodeTypeUnsupported;
      }
      char* embedded_code;
      UwSettings* settings = privet_request->session->device->settings;
      if (!get_embedded_code_(settings, &embedded_code)) {
        UW_LOG_WARN("Failed to get embedded code\n");
        return kUwStatusPairingEmbeddedCodeProviderFailed;
      }
      if (!uw_buffer_append(&pairing_passcode_buffer, (uint8_t*)embedded_code,
                            strlen(embedded_code))) {
        UW_LOG_WARN("Failed to append embedded code\n");
        return kUwStatusPairingEmbeddedCodeAppendFailed;
      }
      break;
    default:
      UW_LOG_WARN(
          "Privet /pairing/start param 'pairing' unsupported value: %d\n",
          pairing_param.value.int_value);
      return kUwStatusPrivetInvalidParam;
  }

  switch (crypto_param.value.int_value) {
    case PRIVET_INFO_AUTH_VALUE_CRYPTO_SPAKE_P224:
      // Break to the code below for now. Move the code below to a function when
      // other crypto modes are supported.
      break;
    default:
      UW_LOG_WARN(
          "Privet /pairing/start param 'crypto' unsupported value: %d\n",
          crypto_param.value.int_value);
      return kUwStatusPrivetInvalidParam;
  }

  UwSession* session = uw_privet_request_get_session_(privet_request);
  UwSpakeState* spake_state = &session->server_spake_state;
  uw_spake_init_(spake_state, true, pairing_buf,
                 uw_buffer_get_length(&pairing_passcode_buffer));
  uint8_t* device_commitment_buf = session->device_commitment_buf;
  UwBuffer device_commitment;
  uw_buffer_init(&device_commitment, device_commitment_buf,
                 UW_SPAKE_P224_POINT_SIZE);

  if (!uw_spake_compute_commitment_(spake_state, &device_commitment)) {
    return kUwStatusPrivetInvalidParam;
  }

  session->pairing_session_id = generate_session_id();

  // Inform the pairing callback that pairing is beginning.
  UwSettings* settings = privet_request->session->device->settings;
  if (settings->pairing_callback.begin != NULL) {
    UwPairingBegin pairing_begin_callback = settings->pairing_callback.begin;
    if (!pairing_begin_callback(session->pairing_session_id, pairing_type,
                                (const char*)pairing_buf)) {
      UW_LOG_WARN("Callback failed during paring start\n");
      return kUwStatusPrivetInvalidParam;
    }
  }

  UwMapValue result[] = {
      {.key = uw_value_int(PRIVET_PAIRING_START_KEY_SESSION_ID),
       .value = uw_value_int(session->pairing_session_id)},
      {.key = uw_value_int(PRIVET_PAIRING_START_KEY_DEVICE_COMMITMENT),
       .value = uw_value_byte_array(device_commitment_buf,
                                    UW_SPAKE_P224_POINT_SIZE)}};

  UwValue result_value =
      uw_value_map(result, uw_value_map_count(sizeof(result)));

  return uw_privet_request_reply_privet_ok_(privet_request, &result_value);
}

UwValue serialize_macaroon_to_value_(UwMacaroon* macaroon,
                                     uint8_t* buffer,
                                     size_t buf_len) {
  size_t macaroon_serialized_len;
  if (!uw_macaroon_serialize_(macaroon, buffer, buf_len,
                              &macaroon_serialized_len)) {
    UW_LOG_WARN("Error serializing macaroon.\n");
    return uw_value_null();
  }

  return uw_value_byte_array(buffer, macaroon_serialized_len);
}

static bool serialize_macaroons_to_buffer_(
    UwMacaroon* cat_macaroon,
    UwMacaroon* sat_macaroon,
    UwBuffer* serialized_macaroons_buffer) {
  uint8_t cat_macaroon_serialized_bytes[50] = {};
  UwValue cat_macaroon_value =
      serialize_macaroon_to_value_(cat_macaroon, cat_macaroon_serialized_bytes,
                                   sizeof(cat_macaroon_serialized_bytes));

  if (cat_macaroon_value.type == kUwValueTypeNull) {
    UW_LOG_WARN("Error serializing client authorization token.\n");
    return false;
  }

  uint8_t sat_macaroon_serialized_bytes[50] = {};
  UwValue sat_macaroon_value =
      serialize_macaroon_to_value_(sat_macaroon, sat_macaroon_serialized_bytes,
                                   sizeof(sat_macaroon_serialized_bytes));

  if (sat_macaroon_value.type == kUwValueTypeNull) {
    UW_LOG_WARN("Error serializing client authorization token.\n");
    return false;
  }

  UwMapValue serialized_macaroons[] = {
      {.key = uw_value_int(PRIVET_PAIRING_CONFIRM_KEY_PAIRING_CAT_MACAROON),
       .value = cat_macaroon_value},
      {.key = uw_value_int(PRIVET_PAIRING_CONFIRM_KEY_SAT_MACAROON),
       .value = sat_macaroon_value}};

  UwValue serialized_macaroons_value = uw_value_map(
      serialized_macaroons, uw_value_map_count(sizeof(serialized_macaroons)));

  UwStatus encoding_status = uw_value_encode_value_to_buffer_(
      serialized_macaroons_buffer, &serialized_macaroons_value);

  if (!uw_status_is_success(encoding_status)) {
    UW_LOG_WARN("Error encoding tokens.\n");
    return false;
  }

  return true;
}

static UwStatus set_time_from_encrypted_timestamp_(
    UwValue* encrypted_timestamp_param,
    uint8_t* ephemeral_pairing_key) {
  UwBuffer encrypted_buffer;
  uw_buffer_init(&encrypted_buffer,
                 (uint8_t*)encrypted_timestamp_param->value.byte_string_value,
                 encrypted_timestamp_param->length);
  uw_buffer_set_length_(&encrypted_buffer, encrypted_timestamp_param->length);

  uint8_t cbor_buf[30];
  UwBuffer decrypted_buffer;
  uw_buffer_init(&decrypted_buffer, cbor_buf, sizeof(cbor_buf));

  static const uint8_t timestamp_nonce = 0;

  if (!uw_eax_decrypt_(ephemeral_pairing_key, /* tag_length */ 12,
                       &timestamp_nonce, sizeof(timestamp_nonce), /* ad */ NULL,
                       /* ad_length */ 0, &encrypted_buffer,
                       &decrypted_buffer)) {
    return UW_STATUS_AND_LOG_WARN(kUwStatusInvalidArgument,
                                  "Error decrypting timestamp.\n");
  }

  UwValue timestamp_param = uw_value_undefined();

  UwMapFormat format[] = {
      {.key = uw_value_int(PRIVET_PAIRING_CONFIRM_TIMESTAMP_MAP_KEY_TIMESTAMP),
       .type = kUwValueTypeInt64,
       .value = &timestamp_param},
  };

  if (!uw_status_is_success(
          uw_value_scan_map(&decrypted_buffer, format,
                            uw_value_scan_map_count(sizeof(format))))) {
    return UW_STATUS_AND_LOG_WARN(kUwStatusInvalidArgument,
                                  "Error parsing timestamp.\n");
  }

  if (uw_value_is_undefined(&timestamp_param)) {
    return UW_STATUS_AND_LOG_WARN(kUwStatusInvalidArgument,
                                  "Timestamp not found after parsing.\n");
  }

  return uw_time_set_timestamp_seconds_(timestamp_param.value.int_value,
                                        kUwTimeSourceOwner);
}

UwStatus uw_pairing_confirm_reply_(UwPrivetRequest* privet_request,
                                   UwDeviceCrypto* device_crypto) {
  UwBuffer* privet_param_buffer =
      uw_privet_request_get_param_buffer_(privet_request);

  if (uw_buffer_is_null(privet_param_buffer)) {
    return kUwStatusPrivetInvalidParam;
  }

  UwValue session_id_param = uw_value_undefined();
  UwValue client_commitment_param = uw_value_undefined();
  UwValue encrypted_timestamp_param = uw_value_undefined();

  UwMapFormat format[] = {
      {.key = uw_value_int(PRIVET_PAIRING_CONFIRM_KEY_SESSION_ID),
       .type = kUwValueTypeInt,
       .value = &session_id_param},
      {.key = uw_value_int(PRIVET_PAIRING_CONFIRM_KEY_CLIENT_COMMITMENT),
       .type = kUwValueTypeByteString,
       .value = &client_commitment_param},
      {.key = uw_value_int(PRIVET_PAIRING_CONFIRM_KEY_TIMESTAMP),
       .type = kUwValueTypeByteString,
       .value = &encrypted_timestamp_param},
  };

  if (!uw_status_is_success(
          uw_value_scan_map(privet_param_buffer, format,
                            uw_value_scan_map_count(sizeof(format))))) {
    return UW_STATUS_AND_LOG_WARN(
        kUwStatusPrivetInvalidParam,
        "Error parsing /pairing/confirm parameters\n");
  }

  bool has_session_id = !uw_value_is_undefined(&session_id_param);
  bool has_client_commitment = !uw_value_is_undefined(&client_commitment_param);

  if (!has_session_id || !has_client_commitment) {
    return UW_STATUS_AND_LOG_WARN(
        kUwStatusPrivetInvalidParam,
        "Privet /pairing/confirm required param missing:%s%s\n",
        (has_session_id ? "" : " session_id"),
        (has_client_commitment ? "" : " client_commitment"));
  }

  UwSession* session = uw_privet_request_get_session_(privet_request);

  uint32_t session_id = session->pairing_session_id;

  if (session_id_param.value.int_value != session_id) {
    return UW_STATUS_AND_LOG_WARN(
        kUwStatusPrivetInvalidParam,
        "Privet /pairing/confirm session_id (%u) from request doesn't match "
        "session context (%u)\n",
        (unsigned int)session_id_param.value.int_value,
        (unsigned int)session_id);
  }

  uint8_t client_commitment_bytes[UW_SPAKE_P224_POINT_SIZE];

  // The client commitment comes in as a request parameter.
  UwBuffer client_commitment;
  uw_buffer_init(&client_commitment, client_commitment_bytes,
                 UW_SPAKE_P224_POINT_SIZE);
  uw_buffer_append(&client_commitment,
                   client_commitment_param.value.byte_string_value,
                   UW_SPAKE_P224_POINT_SIZE);

  UwSpakeState* spake_state = &session->server_spake_state;

  // Merge in the client_commitment get the pairing the key.
  uint8_t ephemeral_pairing_key[UW_SPAKE_P224_POINT_SIZE];
  if (!uw_spake_finalize_(spake_state, &client_commitment,
                          ephemeral_pairing_key,
                          sizeof(ephemeral_pairing_key))) {
    return UW_STATUS_AND_LOG_WARN(kUwStatusPrivetInvalidParam,
                                  "Error finalizing SPAKE exchange.");
  }
  uw_device_crypto_remember_pairing_key_(device_crypto, ephemeral_pairing_key,
                                         sizeof(ephemeral_pairing_key),
                                         uw_time_get_timestamp_seconds_());

  // The timestamp is optional, but recommended. Set the time if present, warn
  // if it is missing.
  bool has_encrypted_timestamp =
      !uw_value_is_undefined(&encrypted_timestamp_param);
  if (!has_encrypted_timestamp) {
    UW_LOG_WARN(
        "Privet /pairing/confirm param missing optional but recommended "
        "encrypted timestamp\n");
  } else {
    if (!uw_status_is_success(set_time_from_encrypted_timestamp_(
            &encrypted_timestamp_param, ephemeral_pairing_key))) {
      return UW_STATUS_AND_LOG_WARN(
          kUwStatusPrivetInvalidParam,
          "Error setting time from encrypted timestamp.\n");
    }
  }

  // Mint a new client authorization token (CAT) that will be part of the
  // response.
  UwMacaroon cat_macaroon = {};
  uint8_t cat_bytes[110] = {};
  if (!uw_macaroon_mint_client_authorization_token_(
          device_crypto->ephemeral_pairing_key,
          sizeof(device_crypto->ephemeral_pairing_key),
          /* token str */ NULL, /* token_str_len */ 0,
          uw_time_get_timestamp_seconds_(),
          kUwMacaroonCaveatCloudServiceIdNotCloudRegistered, cat_bytes,
          sizeof(cat_bytes), &cat_macaroon)) {
    return UW_STATUS_AND_LOG_WARN(
        kUwStatusInvalidArgument,
        "Error initializing minting client authorization token.\n");
  }

  // Mint a server authentication token (SAT) for the device itself.
  uint8_t sat_nonce[UW_MACAROON_INIT_DELEGATION_NONCE_SIZE];
  if (!uwp_crypto_getrandom(sat_nonce, sizeof(sat_nonce))) {
    return UW_STATUS_AND_LOG_WARN(
        kUwStatusInvalidArgument,
        "Error initializing nonce for server authentication token.\n");
  }

  UwMacaroon sat_macaroon = {};
  uint8_t sat_bytes[80] = {};
  if (!uw_macaroon_mint_server_authentication_token_(
          device_crypto->device_authentication_key,
          sizeof(device_crypto->device_authentication_key),
          /* token str */ NULL, /* token_str_len */ 0, sat_nonce, sat_bytes,
          sizeof(sat_bytes), &sat_macaroon)) {
    return UW_STATUS_AND_LOG_WARN(
        kUwStatusInvalidArgument,
        "Error initializing minting server authentication token.\n");
  }

  // Serialize the macaroons.
  UwBuffer serialized_macaroons_buffer;
  uint8_t serialized_macaroons_bytes[100];
  uw_buffer_init(&serialized_macaroons_buffer, serialized_macaroons_bytes,
                 sizeof(serialized_macaroons_bytes));

  if (!serialize_macaroons_to_buffer_(&cat_macaroon, &sat_macaroon,
                                      &serialized_macaroons_buffer)) {
    return kUwStatusInvalidArgument;
  }

  uw_buffer_dump_for_debug_(&serialized_macaroons_buffer,
                            "Serialized CAT and SAT macaroons");

  // Encrypt CAT and SAT with pairing key using AES.
  uint8_t encrypted_tokens_bytes[100];
  UwBuffer encrypted_tokens_buffer;
  uw_buffer_init(&encrypted_tokens_buffer, encrypted_tokens_bytes,
                 sizeof(encrypted_tokens_bytes));

  static const uint8_t tokens_nonce = 1;

  if (!uw_eax_encrypt_(device_crypto->ephemeral_pairing_key,
                       /* tag_length */ 12, &tokens_nonce, sizeof(tokens_nonce),
                       /* ad */ NULL, /* ad_length */ 0,
                       &serialized_macaroons_buffer,
                       &encrypted_tokens_buffer)) {
    return UW_STATUS_AND_LOG_WARN(kUwStatusInvalidArgument,
                                  "Error encrypting tokens.\n");
  }

  // Inform the provider that pairing is ending.
  UwSettings* settings = privet_request->session->device->settings;
  if (settings->pairing_callback.end != NULL) {
    UwPairingEnd pairing_end_callback = settings->pairing_callback.end;
    if (!pairing_end_callback(session_id)) {
      return UW_STATUS_AND_LOG_WARN(kUwStatusPrivetInvalidParam,
                                    "Callback failed during paring end\n");
    }
  }

  UwMapValue result[] = {
      {.key = uw_value_int(PRIVET_PAIRING_CONFIRM_KEY_ENCRYPTED_TOKENS),
       .value = uw_value_byte_array(
           encrypted_tokens_bytes,
           uw_buffer_get_length(&encrypted_tokens_buffer))}};

  UwValue result_value =
      uw_value_map(result, uw_value_map_count(sizeof(result)));

  return uw_privet_request_reply_privet_ok_(privet_request, &result_value);
}
