// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBUWEAVE_INCLUDE_UWEAVE_VALUE_SCAN_H_
#define LIBUWEAVE_INCLUDE_UWEAVE_VALUE_SCAN_H_

#include "tinycbor/src/cbor.h"
#include "uweave/buffer.h"
#include "uweave/status.h"
#include "uweave/value.h"

/**
* Provides the mechanism to scan the cbor parameters at a single level of a
* cbor input that might take the form of {KEY_1: v1, KEY_2: v2}:
*
*  UwValue param1 = uw_value_undefined();
*  UwValue param2 = uw_value_undefined();
*
*  UwMapFormat format[] = {
*      {.key = uw_value_int(KEY_1),
*       .type = kUwValueTypeInt,
*       .value = &param1},
*      {.key = uw_value_int(KEY_2),
*       .type = kUwValueTypeInt,
*       .value = &param2},
*  };
*
*  if (!uw_status_is_success(uw_value_scan_map(
*      buffer, format, uw_value_scan_map_count(sizeof(format))))) {
*    uw_execute_request_reply_with_error(...);
*    return true;
*  }
*
*  if (!uw_value_is_undefined(&param1)) {
*    // use param1
*  }
*
*
*  To parse a nested map, extract it as an input buffer and parse recursively.
*  For example, to parse the map {OUTER_KEY: { KEY_1: v1, KEY_2: v2}}, use
*  a pattern similar to:
*
*  UwValue nested_param_buffer = uw_value_undefined();
*
*  UwMapFormat outer_format[] = {
*    {.key = uw_value_int(OUTER_KEY),
*     .type = kUwValueTypeBinaryCbor,
*     .value = &nested_param_buffer},
*  };
*
*  if (!uw_value_scan_map(buffer, outer_format,
*                         uw_value_scan_map_count(sizeof(outer_format)))) {
*    uw_execute_request_reply_with_error(...);
*    return true;
*  }
*
*  UwValue nested_param1 = uw_value_undefined();
*  UwValue nested_param2 = uw_value_undefined();
*
*  UwMapFormat nested_format[] = {
*      {.key = uw_value_int(KEY_1),
*       .type = kUwValueTypeInt,
*       .value = &nested_param1},
*      {.key = uw_value_int(KEY_2),
*       .type = kUwValueTypeInt,
*       .value = &nested_param2},
*  };
*
* if (!uw_value_is_undefined(&nested_param_buffer)) {
*   if (!uw_value_scan_map_with_value(
*           &nested_param_buffer, nested_format,
*           uw_value_scan_map_count(sizeof(nested_format)))) {
*     uw_execute_request_reply_with_error(...);
*     return true;
*   }
* }
* // At this point, if we parsed {OUTER_KEY: { KEY_1: 5, KEY_2: 6}},
* // Then nested_param1.value.int_value == 5
* // and nested_param2.value.int_value == 6.
*/

typedef struct {
  UwValue key;
  UwValueType type;
  UwValue* value;
} UwMapFormat;

UwStatus uw_value_scan_map(const UwBuffer* buffer,
                           const UwMapFormat format[],
                           size_t count);

UwStatus uw_value_scan_map_with_value(const UwValue* binary_cbor_value,
                                      const UwMapFormat format[],
                                      size_t count);

static inline size_t uw_value_scan_map_count(size_t format_bytes) {
  return format_bytes / sizeof(UwMapFormat);
}

/**
 * The array iterator provides a mechanism to extract individual array elements
 * from an encoded array.  For example:
 *   UwValueArrayIterator array_iter;
 *   if (!uw_status_is_success(
 *       uw_value_array_iterator_init(&array_iter, &binary_value))) {
 *     return err;
 *  }
 *
 *  size_t size = uw_value_array_iterator_size(&array_iter);
 *
 *  for (size_t i = 0; i < size; ++i) {
 *    UwValue value;
 *    if (!uw_status_is_success(
 *        uw_value_array_iterator_get_next(
 *            &array_iter, kUwValueTypeInt, &value))) {
 *      return err;
 *   }
 *   printf("%d\n", value.value.int_value);
 *  }
 */
typedef struct {
  CborParser parser;
  CborValue root;
  CborValue iter;
  size_t size;
  size_t index;
} UwValueArrayIterator;

/**
 * Initializes the array_iter with the binary_cbor_value.  Returns an error if
 * the binary value is malformed or invalid.
 */
UwStatus uw_value_array_iterator_init(UwValueArrayIterator* array_iter,
                                      const UwValue* binary_cbor_value);

/** Returns the length of the array. */
static inline size_t uw_value_array_iterator_size(
    const UwValueArrayIterator* array_iter) {
  return array_iter->size;
}

/**
 * Attempts to parse a value of the given type from the array.  Advances to the
 * next element on success.
 */
UwStatus uw_value_array_iterator_get_next(UwValueArrayIterator* array_iter,
                                          UwValueType type,
                                          UwValue* value);

#endif  // LIBUWEAVE_INCLUDE_UWEAVE_VALUE_SCAN_H_
