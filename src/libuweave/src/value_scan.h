// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBUWEAVE_SRC_VALUE_SCAN_H_
#define LIBUWEAVE_SRC_VALUE_SCAN_H_

#include "tinycbor/src/cbor.h"
#include "uweave/status.h"
#include "uweave/value_scan.h"

/** Translates a cbor value to a UwValue of the expected type. */
UwStatus uw_value_scan_decode_simple_value_(CborValue* cbor_value,
                                            UwValueType expected_type,
                                            UwValue* value);

/** Retrieves a map key value with a linear lookup scan. */
UwStatus uw_value_scan_map_lookup_(const UwValue* binary_cbor_value,
                                   const UwValue key,
                                   UwValueType expected_type,
                                   UwValue* value);

#endif  // LIBUWEAVE_SRC_VALUE_SCAN_H_
