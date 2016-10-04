// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBUWEAVE_SRC_VALUE_H_
#define LIBUWEAVE_SRC_VALUE_H_

#include <stdbool.h>
#include <stdint.h>

#include "src/buffer.h"
#include "tinycbor/src/cbor.h"
#include "uweave/status.h"
#include "uweave/value.h"

UwStatus uw_value_encode_value_(CborEncoder* encoder, const UwValue* item);

UwStatus uw_value_encode_value_to_buffer_(UwBuffer* encoder,
                                          const UwValue* item);

UwStatus uw_value_encode_map_value_(CborEncoder* encoder,
                                    const UwMapValue* map_item);

#endif  // LIBUWEAVE_SRC_VALUE_H_
