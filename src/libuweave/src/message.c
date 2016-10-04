// Copyright 2016 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/message.h"
#include "src/uw_assert.h"

UwPacketHeaderCmd uw_message_type_to_header_cmd_(UwMessageType type) {
  switch (type) {
    case kUwMessageTypeConnectionRequest:
      return kUwPacketHeaderCmdConnectionRequest;

    case kUwMessageTypeConnectionConfirm:
      return kUwPacketHeaderCmdConnectionConfirm;

    case kUwMessageTypeError:
      return kUwPacketHeaderCmdError;

    default:
      UW_ASSERT(false, "Unable to convert message type %i to a header "
                "command.\n", (int)type);
      return kUwPacketHeaderCmdError;
  }
}
