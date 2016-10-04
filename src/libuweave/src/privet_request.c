// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/privet_request.h"

#include "src/log.h"
#include "src/privet_defines.h"
#include "tinycbor/src/cbor.h"
#include "uweave/status.h"
#include "uweave/value_scan.h"

void uw_privet_request_init_(UwPrivetRequest* privet_request,
                             UwBuffer* request_buffer,
                             UwBuffer* reply_buffer,
                             UwSession* session) {
  memset(privet_request, 0, sizeof(UwPrivetRequest));

  privet_request->request_buffer = request_buffer;
  privet_request->reply_buffer = reply_buffer;
  // TODO(jmccullough): interrupt + connect/disconnect safety.
  privet_request->state = kUwPrivetRequestStateIdle;
  privet_request->session = session;
  privet_request->parse_called = false;
  privet_request->has_reply = false;
  privet_request->api_id = kUwPrivetRequestApiIdUnknown;
  uw_buffer_init(&privet_request->param_buffer, NULL, 0);

  // Ensure the reply-buffer is empty.
  // TODO(jmccullough): Ensure this makes sense in the transport refactor.
  uw_buffer_reset(privet_request->reply_buffer);
}

void uw_privet_request_set_state_(UwPrivetRequest* privet_request,
                                  UwPrivetRequestState state) {
  privet_request->state = state;
}

UwPrivetRequestState uw_privet_request_get_state_(
    UwPrivetRequest* privet_request) {
  return privet_request->state;
}

UwSession* uw_privet_request_get_session_(UwPrivetRequest* privet_request) {
  return privet_request->session;
}

UwPrivetRequestApiId uw_privet_request_get_api_id_(
    UwPrivetRequest* privet_request) {
  return privet_request->api_id;
}

UwPrivetRequestApiId uw_privet_request_get_request_id_(
    UwPrivetRequest* privet_request) {
  return privet_request->request_id;
}

UwBuffer* uw_privet_request_get_param_buffer_(UwPrivetRequest* privet_request) {
  return &privet_request->param_buffer;
}

bool uw_privet_request_parse_(UwPrivetRequest* privet_request) {
  if (privet_request->parse_called) {
    UW_LOG_WARN("uw_privet_request_parse_ called more than once\n");
    return false;
  }

  privet_request->parse_called = true;

  privet_request->has_request_id = false;
  uw_buffer_init(&privet_request->param_buffer, NULL, 0);

  UwValue version = uw_value_undefined();
  UwValue api_id = uw_value_undefined();
  UwValue request_id = uw_value_undefined();
  UwValue params = uw_value_undefined();

  UwMapFormat format[] = {
      {.key = uw_value_int(PRIVET_RPC_KEY_VERSION),
       .type = kUwValueTypeInt,
       .value = &version},
      {.key = uw_value_int(PRIVET_RPC_KEY_API_ID),
       .type = kUwValueTypeInt,
       .value = &api_id},
      {.key = uw_value_int(PRIVET_RPC_KEY_REQUEST_ID),
       .type = kUwValueTypeInt,
       .value = &request_id},
      {.key = uw_value_int(PRIVET_RPC_KEY_PARAMS),
       .type = kUwValueTypeBinaryCbor,
       .value = &params},
  };

  UwStatus scan_status =
      uw_value_scan_map(privet_request->request_buffer, format,
                        uw_value_scan_map_count(sizeof(format)));
  if (!uw_status_is_success(scan_status)) {
    UW_LOG_WARN("Error parsing privet_request: %d\n", scan_status);
    return false;
  }

  if (!uw_value_is_undefined(&version)) {
    privet_request->privet_rpc_version = version.value.int_value;
  } else {
    privet_request->privet_rpc_version = PRIVET_RPC_VALUE_VERSION;
  }

  if (!uw_value_is_undefined(&request_id)) {
    privet_request->request_id = request_id.value.int_value;
    privet_request->has_request_id = true;
  }

  if (!uw_value_is_undefined(&api_id)) {
    privet_request->api_id = api_id.value.int_value;
  } else {
    UW_LOG_WARN("Privet message missing required: api_id\n");
    return false;
  }

  if (!uw_value_is_undefined(&params)) {
    // Dropping the const from the buffer because uw_buffer only takes non-const
    // pointers.
    uint8_t* param_start = (uint8_t*)params.value.binary_cbor_value;
    uw_buffer_slice(privet_request->request_buffer, param_start, params.length,
                    &privet_request->param_buffer);
  }

  return true;
}

