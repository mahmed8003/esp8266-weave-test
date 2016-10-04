// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/auth_request.h"

#include "src/counters.h"
#include "src/log.h"
#include "src/macaroon.h"
#include "src/privet_defines.h"
#include "src/privet_request.h"
#include "src/time.h"
#include "uweave/status.h"
#include "uweave/value.h"
#include "uweave/value_scan.h"
#include "uweave/provider/time.h"

static UwStatus validate_macaroon_(
    const UwValue* auth_code,
    const uint8_t* key,
    size_t key_len,
    const uint8_t* ble_session_id,
    size_t ble_session_id_len,
    UwMacaroonValidationResult* validation_result) {
  uint8_t macaroon_buffer[128];
  UwMacaroon macaroon = {};
  if (!uw_macaroon_deserialize_(auth_code->value.byte_string_value,
                                auth_code->length, macaroon_buffer,
                                sizeof(macaroon_buffer), &macaroon)) {
    return UW_STATUS_AND_LOG_WARN(kUwStatusInvalidInput,
                                  "Failed to deserialize macaroon\n");
  }

  UwMacaroonContext macaroon_context = {};
  if (!uw_macaroon_context_create_(
          uw_macaroon_unix_epoch_to_j2000(uw_time_get_timestamp_seconds_()),
          ble_session_id, ble_session_id_len, NULL /* auth_challenge_str */,
          0 /* auth_challenge_str_len */, &macaroon_context)) {
    return UW_STATUS_AND_LOG_WARN(kUwStatusInvalidArgument,
                                  "Macaroon context creation failed\n");
  }

  if (!uw_macaroon_validate_(&macaroon, key, key_len, &macaroon_context,
                             validation_result)) {
    // TODO(jmccullough): Promote to more specific error.
    return UW_STATUS_AND_LOG_WARN(kUwStatusVerificationFailed,
                                  "Macaroon validation failed\n");
  }
  return kUwStatusSuccess;
}

