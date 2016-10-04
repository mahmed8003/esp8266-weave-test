// Copyright 2016 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBUWEAVE_SRC_ACCESS_CONTROL_REQUEST_H_
#define LIBUWEAVE_SRC_ACCESS_CONTROL_REQUEST_H_

#include "src/device.h"
#include "src/privet_request.h"
#include "uweave/status.h"

UwStatus uw_access_control_request_claim_(UwDevice* device,
                                          UwPrivetRequest* privet_request);

UwStatus uw_access_control_request_confirm_(UwDevice* device,
                                            UwPrivetRequest* privet_request);

#endif  // LIBUWEAVE_SRC_ACCESS_CONTROL_REQUEST_H_