UwStatus uw_privet_request_reply_(UwPrivetRequest* privet_request,
                                  bool is_success,
                                  const UwValue* payload) {
  if (!privet_request->parse_called) {
    UW_LOG_ERROR("Reply called before parse.\n");
    return kUwStatusPrivetParseError;
  }

  if (!privet_request->has_request_id) {
    UW_LOG_ERROR("Can't reply, request had no request id.\n");
    return kUwStatusPrivetParseError;
  }

  if (privet_request->has_reply) {
    UW_LOG_ERROR("Reply already set.\n");
    return kUwStatusPrivetParseError;
  }

  UwMapValue reply_pairs[] = {
      {.key = uw_value_int(PRIVET_RPC_KEY_REQUEST_ID),
       .value = uw_value_int(privet_request->request_id)},
      {.key = uw_value_int(is_success ? PRIVET_RPC_KEY_RESULT
                                      : PRIVET_RPC_KEY_ERROR),
       .value = *payload}};

  UwValue reply_value =
      uw_value_map(reply_pairs, uw_value_map_count(sizeof(reply_pairs)));

  CborEncoder encoder;
  uint8_t* bytes;
  size_t length;
  uw_buffer_get_bytes_(privet_request->reply_buffer, &bytes, &length);
  cbor_encoder_init(&encoder, bytes, length, 0);

  UwStatus encoding_status = uw_value_encode_value_(&encoder, &reply_value);
  privet_request->has_reply = uw_status_is_success(encoding_status);
  uw_buffer_set_length_(privet_request->reply_buffer, encoder.ptr - bytes);

  // Promote to a more specific error.
  if (encoding_status == kUwStatusValueEncodingOutOfSpace) {
    return kUwStatusPrivetResponseTooLarge;
  }
  return encoding_status;
}

UwStatus uw_privet_request_reply_privet_ok_(UwPrivetRequest* privet_request,
                                            const UwValue* data) {
  return uw_privet_request_reply_(privet_request,
                                  /* is_success */ true, data);
}

UwStatus uw_privet_request_reply_privet_error_(UwPrivetRequest* privet_request,
                                               UwStatus code,
                                               const char message[],
                                               const UwValue* data) {
  UwMapValue error_pairs[] = {
      {.key = uw_value_int(PRIVET_RPC_ERROR_KEY_CODE),
       .value = uw_value_int(code)},
      {},
      {}  // Empty slots save space for message and/or data values.
  };

  size_t pairs_count = 1;

  if (message != NULL) {
    error_pairs[pairs_count].key = uw_value_int(PRIVET_RPC_ERROR_KEY_MESSAGE);
    error_pairs[pairs_count].value = uw_value_utf8_string(message);
    pairs_count++;
  }

  if (data != NULL) {
    error_pairs[pairs_count].key = uw_value_int(PRIVET_RPC_ERROR_KEY_DATA);
    error_pairs[pairs_count].value = *data;
    pairs_count++;
  }

  UwValue error_value = uw_value_map(error_pairs, pairs_count);
  return uw_privet_request_reply_(privet_request,
                                  /* is_success */ false, &error_value);
}

bool uw_privet_request_has_reply_(UwPrivetRequest* privet_request) {
  return privet_request->has_reply;
}

UwStatus uw_privet_request_has_required_role_or_reply_error_(
    UwPrivetRequest* privet_request,
    UwRole role) {
  UwStatus role_status = uw_session_role_at_least(
      uw_privet_request_get_session_(privet_request), role);
  if (!uw_status_is_success(role_status)) {
    UW_LOG_ERROR("Permission denied(%d)\n", role_status);
    uw_privet_request_reply_privet_error_(privet_request, role_status,
                                          /* error message */ NULL,
                                          /* error data */ NULL);
  }
  return role_status;
}
