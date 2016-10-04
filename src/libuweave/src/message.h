// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBUWEAVE_SRC_MESSAGE_H_
#define LIBUWEAVE_SRC_MESSAGE_H_

#include "src/packet_header.h"

#define UW_MESSAGE_CONN_REQUEST_MAX_DATA 13
#define UW_MESSAGE_CONN_CONFIRM_MAX_DATA 15

/** States used for both in and out messages. */
typedef enum {
  kUwMessageStateError = 0,
  kUwMessageStateEmpty,
  kUwMessageStateBusy,
  kUwMessageStateComplete,
} UwMessageState;

typedef enum {
  kUwMessageTypeUnknown = 0,
  kUwMessageTypeConnectionRequest,
  kUwMessageTypeConnectionConfirm,
  kUwMessageTypeError,
  kUwMessageTypeData
} UwMessageType;

UwPacketHeaderCmd uw_message_type_to_header_cmd_(UwMessageType type);

#endif  // LIBUWEAVE_SRC_MESSAGE_H_
