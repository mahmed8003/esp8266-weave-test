// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBUWEAVE_SRC_EXECUTE_REQUEST_H_
#define LIBUWEAVE_SRC_EXECUTE_REQUEST_H_

#include "src/privet_request.h"

typedef struct {
  UwPrivetRequest* privet_request;
  bool parse_called;
  uint32_t trait;
  uint32_t name;
  UwBuffer param_buffer;
  CborParser param_parser;
  CborValue param_value;
} UwExecuteRequest;

UwStatus uw_execute_request_init_(UwExecuteRequest* execute_request,
                                  UwPrivetRequest* privet_request);

#endif  // LIBUWEAVE_SRC_EXECUTE_REQUEST_H_
