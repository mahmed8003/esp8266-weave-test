// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBUWEAVE_SRC_STATE_REQUEST_H_
#define LIBUWEAVE_SRC_STATE_REQUEST_H_

#include "src/privet_request.h"
#include "uweave/buffer.h"
#include "uweave/state_reply.h"

struct UwStateReply_ {
  UwPrivetRequest* privet_request;
};

void uw_state_reply_init_(UwStateReply* state_reply,
                          UwPrivetRequest* privet_request);

#endif  // LIBUWEAVE_SRC_STATE_REQUEST_H_
