// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBUWEAVE_SRC_AUTH_REQUEST_H
#define LIBUWEAVE_SRC_AUTH_REQUEST_H

#include "src/device.h"
#include "src/privet_request.h"
#include "uweave/status.h"

/**
 * Processes an /auth request and updates the results in the UwPrivetRequest's
 * session.
 */
UwStatus uw_auth_request_handler_(UwDevice* device,
                                  UwPrivetRequest* auth_request);

#endif  // LIBUWEAVE_SRC_AUTH_REQUEST_H
