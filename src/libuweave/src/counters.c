// Copyright 2016 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/counters.h"

#include <assert.h>
#include <stdbool.h>

#include "src/buffer.h"
#include "src/privet_defines.h"
#include "src/value.h"
#include "src/value_scan.h"
#include "src/time.h"
#include "src/log.h"
#include "uweave/provider/crypto.h"
#include "uweave/provider/storage.h"

static const size_t kCounterBufLen = 512;

size_t uw_counter_set_sizeof(size_t app_counter_count) {
  return sizeof(UwCounterSet) +
         (sizeof(UwCounter) * (kUwInternalCounterLast + app_counter_count));
}

void uw_counter_set_init(UwCounterSet* counter_set,
                         const UwCounterId app_ids[],
                         size_t app_counter_count) {
  UwCounter* uw_counters =
      (UwCounter*)((void*)counter_set + sizeof(UwCounterSet));
  UwCounter* app_counters =
      (UwCounter*)((void*)counter_set + sizeof(UwCounterSet) +
                   sizeof(UwCounter) * kUwInternalCounterLast);

  *counter_set = (UwCounterSet){.uw_counter_count = kUwInternalCounterLast,
                                .uw_counters = uw_counters,
                                .app_counter_count = app_counter_count,
                                .app_counters = app_counters};

  if (uwp_time_is_time_set()) {
    counter_set->generation_time = uw_time_get_timestamp_seconds_();
  }

  // We set the generation id to a random value if the read from storage fails.

  for (size_t i = 0; i < kUwInternalCounterLast; ++i) {
    uw_counters[i] = (UwCounter){.id = i};
  }

  for (size_t i = 0; i < app_counter_count; ++i) {
    app_counters[i] = (UwCounter){.id = app_ids[i]};
  }
}

static UwStatus metric_encoding_callback_(UwValueCallbackMapContext* context,
                                          const void* data,
                                          size_t index) {
  const UwCounter* counters = (const UwCounter*)data;
  UwMapValue map_value = {.key = uw_value_int(counters[index].id),
                          .value = uw_value_int(counters[index].value)};

  return uw_value_callback_map_append(context, &map_value);
}

UwStatus uw_counter_set_encode_(UwCounterSet* counter_set,
                                UwCountersValueCallback callback,
                                void* callback_data) {
  // Set the generation time if time is set.
  if (counter_set->generation_time == 0 && uwp_time_is_time_set()) {
    counter_set->generation_time = uw_time_get_timestamp_seconds_();
  }

  UwMapValue values[5] = {};
  size_t value_count = 0;

  values[value_count++] =
      (UwMapValue){.key = uw_value_int(PRIVET_DEBUG_METRICS_KEY_GENERATION_ID),
                   .value = uw_value_int64(counter_set->generation_id)};

  values[value_count++] = (UwMapValue){
      .key = uw_value_int(PRIVET_DEBUG_METRICS_KEY_GENERATION_TIMESTAMP),
      .value = uw_value_int64(counter_set->generation_time)};

  if (uwp_time_is_time_set()) {
    values[value_count++] = (UwMapValue){
        .key = uw_value_int(PRIVET_DEBUG_METRICS_KEY_TIMESTAMP_NOW),
        .value = uw_value_int64(uw_time_get_timestamp_seconds_())};
  }

  if (counter_set->uw_counter_count > 0) {
    values[value_count++] =
        (UwMapValue){.key = uw_value_int(PRIVET_DEBUG_METRICS_KEY_METRICS),
                     .value = uw_value_callback_map(
                         &metric_encoding_callback_, counter_set->uw_counters,
                         counter_set->uw_counter_count)};
  }

  if (counter_set->app_counter_count > 0) {
    values[value_count++] = (UwMapValue){
        .key = uw_value_int(PRIVET_DEBUG_METRICS_KEY_VENDOR_METRICS),
        .value = uw_value_callback_map(&metric_encoding_callback_,
                                       counter_set->app_counters,
                                       counter_set->app_counter_count)};
  }

  UwValue metric_value = uw_value_map(values, value_count);

  return callback(callback_data, &metric_value);
}

static UwStatus encode_to_buffer_(void* data, const UwValue* value) {
  return uw_value_encode_value_to_buffer_((UwBuffer*)data, value);
}

UwStatus uw_counter_set_serialize_(UwCounterSet* counter_set,
                                   uint8_t* buf,
                                   size_t buf_len,
                                   size_t* result_length) {
  UwBuffer buffer;
  uw_buffer_init(&buffer, buf, buf_len);

  UwStatus encode_status =
      uw_counter_set_encode_(counter_set, &encode_to_buffer_, &buffer);
  if (!uw_status_is_success(encode_status)) {
    return encode_status;
  }

  *result_length = uw_buffer_get_length(&buffer);
  return kUwStatusSuccess;
}

