// Copyright 2016 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/access_control_request.h"

#include "src/ble_advertising.h"
#include "src/counters.h"
#include "src/device_crypto.h"
#include "src/macaroon.h"
#include "src/macaroon_helpers.h"
#include "src/privet_defines.h"
#include "src/value_scan.h"
#include "src/log.h"
#include "src/time.h"

#define CLIENT_ACCESS_TOKEN_BUF_LENGTH 128

UwStatus uw_access_control_request_claim_(UwDevice* device,
                                          UwPrivetRequest* privet_request) {
  if (!uw_privet_request_is_secure(privet_request)) {
    return UW_STATUS_AND_LOG_WARN(kUwStatusEncryptionRequired,
                                  "Encryption required.\n");
  }
  if (!uw_session_is_access_control_authorized(privet_request->session)) {
    return UW_STATUS_AND_LOG_WARN(kUwStatusPairingRequired,
                                  "Not authenticated from /pairing\n");
  }
  uw_device_increment_uw_counter_(device, kUwInternalCounterAccessControlClaim);

  UwDeviceCrypto* device_crypto = &device->device_crypto;

  uint8_t pending_key[UW_DEVICE_CRYPTO_MACAROON_KEY_LEN];
  UwStatus key_status = uw_device_crypto_generate_pending_client_authz_key_(
      device_crypto, pending_key);

  if (!uw_status_is_success(key_status)) {
    UW_LOG_WARN("Error generating key\n");
    return kUwStatusVerificationFailed;
  }

  UwMacaroon client_authz_token;

  uint8_t creation_buf[CLIENT_ACCESS_TOKEN_BUF_LENGTH];
  if (!uw_macaroon_mint_client_authorization_token_(
          pending_key, sizeof(pending_key), NULL, 0,
          uw_macaroon_unix_epoch_to_j2000(uw_time_get_timestamp_seconds_()),
          kUwMacaroonCaveatCloudServiceIdNotCloudRegistered, creation_buf,
          sizeof(creation_buf), &client_authz_token)) {
    UW_LOG_WARN("Error creating client authz token\n");
    return kUwStatusVerificationFailed;
  }

  uint8_t serialize_buf[CLIENT_ACCESS_TOKEN_BUF_LENGTH];
  size_t result_len;
  if (!uw_macaroon_serialize_(&client_authz_token, serialize_buf,
                              sizeof(serialize_buf), &result_len)) {
    UW_LOG_WARN("Error serializing client authz token\n");
    return kUwStatusVerificationFailed;
  }

  UwMapValue result_map[] = {
      {.key =
           uw_value_int(PRIVET_ACCESS_CONTROL_CLAIM_RESPONSE_KEY_CLIENT_TOKEN),
       .value = uw_value_byte_array(serialize_buf, result_len)}};

  UwValue result_value =
      uw_value_map(result_map, uw_value_map_count(sizeof(result_map)));

  return uw_privet_request_reply_privet_ok_(privet_request, &result_value);
}

UwStatus uw_access_control_request_confirm_(UwDevice* device,
                                            UwPrivetRequest* privet_request) {
  if (!uw_session_is_access_control_authorized(privet_request->session)) {
    return UW_STATUS_AND_LOG_WARN(kUwStatusPairingRequired,
                                  "Not authenticated from /pairing\n");
  }

  uw_device_increment_uw_counter_(device,
                                  kUwInternalCounterAccessControlConfirm);

  UwDeviceCrypto* device_crypto = &device->device_crypto;
  UwValue client_token_param = uw_value_undefined();

  UwMapFormat format[] = {
      {.key =
           uw_value_int(PRIVET_ACCESS_CONTROL_CONFIRM_REQUEST_KEY_CLIENT_TOKEN),
       .type = kUwValueTypeByteString,
       .value = &client_token_param}};

  UwStatus scan_status =
      uw_value_scan_map(uw_privet_request_get_param_buffer_(privet_request),
                        format, uw_value_scan_map_count(sizeof(format)));

  if (!uw_status_is_success(scan_status)) {
    return UW_STATUS_AND_LOG_WARN(scan_status, "Parmeter parse error\n");
  }

  if (uw_value_is_undefined(&client_token_param)) {
    return UW_STATUS_AND_LOG_WARN(kUwStatusInvalidInput,
                                  "Access Token Not Found\n");
  }

  uint8_t macaroon_buf[CLIENT_ACCESS_TOKEN_BUF_LENGTH];
  UwMacaroon client_access_token;
  if (!uw_macaroon_deserialize_(client_token_param.value.byte_string_value,
                                client_token_param.length, macaroon_buf,
                                sizeof(macaroon_buf), &client_access_token)) {
    return UW_STATUS_AND_LOG_WARN(
        kUwStatusVerificationFailed,
        "Failed to deserialize client access token\n");
  }

  UwMacaroonContext macaroon_context;
  if (!uw_macaroon_context_create_with_timestamp_(
          uw_macaroon_unix_epoch_to_j2000(uw_time_get_timestamp_seconds_()),
          &macaroon_context)) {
    return UW_STATUS_AND_LOG_WARN(kUwStatusInvalidArgument,
                                  "Macaroon context creation failed\n");
  }
  UwMacaroonValidationResult validation_result = {};
  if (device_crypto->has_pending_client_authz_key) {
    if (!uw_macaroon_validate_(
            &client_access_token,
            device_crypto->pending_client_authorization_key,
            sizeof(device_crypto->pending_client_authorization_key),
            &macaroon_context, &validation_result)) {
      return UW_STATUS_AND_LOG_WARN(kUwStatusVerificationFailed,
                                    "Failed to verify client authz token\n");
    }
    UwStatus commit_status =
        uw_device_crypto_commit_pending_client_authz_key_(device_crypto);
    if (!uw_status_is_success(commit_status)) {
      return commit_status;
    }
  } else {
    // If we confirmed the token and committed it, but the client lost the
    // response, re-check the current token to see if it worked.  If not,
    // re-pairing is required.
    if (!uw_macaroon_validate_(&client_access_token,
                               device_crypto->client_authorization_key,
                               sizeof(device_crypto->client_authorization_key),
                               &macaroon_context, &validation_result)) {
      return UW_STATUS_AND_LOG_WARN(
          kUwStatusVerificationFailed,
          "Failed to verify committed client authz token\n");
    }
  }

  uw_ble_advertising_update_data_(device);

  UwValue empty_reply = uw_value_map(NULL, 0);
  return uw_privet_request_reply_privet_ok_(privet_request, &empty_reply);
}
