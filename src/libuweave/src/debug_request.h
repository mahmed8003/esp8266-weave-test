// Copyright 2016 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBUWEAVE_SRC_DEBUG_REQUEST_H_
#define LIBUWEAVE_SRC_DEBUG_REQUEST_H_

#include "src/device.h"
#include "src/execute_request.h"
#include "src/privet_request.h"
#include "uweave/status.h"

UwStatus uw_debug_command_request_(UwDevice* device,
                                   UwExecuteRequest* execute_request);

#endif  // LIBUWEAVE_SRC_DEBUG_REQUEST_H_