UwStatus uw_counter_set_deserialize_(UwCounterSet* counter_set,
                                     const uint8_t* buf,
                                     size_t buf_len) {
  if (buf_len == 0) {
    return kUwStatusNotFound;
  }

  UwValue binary_cbor_value = uw_value_binary_cbor(buf, buf_len);

  UwValue generation_id = uw_value_undefined();
  UwValue generation_timestamp = uw_value_undefined();
  UwValue vendor_map = uw_value_undefined();
  UwMapFormat format[] = {
      {.key = uw_value_int(PRIVET_DEBUG_METRICS_KEY_GENERATION_ID),
       .type = kUwValueTypeInt,
       .value = &generation_id},
      {.key = uw_value_int(PRIVET_DEBUG_METRICS_KEY_GENERATION_TIMESTAMP),
       .type = kUwValueTypeInt64,
       .value = &generation_timestamp},
      {.key = uw_value_int(PRIVET_DEBUG_METRICS_KEY_VENDOR_METRICS),
       .type = kUwValueTypeBinaryCbor,
       .value = &vendor_map},
  };

  UwStatus scan_status = uw_value_scan_map_with_value(
      &binary_cbor_value, format, uw_value_scan_map_count(sizeof(format)));
  if (!uw_status_is_success(scan_status)) {
    return scan_status;
  }

  if (uw_value_is_undefined(&generation_id) ||
      uw_value_is_undefined(&generation_timestamp)) {
    return kUwStatusInvalidArgument;
  }

  counter_set->generation_id = generation_id.value.int_value;
  counter_set->generation_time = generation_timestamp.value.int64_value;

  // TODO(jmccullough): add weave counters.

  if (!uw_value_is_undefined(&vendor_map)) {
    for (size_t i = 0; i < counter_set->app_counter_count; ++i) {
      UwValue counter_value = uw_value_undefined();
      uw_value_scan_map_lookup_(&vendor_map,
                                uw_value_int(counter_set->app_counters[i].id),
                                kUwValueTypeInt, &counter_value);
      if (!uw_value_is_undefined(&counter_value)) {
        counter_set->app_counters[i].value = counter_value.value.int_value;
      }
    }
  }

  return kUwStatusSuccess;
}

UwStatus uw_counter_set_write_to_storage_(UwCounterSet* counter_set) {
  if (counter_set == NULL) {
    return kUwStatusInvalidArgument;
  }

  uint8_t counter_buf[kCounterBufLen];
  memset(counter_buf, 0, sizeof(counter_buf));
  size_t result_len = 0;

  UwStatus serialize_status = uw_counter_set_serialize_(
      counter_set, counter_buf, sizeof(counter_buf), &result_len);

  // Pad to the nearest aligned value.
  result_len = uwp_storage_size_align(result_len);

  if (!uw_status_is_success(serialize_status)) {
    return serialize_status;
  }

  UwStatus write_status =
      uwp_storage_put(kUwStorageFileNameCounters, counter_buf, result_len);
  UW_LOG_INFO("Wrote counters with status %d\n", write_status);

  return write_status;
}

UwStatus uw_counter_set_read_from_storage_(UwCounterSet* counter_set) {
  uint8_t counter_buf[kCounterBufLen];
  size_t counter_buf_len = 0;

  UwStatus status = uwp_storage_get(kUwStorageFileNameCounters, counter_buf,
                                    sizeof(counter_buf), &counter_buf_len);

  // If the read succeeds, deserialize.
  if (uw_status_is_success(status)) {
    status =
        uw_counter_set_deserialize_(counter_set, counter_buf, counter_buf_len);
  }
  // If the deserialization or read failed, initialize the generation_id with a
  // new value.
  if (!uw_status_is_success(status)) {
    if (!uwp_crypto_getrandom((uint8_t*)&counter_set->generation_id,
                              sizeof(counter_set->generation_id))) {
      UW_LOG_WARN("Failed to generate counter set generation id\n");
    }
  }
  return status;
}

UwStatus uw_counter_set_try_coalesce_(UwCounterSet* counter_set) {
  // If no changes, continue.
  if (counter_set->earliest_change_time == 0) {
    return kUwStatusSuccess;
  }

  time_t ticks = uw_time_get_uptime_seconds_();
  if ((ticks - counter_set->earliest_change_time) <
      kUwCounterCoalesceIntervalSeconds) {
    return kUwStatusSuccess;
  }

  // Clear the dirty status.
  counter_set->earliest_change_time = 0;

  return uw_counter_set_write_to_storage_(counter_set);
}
