// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/device_crypto.h"

#include "src/buffer.h"
#include "src/log.h"
#include "src/value.h"
#include "src/value_scan.h"
#include "uweave/provider/storage.h"

static void try_loading_keys_(UwDeviceCrypto* device_crypto) {
  uint8_t key_cbor_buf[UW_DEVICE_CRYPTO_BUFFER_LEN];
  size_t result_len = 0;

  UwStatus file_status = uwp_storage_get(kUwStorageFileNameKeys, key_cbor_buf,
                                         sizeof(key_cbor_buf), &result_len);
  // If we can't read the file, proceed without the keys.
  if (!uw_status_is_success(file_status)) {
    UW_LOG_WARN("Key file not read: %d\n", file_status);
    return;
  }

  if (result_len == 0) {
    UW_LOG_WARN("No key data available.\n");
    return;
  }

  UwValue device_auth_param = uw_value_undefined();
  UwValue client_authz_param = uw_value_undefined();
  UwValue device_id_param = uw_value_undefined();

  UwMapFormat keys_format[] = {
      {.key = uw_value_int(UW_DEVICE_CRYPTO_KEY_DEVICE_AUTH_KEY),
       .type = kUwValueTypeByteString,
       .value = &device_auth_param},
      {.key = uw_value_int(UW_DEVICE_CRYPTO_KEY_CLIENT_AUTHZ_KEY),
       .type = kUwValueTypeByteString,
       .value = &client_authz_param},
      {.key = uw_value_int(UW_DEVICE_CRYPTO_KEY_DEVICE_ID),
       .type = kUwValueTypeByteString,
       .value = &device_id_param},
  };

  UwBuffer buffer;
  uw_buffer_init(&buffer, key_cbor_buf, sizeof(key_cbor_buf));
  uw_buffer_set_length_(&buffer, result_len);

  UwStatus scan_result = uw_value_scan_map(
      &buffer, keys_format, uw_value_scan_map_count(sizeof(keys_format)));

  if (!uw_status_is_success(scan_result)) {
    UW_LOG_WARN("Error scanning key file: %d\n", scan_result);
    return;
  }

  if (!uw_value_is_undefined(&device_auth_param)) {
    if (device_auth_param.length == UW_MACAROON_MAC_LEN) {
      device_crypto->has_device_auth_key = true;
      memcpy(device_crypto->device_authentication_key,
             device_auth_param.value.byte_string_value,
             sizeof(device_crypto->device_authentication_key));
    } else {
      UW_LOG_WARN("Invalid device auth key len: %d\n",
                  (int)device_auth_param.length);
    }
  }

  if (!uw_value_is_undefined(&client_authz_param)) {
    if (client_authz_param.length == UW_MACAROON_MAC_LEN) {
      device_crypto->has_client_authz_key = true;
      memcpy(device_crypto->client_authorization_key,
             client_authz_param.value.byte_string_value,
             sizeof(device_crypto->client_authorization_key));
    } else {
      UW_LOG_WARN("Invalid client authz key len: %d\n",
                  (int)client_authz_param.length);
    }
  }

  if (!uw_value_is_undefined(&device_id_param)) {
    if (device_id_param.length > 0) {
      size_t len = sizeof(device_crypto->device_id);
      if (device_id_param.length < len) {
        len = device_id_param.length;
      }
      // Grab whatever is there.
      device_crypto->has_device_id = true;
      memcpy(device_crypto->device_id, device_id_param.value.byte_string_value,
             len);
    }
  }
}

static UwStatus save_keys_(UwDeviceCrypto* device_crypto) {
  size_t key_count = 0;
  UwMapValue key_data[3] = {};

  if (device_crypto->has_device_auth_key) {
    key_data[key_count++] =
        (UwMapValue){.key = uw_value_int(UW_DEVICE_CRYPTO_KEY_DEVICE_AUTH_KEY),
                     .value = uw_value_byte_array(
                         device_crypto->device_authentication_key,
                         sizeof(device_crypto->device_authentication_key))};
  }

  if (device_crypto->has_client_authz_key) {
    key_data[key_count++] =
        (UwMapValue){.key = uw_value_int(UW_DEVICE_CRYPTO_KEY_CLIENT_AUTHZ_KEY),
                     .value = uw_value_byte_array(
                         device_crypto->client_authorization_key,
                         sizeof(device_crypto->client_authorization_key))};
  }

  if (device_crypto->has_device_id) {
    key_data[key_count++] =
        (UwMapValue){.key = uw_value_int(UW_DEVICE_CRYPTO_KEY_DEVICE_ID),
                     .value = uw_value_byte_array(
                         device_crypto->device_id,
                         sizeof(device_crypto->device_authentication_key))};
  }

  UwValue persisted_value = uw_value_map(key_data, key_count);

  uint8_t key_cbor_buf[UW_DEVICE_CRYPTO_BUFFER_LEN];
  memset(key_cbor_buf, 0, sizeof(key_cbor_buf));

  CborEncoder encoder;
  cbor_encoder_init(&encoder, key_cbor_buf, sizeof(key_cbor_buf), 0);
  UwStatus encoding_result = uw_value_encode_value_(&encoder, &persisted_value);
  if (!uw_status_is_success(encoding_result)) {
    return encoding_result;
  }

  size_t len = encoder.ptr - key_cbor_buf;

  // Zero pad-out the end of the buffer to match UW_STORAGE_ALIGNMENT.
  if (len & (UW_STORAGE_ALIGNMENT - 1)) {
    len &= ~(UW_STORAGE_ALIGNMENT - 1);
    len += UW_STORAGE_ALIGNMENT;
  }

  return uwp_storage_put(kUwStorageFileNameKeys, key_cbor_buf, len);
}