UwStatus uw_auth_request_handler_(UwDevice* device,
                                  UwPrivetRequest* auth_request) {
  if (!uw_privet_request_is_secure(auth_request)) {
    return kUwStatusEncryptionRequired;
  }

  UwBuffer* privet_param_buffer =
      uw_privet_request_get_param_buffer_(auth_request);

  if (uw_buffer_is_null(privet_param_buffer)) {
    return UW_STATUS_AND_LOG_WARN(kUwStatusPrivetInvalidParam,
                                  "Malformed auth_request structure\n");
  }

  UwSession* session = uw_privet_request_get_session_(auth_request);
  if (!uw_session_is_valid_(session)) {
    return UW_STATUS_AND_LOG_WARN(kUwStatusAuthenticationRequired,
                                  "Invalid session\n");
  }

  UwValue mode = uw_value_undefined();
  UwValue auth_code = uw_value_undefined();

  UwMapFormat format[] = {
      {.key = uw_value_int(PRIVET_AUTH_KEY_MODE),
       .type = kUwValueTypeInt,
       .value = &mode},
      {.key = uw_value_int(PRIVET_AUTH_KEY_AUTH_CODE),
       .type = kUwValueTypeByteString,
       .value = &auth_code},
  };

  if (!uw_status_is_success(
          uw_value_scan_map(privet_param_buffer, format,
                            uw_value_scan_map_count(sizeof(format))))) {
    // Make sure the session is valid, but unprivileged.
    uw_session_start_valid_(session);
    return UW_STATUS_AND_LOG_WARN(kUwStatusPrivetInvalidParam,
                                  "Error parsing /auth parameters\n");
  }

  bool has_mode = !uw_value_is_undefined(&mode);
  bool has_auth_code = !uw_value_is_undefined(&auth_code);

  if (!has_mode || !has_auth_code) {
    return UW_STATUS_AND_LOG_WARN(kUwStatusPrivetInvalidParam,
                                  "Privet auth param missing required:%s%s\n",
                                  (has_mode ? "" : " mode"),
                                  (has_auth_code ? "" : "auth_code"));
  }

  UwRole role = kUwRoleUnspecified;
  time_t expiration_time = 0;

  switch (mode.value.int_value) {
    case PRIVET_AUTH_MODE_VALUE_ANONYMOUS: {
      // TODO(jmccullough): Re-enable when we have a anonymous encryption story.
      return UW_STATUS_AND_LOG_WARN(kUwStatusInvalidInput,
                                    "Anonymous access denied\n");
    }
    case PRIVET_AUTH_MODE_VALUE_PAIRING: {
      uw_device_increment_uw_counter_(device, kUwInternalCounterAuthPairing);
      // TODO(jmccullough): STOPSHIP limit ephemeral pairing timestamp.
      if (!device->device_crypto.ephemeral_issue_timestamp) {
        return UW_STATUS_AND_LOG_WARN(
            kUwStatusPairingRequired,
            "Ephemeral pairing key timestamp invalid\n");
      }
      UwMacaroonValidationResult validation_result = {};
      UwStatus validation_status = validate_macaroon_(
          &auth_code, device->device_crypto.ephemeral_pairing_key,
          sizeof(device->device_crypto.ephemeral_pairing_key),
          NULL /* ble_session_id */, 0 /* ble_session_id_len */,
          &validation_result);
      if (!uw_status_is_success(validation_status)) {
        return UW_STATUS_AND_LOG_WARN(validation_status,
                                      "Failed to auth with pairing token\n");
      }

      role = (UwRole)validation_result.granted_scope;
      uw_session_set_access_control_authorized(session, true);
      expiration_time =
          uw_macaroon_get_expiration_unix_epoch_time_(&validation_result);
      break;
    }
    case PRIVET_AUTH_MODE_VALUE_TOKEN: {
      uw_device_increment_uw_counter_(device, kUwInternalCounterAuthToken);
      UwMacaroonValidationResult validation_result = {};
      UwStatus validation_status = validate_macaroon_(
          &auth_code, device->device_crypto.client_authorization_key,
          sizeof(device->device_crypto.client_authorization_key),
          uw_channel_encryption_session_id_(&(session->crypto_state)),
          UW_BLE_SESSION_ID_LEN, &validation_result);
      if (!uw_status_is_success(validation_status)) {
        return UW_STATUS_AND_LOG_WARN(validation_status,
                                      "Failed to auth with client token\n");
      }

      role = (UwRole)validation_result.granted_scope;
      expiration_time =
          uw_macaroon_get_expiration_unix_epoch_time_(&validation_result);
      break;
    }
    default: {
      return UW_STATUS_AND_LOG_WARN(kUwStatusInvalidInput,
                                    "Unknown auth_mode\n");
    }
  }

  uw_trace_auth_result(device, mode.value.int_value, role);

  if (role == kUwRoleUnspecified) {
    return UW_STATUS_AND_LOG_WARN(kUwStatusInvalidArgument,
                                  "No role defined by token\n");
  }

  if (role > kUwRoleManager && !uwp_time_is_time_set()) {
    return UW_STATUS_AND_LOG_WARN(kUwStatusTimeRequired,
                                  "Time not set on non-owner authentication\n");
  }

  // Session limit enforcement is handled in the macaroon validation.
  if (session->expiration_time == 0 || session->expiration_time == UINT32_MAX) {
    UW_LOG_INFO("Session has no expiration.\n");
  } else {
    UW_LOG_INFO("Session is valid until: %ld\n", expiration_time);
  }

  // Only grant access to the session once all checks have passed.
  uw_session_set_role_(session, role);
  session->expiration_time = expiration_time;

  UwMapValue result[] = {
      {.key = uw_value_int(PRIVET_AUTH_RESPONSE_KEY_ROLE),
       .value = uw_value_int(role)},
      {.key = uw_value_int(PRIVET_AUTH_RESPONSE_KEY_TIME),
       .value = uw_value_int64(uw_time_get_timestamp_seconds_())},
      {.key = uw_value_int(PRIVET_AUTH_RESPONSE_KEY_TIME_STATUS),
       .value = uw_value_int(uw_time_get_status_())},
  };

  UwValue result_map = uw_value_map(result, uw_value_map_count(sizeof(result)));
  return uw_privet_request_reply_privet_ok_(auth_request, &result_map);
}
