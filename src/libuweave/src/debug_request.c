// Copyright 2016 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/debug_request.h"

#include "src/counters.h"
#include "src/device.h"
#include "src/privet_defines.h"
#include "src/time.h"
#include "src/value_scan.h"
#include "uweave/value.h"

static UwStatus encode_command_result_(void* data, const UwValue* results) {
  UwMapValue command_results[] = {
      {.key = uw_value_int(PRIVET_COMMAND_OBJ_KEY_STATE),
       .value = uw_value_int(PRIVET_COMMAND_OBJ_VALUE_STATE_DONE)},
      {.key = uw_value_int(PRIVET_COMMAND_OBJ_KEY_RESULT), .value = *results}};

  UwValue command_map = uw_value_map(
      command_results, uw_value_map_count(sizeof(command_results)));

  return uw_privet_request_reply_privet_ok_((UwPrivetRequest*)data,
                                            &command_map);
}

static UwStatus encode_metric_result_(void* data, const UwValue* results) {
  UwMapValue metric_results[] = {
      {.key = uw_value_int(PRIVET_DEBUG_RESPONSE_KEY_METRICS),
       .value = *results}};
  UwValue results_map =
      uw_value_map(metric_results, uw_value_map_count(sizeof(metric_results)));

  return encode_command_result_(data, &results_map);
}

static UwStatus encode_metrics_(UwDevice* device,
                                UwExecuteRequest* execute_request) {
  return uw_counter_set_encode_(device->counter_set, &encode_metric_result_,
                                (void*)execute_request->privet_request);
}

static UwStatus encode_trace_bounds_(UwDevice* device,
                                     UwExecuteRequest* execute_request) {
  size_t min_id;
  size_t max_id;
  uw_trace_log_get_range_(&device->trace_log, &min_id, &max_id);

  UwMapValue bounds[] = {
      {.key = uw_value_int(PRIVET_DEBUG_QUERY_RESULT_KEY_FIRST),
       .value = uw_value_int(min_id)},
      {.key = uw_value_int(PRIVET_DEBUG_QUERY_RESULT_KEY_LAST),
       .value = uw_value_int(max_id)},
  };

  UwMapValue query_response[] = {
      {.key = uw_value_int(PRIVET_DEBUG_RESPONSE_KEY_TRACE_QUERY_RESULT),
       .value = uw_value_map(bounds, uw_value_map_count(sizeof(bounds)))},
  };

  UwValue query_map =
      uw_value_map(query_response, uw_value_map_count(sizeof(query_response)));

  return encode_command_result_((void*)execute_request->privet_request,
                                &query_map);
}

/**
 * Extracts the dump parameters from the debug request.  The debug requests are
 * structured to be flexible and simple to decode.  The request level params
 * are encoded as:
 *   {
 *     dump_params: { begin: x, end: y }
 *   }
 */
static UwStatus encode_trace_dump_(UwDevice* device,
                                   UwExecuteRequest* execute_request) {
  uint8_t* buf;
  size_t buf_len;
  uw_buffer_get_bytes_(&execute_request->param_buffer, &buf, &buf_len);
  UwValue param_value = uw_value_binary_cbor(buf, buf_len);

  // Grab the parameter sub-map.
  UwValue dump_params = uw_value_undefined();
  UwStatus scan_status = uw_value_scan_map_lookup_(
      &param_value, uw_value_int(PRIVET_DEBUG_KEY_TRACE_DUMP_PARAMETERS),
      kUwValueTypeBinaryCbor, &dump_params);

  if (!uw_status_is_success(scan_status)) {
    return scan_status;
  }

  if (uw_value_is_undefined(&dump_params)) {
    return kUwStatusInvalidArgument;
  }

  // Extract the start and end from the parameter sub-map.
  UwValue start = uw_value_undefined();
  UwValue end = uw_value_undefined();

  UwMapFormat format[] = {
      {.key = uw_value_int(PRIVET_DEBUG_TRACE_DUMP_KEY_START),
       .type = kUwValueTypeInt,
       .value = &start},
      {.key = uw_value_int(PRIVET_DEBUG_TRACE_DUMP_KEY_END),
       .type = kUwValueTypeInt,
       .value = &end},
  };

  UwStatus param_scan_status = uw_value_scan_map_with_value(
      &dump_params, format, uw_value_scan_map_count(sizeof(format)));

  if (!uw_status_is_success(param_scan_status)) {
    return param_scan_status;
  }

  if (uw_value_is_undefined(&start) || uw_value_is_undefined(&end)) {
    return kUwStatusInvalidArgument;
  }

  return uw_trace_log_encode_to_privet_request_(
      &device->trace_log, start.value.int_value, end.value.int_value,
      execute_request->privet_request);
}

UwStatus uw_debug_command_request_(UwDevice* device,
                                   UwExecuteRequest* execute_request) {
  switch (execute_request->name) {
    case PRIVET_DEBUG_NAME_METRICS: {
      return encode_metrics_(device, execute_request);
    }
    case PRIVET_DEBUG_NAME_TRACE_QUERY: {
      return encode_trace_bounds_(device, execute_request);
    }
    case PRIVET_DEBUG_NAME_TRACE_DUMP: {
      return encode_trace_dump_(device, execute_request);
    }
    default: { break; }
  }
  return kUwStatusInvalidArgument;
}
