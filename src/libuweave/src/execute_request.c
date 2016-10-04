// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string.h>

#include "src/execute_request.h"
#include "src/log.h"
#include "src/privet_defines.h"
#include "src/privet_request.h"
#include "uweave/status.h"
#include "uweave/value_scan.h"

static inline UwStatus parse_(UwExecuteRequest* execute_request) {
  if (execute_request->parse_called) {
    UW_LOG_WARN("uw_execute_request_parse_ called more than once\n");
    return kUwStatusInvalidArgument;
  }

  execute_request->parse_called = true;

  UwBuffer* privet_param_buffer =
      uw_privet_request_get_param_buffer_(execute_request->privet_request);
  if (uw_buffer_is_null(privet_param_buffer)) {
    return kUwStatusInvalidArgument;
  }

  UwValue trait = uw_value_undefined();
  UwValue name = uw_value_undefined();
  UwValue param = uw_value_undefined();

  UwMapFormat format[] = {
      {.key = uw_value_int(PRIVET_EXECUTE_KEY_TRAIT),
       .type = kUwValueTypeInt,
       .value = &trait},
      {.key = uw_value_int(PRIVET_EXECUTE_KEY_NAME),
       .type = kUwValueTypeInt,
       .value = &name},
      {.key = uw_value_int(PRIVET_EXECUTE_KEY_PARAM),
       .type = kUwValueTypeBinaryCbor,
       .value = &param},
  };

  UwStatus scan_status = uw_value_scan_map(
      privet_param_buffer, format, uw_value_scan_map_count(sizeof(format)));
  if (!uw_status_is_success(scan_status)) {
    UW_LOG_WARN("Error parsing parameters: %d\n", scan_status);
    return scan_status;
  }

  bool has_trait = !uw_value_is_undefined(&trait);
  bool has_name = !uw_value_is_undefined(&name);

  if (!(has_trait && has_name)) {
    UW_LOG_WARN("Privet execute param missing required:%s%s\n",
                (has_trait ? "" : " trait"), (has_name ? "" : " name"));
    return kUwStatusInvalidInput;
  }

  execute_request->trait = trait.value.int_value;
  execute_request->name = name.value.int_value;

  if (!uw_value_is_undefined(&param) && param.length > 0) {
    // Dropping the const from the buffer because uw_buffer only takes non-const
    // pointers.
    uint8_t* param_start = (uint8_t*)param.value.binary_cbor_value;
    uw_buffer_slice(privet_param_buffer, param_start, param.length,
                    &execute_request->param_buffer);

    if (cbor_parser_init(param_start, param.length, 0,
                         &execute_request->param_parser,
                         &execute_request->param_value) != CborNoError) {
      UW_LOG_ERROR("Unable to initialize parameter parser\n");
      return kUwStatusInvalidArgument;
    }
  }

  return kUwStatusSuccess;
}

UwStatus uw_execute_request_init_(UwExecuteRequest* execute_request,
                                  UwPrivetRequest* privet_request) {
  memset(execute_request, 0, sizeof(UwExecuteRequest));

  execute_request->parse_called = false;
  execute_request->privet_request = privet_request;
  execute_request->trait = 0;
  execute_request->name = 0;
  uw_buffer_init(&execute_request->param_buffer, NULL, 0);

  return parse_(execute_request);
}
