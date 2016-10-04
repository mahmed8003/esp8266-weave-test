// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/setup_request.h"

#include "src/ble_advertising.h"
#include "src/counters.h"
#include "src/log.h"
#include "src/privet_defines.h"
#include "src/privet_request.h"
#include "src/settings.h"
#include "src/time.h"
#include "tinycbor/src/cbor.h"
#include "uweave/ble_transport.h"
#include "uweave/config.h"
#include "uweave/status.h"
#include "uweave/value_scan.h"

UwStatus uw_setup_request_(UwPrivetRequest* setup_request,
                           UwSettings* settings) {
  UwBuffer* privet_param_buffer =
      uw_privet_request_get_param_buffer_(setup_request);

  if (uw_buffer_is_null(privet_param_buffer)) {
    UW_LOG_WARN("param buffer is null\n");
    return kUwStatusPrivetInvalidParam;
  }

  // TODO(dkhawk): Handle these other parameters
  // Use the description
  // Use the location
  // Use the GCD data
  // Use the WiFi data
  // Pass the Vendor data back via a callback
  UwValue name = uw_value_undefined();
  UwValue timestamp = uw_value_undefined();
  UwMapFormat format[] = {
      {.key = uw_value_int(PRIVET_SETUP_KEY_NAME),
       .type = kUwValueTypeUTF8String,
       .value = &name},
      {.key = uw_value_int(PRIVET_SETUP_KEY_TIMESTAMP),
       .type = kUwValueTypeInt64,
       .value = &timestamp},
  };

  if (!uw_status_is_success(
          uw_value_scan_map(privet_param_buffer, format,
                            uw_value_scan_map_count(sizeof(format))))) {
    UW_LOG_WARN("Error parsing /setup parameters\n");
    return kUwStatusPrivetInvalidParam;
  }

  bool name_changed = false;

  if (!uw_value_is_undefined(&name)) {
    if ((name.length > UW_SETTINGS_MAX_NAME_LENGTH) ||
        (name.length > PRIVET_SETUP_NAME_MAX_LENGTH)) {
      UW_LOG_WARN("Setup `name` too long (max is 32, got %zu)\n", name.length);
      return kUwStatusTooLong;
    } else {
      name_changed = (strncmp(name.value.string_value, settings->name,
                              sizeof(settings->name)) != 0);
    }
  }

  bool timestamp_changed = false;
  time_t timestamp_seconds = 0;
  if (!uw_value_is_undefined(&timestamp)) {
    timestamp_seconds = timestamp.value.int64_value;
    // TODO(jmccullough): Sanity check the timestamp.
    // At least the unix gigasecond, (2001-08-29 15:50:55).
    if (timestamp_seconds < 1000000000) {
      UW_LOG_WARN("Invalid timestamp\n");
      return kUwStatusInvalidInput;
    }
    uw_device_increment_uw_counter_(setup_request->session->device,
                                    kUwInternalCounterSetupTimeSet);
    timestamp_changed = true;
  }

  // Only commit changes after verifying all changeable parameters.
  if (timestamp_changed) {
    uw_time_set_timestamp_seconds_(timestamp_seconds, kUwTimeSourceOwner);
  }

  // TODO(dkhawk): set the GCD, WiFi, and vendor status in the reply
  if (name_changed) {
    uw_value_copy_string(&name, settings->name, sizeof(settings->name));
    UwSession* session = uw_privet_request_get_session_(setup_request);
    uw_ble_advertising_update_data_(uw_session_get_device_(session));
    uw_settings_write_to_storage_(settings);
  }

  UwMapValue result[] = {
      {.key = uw_value_int(PRIVET_SETUP_KEY_VERSION),
       .value = uw_value_int(PRIVET_API_VALUE_VERSION)}
      // TODO(dkhawk): add wifi and gcd status (not yet supported)
  };

  UwValue result_value =
      uw_value_map(result, uw_value_map_count(sizeof(result)));

  return uw_privet_request_reply_privet_ok_(setup_request, &result_value);
}
