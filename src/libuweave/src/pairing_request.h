// Copyright 2016 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBUWEAVE_SRC_PAIRING_REQUEST_H_
#define LIBUWEAVE_SRC_PAIRING_REQUEST_H_

#include "src/device_crypto.h"
#include "src/privet_request.h"

UwStatus uw_pairing_start_reply_(UwPrivetRequest* privet_request);
UwStatus uw_pairing_confirm_reply_(UwPrivetRequest* privet_request,
                                   UwDeviceCrypto* device_crypto);

#endif  // LIBUWEAVE_SRC_PAIRING_REQUEST_H_
