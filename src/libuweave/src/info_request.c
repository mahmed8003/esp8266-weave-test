// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/info_request.h"

#include <stdbool.h>
#include <stdint.h>

#include "src/buffer.h"
#include "src/device.h"
#include "src/log.h"
#include "src/privet_defines.h"
#include "src/settings.h"
#include "src/time.h"
#include "src/value.h"
#include "tinycbor/src/cbor.h"
#include "uweave/config.h"

UwValue populate_pairing_array_from_settings(const UwSettings* settings,
                                             UwValue* pairing_array,
                                             size_t pairing_array_length) {
  size_t index = 0;
  uint8_t supported_pairing_types = settings->supported_pairing_types;
  if ((supported_pairing_types & kUwPairingTypePinCode) &&
      (index < pairing_array_length)) {
    pairing_array[index++] = uw_value_int(PRIVET_INFO_AUTH_VALUE_PAIRING_PIN);
  }
  if ((supported_pairing_types & kUwPairingTypeEmbeddedCode) &&
      (index < pairing_array_length)) {
    pairing_array[index++] =
        uw_value_int(PRIVET_INFO_AUTH_VALUE_PAIRING_EMBEDDED);
  }
  return uw_value_array(pairing_array, index);
}

bool uw_info_request_set_info_(UwPrivetRequest* privet_request,
                               const UwDevice* device) {
  const UwSettings* settings = device->settings;
  const UwDeviceCrypto* device_crypto = &device->device_crypto;
  // Add default authentication map to info repsonse.
  // TODO(mcolagrosso): Populate this map with the device's supported
  // authentication.
  UwValue auth_mode_array[] = {uw_value_int(PRIVET_AUTH_MODE_VALUE_PAIRING),
                               uw_value_int(PRIVET_AUTH_MODE_VALUE_TOKEN)};
  UwValue pairing_array[2] = {};
  UwValue crypto_array[] = {
      uw_value_int(PRIVET_INFO_AUTH_VALUE_CRYPTO_SPAKE_P224)};

  UwMapValue authentication[] = {
      {.key = uw_value_int(PRIVET_INFO_AUTH_KEY_MODE),
       .value = uw_value_array(auth_mode_array,
                               uw_value_array_count(sizeof(auth_mode_array)))},
      {.key = uw_value_int(PRIVET_INFO_AUTH_KEY_PAIRING),
       .value = populate_pairing_array_from_settings(settings, pairing_array,
                                                     sizeof(pairing_array))},
      {.key = uw_value_int(PRIVET_INFO_AUTH_KEY_CRYPTO),
       .value = uw_value_array(crypto_array,
                               uw_value_array_count(sizeof(crypto_array)))}};

  char model_manifest_id[5];
  uw_settings_get_model_manifest_id(settings, model_manifest_id,
                                    sizeof(model_manifest_id));

  UwMapValue result[] = {
      {.key = uw_value_int(PRIVET_INFO_KEY_VERSION),
       .value = uw_value_int(PRIVET_API_VALUE_VERSION)},
      {.key = uw_value_int(PRIVET_INFO_KEY_AUTH),
       .value = uw_value_map(authentication,
                             uw_value_map_count(sizeof(authentication)))},
      {.key = uw_value_int(PRIVET_INFO_KEY_MODEL_MANIFEST_ID),
       .value = uw_value_utf8_string_with_length(model_manifest_id,
                                                 sizeof(model_manifest_id))},
      {.key = uw_value_int(PRIVET_INFO_KEY_DEVICE_ID),
       .value = uw_value_byte_array(device_crypto->device_id,
                                    sizeof(device_crypto->device_id))},
      {.key = uw_value_int(PRIVET_INFO_KEY_NAME),
       .value = uw_value_utf8_string(settings->name)},
      {.key = uw_value_int(PRIVET_INFO_KEY_TIMESTAMP),
       .value = uw_value_int64(uw_time_get_timestamp_seconds_())},
      {.key = uw_value_int(PRIVET_INFO_KEY_TIME_STATUS),
       .value = uw_value_int(uw_time_get_status_())},
      {.key = uw_value_int(PRIVET_INFO_KEY_BUILD),
       .value = uw_value_utf8_string("v2.3.0")},
  };

  UwValue result_value =
      uw_value_map(result, uw_value_map_count(sizeof(result)));

  return uw_privet_request_reply_privet_ok_(privet_request, &result_value);
}
