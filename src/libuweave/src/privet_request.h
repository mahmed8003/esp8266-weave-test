// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBUWEAVE_SRC_PRIVET_REQUEST_H_
#define LIBUWEAVE_SRC_PRIVET_REQUEST_H_

#include "src/buffer.h"
#include "src/session.h"
#include "src/value.h"

#include "uweave/status.h"

typedef enum {
  kUwPrivetRequestStateIdle,     // This request is not currently in use.
  kUwPrivetRequestStateRequest,  // This request is accepting request data.
  kUwPrivetRequestStateQueue,    // All request data is in, awaiting dispatch.
  kUwPrivetRequestStateExecute,  // This request is executing.
  kUwPrivetRequestStateReply     // This request is replying.
} UwPrivetRequestState;

typedef struct UwPrivetRequest_ {
  UwPrivetRequestState state;
  UwSession* session;
  UwBuffer* request_buffer;
  UwBuffer* reply_buffer;
  bool parse_called;
  bool has_reply;
  uint32_t privet_rpc_version;
  uint32_t api_id;
  bool has_request_id;
  uint32_t request_id;
  UwBuffer param_buffer;
} UwPrivetRequest;

typedef enum {
  kUwPrivetRequestApiIdUnknown = -1,
  kUwPrivetRequestApiIdInfo = 0,
  kUwPrivetRequestApiIdPairingStart = 2,
  kUwPrivetRequestApiIdPairingConfirm = 3,
  kUwPrivetRequestApiIdAuth = 5,
  kUwPrivetRequestApiIdState = 6,
  kUwPrivetRequestApiIdExecute = 8,
  kUwPrivetRequestApiIdSetup = 9,
  kUwPrivetRequestApiIdAccessControlClaim = 24,
  kUwPrivetRequestApiIdAccessControlConfirm = 25,
  kUwPrivetRequestApiIdDebug = 29,
} UwPrivetRequestApiId;

void uw_privet_request_init_(UwPrivetRequest* privet_request,
                             UwBuffer* request_buffer,
                             UwBuffer* reply_buffer,
                             UwSession* session);

/**
 * Sets the request to a new state.  See UwPrivetRequestState for details.
 */
void uw_privet_request_set_state_(UwPrivetRequest* privet_request,
                                  UwPrivetRequestState state);

/**
 * Gets the request's current state.
 */
UwPrivetRequestState uw_privet_request_get_state_(
    UwPrivetRequest* privet_request);

UwSession* uw_privet_request_get_session_(UwPrivetRequest* privet_request);

bool uw_privet_request_parse_(UwPrivetRequest* privet_request);

UwPrivetRequestApiId uw_privet_request_get_api_id_(
    UwPrivetRequest* privet_request);

UwPrivetRequestApiId uw_privet_request_get_request_id_(
    UwPrivetRequest* privet_request);

UwBuffer* uw_privet_request_get_param_buffer_(UwPrivetRequest* privet_request);

UwStatus uw_privet_request_reply_privet_ok_(UwPrivetRequest* privet_request,
                                            const UwValue* value);
UwStatus uw_privet_request_reply_privet_error_(UwPrivetRequest* privet_request,
                                               UwStatus code,
                                               const char message[],
                                               const UwValue* data);
bool uw_privet_request_has_reply_(UwPrivetRequest* privet_request);

/**
 * Ensures the current connection has the required role (or higher) or
 * adds an error to the response.  Returns kUwStatusSuccess if the connection
 * met the required access level.
 */
UwStatus uw_privet_request_has_required_role_or_reply_error_(
    UwPrivetRequest* privet_request,
    UwRole role);

/**
 * Returns true if the request's session is encrypted.
 */
static inline bool uw_privet_request_is_secure(
    UwPrivetRequest* privet_request) {
  return uw_session_is_secure(privet_request->session);
}

#endif  // LIBUWEAVE_SRC_PRIVET_REQUEST_H_
