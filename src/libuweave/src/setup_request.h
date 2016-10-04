// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBUWEAVE_SRC_SETUP_REQUEST_H_
#define LIBUWEAVE_SRC_SETUP_REQUEST_H_

#include "uweave/setup_request.h"

#include "src/privet_request.h"
#include "uweave/settings.h"

#include <stdbool.h>

UwStatus uw_setup_request_(UwPrivetRequest* setup_request,
                           UwSettings* settings);

#endif  // LIBUWEAVE_SRC_SETUP_REQUEST_H_
