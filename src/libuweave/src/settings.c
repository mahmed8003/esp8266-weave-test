// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/settings.h"

#include "src/buffer.h"
#include "src/log.h"
#include "src/value.h"
#include "src/value_scan.h"
#include "src/uw_assert.h"
#include "uweave/provider/storage.h"
#include "uweave/value.h"
#include "uweave/value_scan.h"

#include <string.h>

#define UW_SETTINGS_STORAGE_BUF_LEN 512

#define UW_SETTINGS_STORAGE_KEY_NAME 1

void uw_settings_get_model_manifest_id(const UwSettings* settings,
                                       char model_manifest_id[],
                                       size_t model_manifest_id_len) {
  UW_ASSERT(model_manifest_id_len == 5,
            "Expected 5 bytes for model_manifest_id\n");
  // The first two bytes are the device class.
  memcpy(model_manifest_id, settings->device_class, 2);
  // The next three bytes are the model ID.
  memcpy(model_manifest_id + 2, settings->model_id, 3);
}

UwStatus uw_settings_read_from_storage_(UwSettings* settings) {
  uint8_t settings_buf[UW_SETTINGS_STORAGE_BUF_LEN];
  size_t settings_len;
  UwStatus status = uwp_storage_get(kUwStorageFileNameSettings, settings_buf,
                                    sizeof(settings_buf), &settings_len);

  if (!uw_status_is_success(status)) {
    UW_LOG_WARN("Failed to read settings: %d\n", status);
    return status;
  }

  UwBuffer buffer;
  uw_buffer_init(&buffer, settings_buf, sizeof(settings_buf));
  uw_buffer_set_length_(&buffer, settings_len);

  UwValue name_value = uw_value_undefined();

  UwMapFormat format[] = {{.key = uw_value_int(UW_SETTINGS_STORAGE_KEY_NAME),
                           .type = kUwValueTypeUTF8String,
                           .value = &name_value}};

  UwStatus scan_status = uw_value_scan_map(
      &buffer, format, uw_value_scan_map_count(sizeof(format)));
  if (!uw_status_is_success(scan_status)) {
    UW_LOG_WARN("Failed to parse settings: %d\n", scan_status);
    return scan_status;
  }

  if (!uw_value_is_undefined(&name_value)) {
    uw_value_copy_string(&name_value, settings->name, sizeof(settings->name));
  }

  return kUwStatusSuccess;
}

UwStatus uw_settings_write_to_storage_(UwSettings* settings) {
  uint8_t settings_buf[UW_SETTINGS_STORAGE_BUF_LEN];
  memset(settings_buf, 0, sizeof(settings_buf));

  int settings_count = 0;
  UwMapValue settings_items[1] = {{}};

  size_t name_len = strlen(settings->name);
  if (name_len > 0) {
    settings_items[settings_count++] = (UwMapValue){
        .key = uw_value_int(UW_SETTINGS_STORAGE_KEY_NAME),
        .value = uw_value_utf8_string_with_length(settings->name, name_len)};
  }

  UwValue settings_value = uw_value_map(settings_items, settings_count);

  CborEncoder encoder;
  cbor_encoder_init(&encoder, settings_buf, sizeof(settings_buf), 0);

  UwStatus encoding_status = uw_value_encode_value_(&encoder, &settings_value);
  if (!uw_status_is_success(encoding_status)) {
    UW_LOG_WARN("Settings encoding failure: %d\n", encoding_status);
    return encoding_status;
  }

  size_t settings_len = uwp_storage_size_align(encoder.ptr - settings_buf);

  UwStatus store_status =
      uwp_storage_put(kUwStorageFileNameSettings, settings_buf, settings_len);
  if (!uw_status_is_success(store_status)) {
    UW_LOG_WARN("Error writing settings: %d\n", store_status);
  }
  return store_status;
}
