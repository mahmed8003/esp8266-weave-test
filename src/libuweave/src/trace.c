// Copyright 2016 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/trace.h"

#include "src/device.h"
#include "src/privet_defines.h"

void uw_trace_log_get_range_(UwTraceLog* trace_log,
                             size_t* min_id,
                             size_t* max_id) {
  UwTraceEntry* current_entry =
      &trace_log
           ->entries[(trace_log->next_index - 1) % UW_TRACE_LOG_ENTRY_COUNT];
  // No entries yet.
  if (current_entry->type == kUwTraceTypeEmpty) {
    *min_id = 0;
    *max_id = 0;
    return;
  }

  UwTraceEntry* next_entry = &trace_log->entries[trace_log->next_index];
  // If the next entry is empty, we have not yet wrapped around and the min
  // value is at zero.  Otherwise, the next entry is the lowest id entry.
  if (next_entry->type == kUwTraceTypeEmpty) {
    *min_id = trace_log->entries[0].id;
    *max_id = current_entry->id;
  } else {
    *min_id = next_entry->id;
    *max_id = current_entry->id;
  }
}

typedef struct {
  UwTraceLog* trace_log;
  size_t start_index;
  size_t length;
} TraceEncodingContext;

static TraceEncodingContext find_entries_(UwTraceLog* trace_log,
                                          uint32_t start_id,
                                          uint32_t end_id) {
  size_t last_index = (trace_log->next_index - 1) % UW_TRACE_LOG_ENTRY_COUNT;
  UwTraceEntry* last_entry = &trace_log->entries[last_index];
  if (last_entry->type == kUwTraceTypeEmpty) {
    return (TraceEncodingContext){.length = 0};
  }

  size_t start_index = trace_log->next_index;
  UwTraceEntry* next_entry = &trace_log->entries[start_index];
  if (next_entry->type == kUwTraceTypeEmpty) {
    start_index = 0;
  }

  UwTraceEntry* first_entry = &trace_log->entries[start_index];

  uint32_t min_id = first_entry->id;
  uint32_t max_id = last_entry->id;

  // max_id is inclusive.
  uint32_t count = max_id - min_id + 1;

  // TODO(jmccullough): Deal with wraparound.
  if (start_id >= end_id || start_id > max_id || end_id < min_id) {
    return (TraceEncodingContext){.length = 0};
  }

  if (end_id < max_id) {
    uint32_t delta = max_id - end_id;
    count -= delta;
  }

  if (start_id > min_id) {
    uint32_t delta = start_id - min_id;
    start_index += delta;
    count -= delta;
  }

  if (count > kUwTraceDumpMaxEntries) {
    count = kUwTraceDumpMaxEntries;
  }

  return (TraceEncodingContext){
      .trace_log = trace_log, .start_index = start_index, .length = count};
}

