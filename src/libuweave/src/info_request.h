// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBUWEAVE_SRC_INFO_REPLY_H_
#define LIBUWEAVE_SRC_INFO_REPLY_H_

#include "src/privet_request.h"
#include "uweave/buffer.h"
#include "uweave/settings.h"
#include "uweave/value.h"

bool uw_info_request_set_info_(UwPrivetRequest* privet_request,
                               const UwDevice* device);

#endif  // LIBUWEAVE_SRC_INFO_REPLY_H_