UwStatus uw_device_crypto_init_(UwDeviceCrypto* device_crypto) {
  *device_crypto = (UwDeviceCrypto){};

  try_loading_keys_(device_crypto);

  bool do_save = false;

  if (!device_crypto->has_device_auth_key) {
    bool device_auth_success =
        uwp_crypto_getrandom(device_crypto->device_authentication_key,
                             sizeof(device_crypto->device_authentication_key));
    if (!device_auth_success) {
      return kUwStatusCryptoRandomNumberFailure;
    }
    device_crypto->has_device_auth_key = true;
    do_save = true;
  }

  if (!device_crypto->has_device_id) {
    bool device_id_success = uwp_crypto_getrandom(
        device_crypto->device_id, sizeof(device_crypto->device_id));
    if (!device_id_success) {
      return kUwStatusCryptoRandomNumberFailure;
    }
    device_crypto->has_device_id = true;
    do_save = true;
  }

  if (do_save) {
    UwStatus save_status = save_keys_(device_crypto);
    if (!uw_status_is_success(save_status)) {
      return save_status;
    }
  }

  return kUwStatusSuccess;
}

void uw_device_crypto_reset_(UwDeviceCrypto* device_crypto) {
  *device_crypto = (UwDeviceCrypto){};
  save_keys_(device_crypto);
  uw_device_crypto_init_(device_crypto);
}

UwStatus uw_device_crypto_remember_pairing_key_(UwDeviceCrypto* device_crypto,
                                                uint8_t* pairing_key,
                                                size_t pairing_key_len,
                                                uint64_t timestamp) {
  if (pairing_key_len != sizeof(device_crypto->ephemeral_pairing_key)) {
    return UW_STATUS_AND_LOG_WARN(
        kUwStatusInvalidInput,
        "Invalid pairing key length %d does not match %d\n",
        (int)pairing_key_len,
        (int)sizeof(device_crypto->ephemeral_pairing_key));
  }

  device_crypto->ephemeral_issue_timestamp = timestamp;
  memcpy(device_crypto->ephemeral_pairing_key, pairing_key,
         sizeof(device_crypto->ephemeral_pairing_key));
  return kUwStatusSuccess;
}

UwStatus uw_device_crypto_generate_pending_client_authz_key_(
    UwDeviceCrypto* device_crypto,
    uint8_t* key_data) {
  bool key_status = uwp_crypto_getrandom(
      device_crypto->pending_client_authorization_key,
      sizeof(device_crypto->pending_client_authorization_key));

  if (!key_status) {
    return kUwStatusCryptoRandomNumberFailure;
  }

  device_crypto->has_pending_client_authz_key = true;

  if (key_data != NULL) {
    memcpy(key_data, device_crypto->pending_client_authorization_key,
           sizeof(device_crypto->pending_client_authorization_key));
  }
  return kUwStatusSuccess;
}

UwStatus uw_device_crypto_commit_pending_client_authz_key_(
    UwDeviceCrypto* device_crypto) {
  if (!device_crypto->has_pending_client_authz_key) {
    return kUwStatusDeviceCryptoNoKeys;
  }

  memcpy(device_crypto->client_authorization_key,
         device_crypto->pending_client_authorization_key,
         sizeof(device_crypto->client_authorization_key));
  device_crypto->has_client_authz_key = true;
  UwStatus save_status = save_keys_(device_crypto);
  if (!uw_status_is_success(save_status)) {
    return save_status;
  }

  device_crypto->has_pending_client_authz_key = false;
  memset(device_crypto->pending_client_authorization_key, 0,
         sizeof(device_crypto->pending_client_authorization_key));
  return kUwStatusSuccess;
}