static UwStatus entry_encoding_callback_(UwValueCallbackArrayContext* context,
                                         const void* data,
                                         size_t index) {
  TraceEncodingContext* encoding_context = (TraceEncodingContext*)data;
  size_t entry_index =
      (encoding_context->start_index + index) % UW_TRACE_LOG_ENTRY_COUNT;

  UwTraceEntry* entry = &encoding_context->trace_log->entries[entry_index];

  UwMapValue param_values[2];
  size_t param_count = 0;

  switch (entry->type) {
    case kUwTraceTypeAuthResult: {
      param_values[param_count++] = (UwMapValue){
          .key = uw_value_int(
              PRIVET_DEBUG_TRACE_DUMP_RESULT_ENTRY_PARAMS_KEY_AUTH_MODE),
          .value = uw_value_int(entry->u.auth.mode)};
      param_values[param_count++] = (UwMapValue){
          .key = uw_value_int(
              PRIVET_DEBUG_TRACE_DUMP_RESULT_ENTRY_PARAMS_KEY_AUTH_ROLE),
          .value = uw_value_int(entry->u.auth.role)};
      break;
    }
    case kUwTraceTypeBleEvent: {
      param_values[param_count++] = (UwMapValue){
          .key = uw_value_int(
              PRIVET_DEBUG_TRACE_DUMP_RESULT_ENTRY_PARAMS_KEY_BLE_EVENT),
          .value = uw_value_int(entry->u.ble.event)};
      param_values[param_count++] = (UwMapValue){
          .key = uw_value_int(
              PRIVET_DEBUG_TRACE_DUMP_RESULT_ENTRY_PARAMS_KEY_BLE_STATE),
          .value = uw_value_int(entry->u.ble.state)};
      break;
    }
    case kUwTraceTypeCallBegin: {
      param_values[param_count++] = (UwMapValue){
          .key = uw_value_int(
              PRIVET_DEBUG_TRACE_DUMP_RESULT_ENTRY_PARAMS_KEY_CALL_API_ID),
          .value = uw_value_int(entry->u.call.api_id)};
      break;
    }
    case kUwTraceTypeCallEnd: {
      param_values[param_count++] = (UwMapValue){
          .key = uw_value_int(
              PRIVET_DEBUG_TRACE_DUMP_RESULT_ENTRY_PARAMS_KEY_CALL_API_ID),
          .value = uw_value_int(entry->u.call.api_id)};
      param_values[param_count++] = (UwMapValue){
          .key = uw_value_int(
              PRIVET_DEBUG_TRACE_DUMP_RESULT_ENTRY_PARAMS_KEY_CALL_STATUS),
          .value = uw_value_int(entry->u.call.status)};
      break;
    }
    case kUwTraceTypeCommandExecute: {
      param_values[param_count++] = (UwMapValue){
          .key = uw_value_int(
              PRIVET_DEBUG_TRACE_DUMP_RESULT_ENTRY_PARAMS_KEY_COMMAND_EXECUTE_TRAIT),
          .value = uw_value_int(entry->u.command_execute.trait)};
      param_values[param_count++] = (UwMapValue){
          .key = uw_value_int(
              PRIVET_DEBUG_TRACE_DUMP_RESULT_ENTRY_PARAMS_KEY_COMMAND_EXECUTE_NAME),
          .value = uw_value_int(entry->u.command_execute.name)};
      break;
    }
    case kUwTraceTypeSession: {
      param_values[param_count++] = (UwMapValue){
          .key = uw_value_int(
              PRIVET_DEBUG_TRACE_DUMP_RESULT_ENTRY_PARAMS_KEY_SESSION_TYPE),
          .value = uw_value_int(entry->u.session.type)};
      param_values[param_count++] = (UwMapValue){
          .key = uw_value_int(
              PRIVET_DEBUG_TRACE_DUMP_RESULT_ENTRY_PARAMS_KEY_SESSION_STATUS),
          .value = uw_value_int(entry->u.session.status)};
      break;
    }
    default: { break; }
  };

  UwMapValue values[4];
  size_t value_count = 0;

  values[value_count++] = (UwMapValue){
      .key = uw_value_int(PRIVET_DEBUG_TRACE_DUMP_RESULT_ENTRY_KEY_TYPE),
      .value = uw_value_int(entry->type)};
  values[value_count++] = (UwMapValue){
      .key = uw_value_int(PRIVET_DEBUG_TRACE_DUMP_RESULT_ENTRY_KEY_TIMESTAMP),
      .value = uw_value_int64(entry->timestamp)};
  values[value_count++] = (UwMapValue){
      .key = uw_value_int(PRIVET_DEBUG_TRACE_DUMP_RESULT_ENTRY_KEY_ID),
      .value = uw_value_int64(entry->id)};

  if (value_count > 0) {
    values[value_count++] = (UwMapValue){
        .key = uw_value_int(PRIVET_DEBUG_TRACE_DUMP_RESULT_ENTRY_KEY_PARAMS),
        .value = uw_value_map(param_values, param_count)};
  }

  UwValue map_value = uw_value_map(values, value_count);

  return uw_value_callback_array_append(context, &map_value);
}

UwStatus uw_trace_log_encode_to_privet_request_(
    UwTraceLog* trace_log,
    size_t start,
    size_t end,
    UwPrivetRequest* privet_request) {
  TraceEncodingContext encoding_context = find_entries_(trace_log, start, end);

  UwMapValue dump_result[] = {
      {.key = uw_value_int(PRIVET_DEBUG_TRACE_DUMP_RESULT_KEY_DUMP),
       .value = uw_value_callback_array(&entry_encoding_callback_,
                                        (void*)&encoding_context,
                                        encoding_context.length)},
  };
  UwMapValue result_value[] = {
      {.key = uw_value_int(PRIVET_DEBUG_RESPONSE_KEY_TRACE_DUMP_RESULT),
       .value =
           uw_value_map(dump_result, uw_value_map_count(sizeof(dump_result)))},
  };

  UwMapValue command_results[] = {
      {.key = uw_value_int(PRIVET_COMMAND_OBJ_KEY_STATE),
       .value = uw_value_int(PRIVET_COMMAND_OBJ_VALUE_STATE_DONE)},
      {.key = uw_value_int(PRIVET_COMMAND_OBJ_KEY_RESULT),
       .value = uw_value_map(result_value,
                             uw_value_map_count(sizeof(result_value)))}};

  UwValue command_map = uw_value_map(
      command_results, uw_value_map_count(sizeof(command_results)));

  return uw_privet_request_reply_privet_ok_(privet_request, &command_map);
}

UwTraceEntry* uw_trace_log_append_(UwDevice* device, UwTraceType type) {
  UwTraceLog* log = &device->trace_log;
  UwTraceEntry* entry = &log->entries[log->next_index];
  log->next_index = (log->next_index + 1) % UW_TRACE_LOG_ENTRY_COUNT;
  *entry = (UwTraceEntry){
      .id = log->next_id++,
      .timestamp = uw_time_get_timestamp_seconds_(),
      .type = type,
  };
  return entry;
}
