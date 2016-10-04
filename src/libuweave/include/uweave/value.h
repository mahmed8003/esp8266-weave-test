// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBUWEAVE_INCLUDE_UWEAVE_VALUE_H_
#define LIBUWEAVE_INCLUDE_UWEAVE_VALUE_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "uweave/status.h"
#include "uweave/value_traits.h"

/*
 * This file defines a structured value encoding to pass uWeave for translation
 * to the internal CBOR representation.
 *
 * A stand-alone has a type and a value:
 * UwValue foo = uw_value_int(5);
 *
 * The contents of an array are represented as a c-array with a terminating
 * type:
 * UwValue foo_array_content[] = {uw_value_int(1), uw_value_int(2)};
 * UwValue foo_array = uw_value_array(foo_array_content, 2);
 *
 * We provide helpers for computing the length of arrays:
 * UwValue foo_array =
 *     uw_value_array(foo_array_content,
 *                    uw_value_array_count(sizeof(foo_array_content)));
 *
 * The contents of a map follow the same pattern as an array, but also include a
 * key:
 * UwMapValue foo_map_content[] =
 *   {{ .key = uw_value_int_value(1), .value = uw_value_int_value(9) },
 *    { .key = uw_value_int_value(2), .value = uw_value_int_value(10) }};
 * UwValue foo_map = uw_value_map(foo_map_content,
 *                                uw_value_map_count(sizeof(foo_map_content)));
 *
 * The base array can be passed to functions expecting the contents of either.
 * To build a nested map or array, use the definition in a nested structure:
 * UwMapValue foo_nested_map_content[] =
 *   {{ .key = uw_value_int_value(1),
 *      .value =
 *          uw_value_array(foo_array_content,
 *                         uw_value_array_count(sizeof(foo_array_content))) },
 *    { .key = uw_value_int_value(1),
 *      .value = uw_value_map(foo_map_content,
 *                            uw_value_map_count(sizeof(foo_map_content))) }};
 * UwValue foo_nested_map =
 *     uw_value_map(foo_nested_map_content,
 *                  uw_value_map_count(sizeof(foo_nested_map_content)));
 *
 * WARNING: Cyclic maps or arrays will result in loops, stack exhaustion, or
 * otherwise undefined behavior.
 *
 * See tests/value_test.c for more examples.
 */

typedef enum {
  kUwValueTypeUnknown,
  kUwValueTypeInt,
  kUwValueTypeInt64,
  kUwValueTypeByteString,
  kUwValueTypeUTF8String,
  kUwValueTypeArray,
  kUwValueTypeMap,
  kUwValueTypeFloat32,
  kUwValueTypeFloat64,
  kUwValueTypeBool,
  kUwValueTypeNull,
  kUwValueTypeUndefined,
  // Special type to hold binary-cbor data.  Will flat-copy the value on
  // serialization or point to the whole single-value cbor (int, map, etc) on
  // parsing.
  kUwValueTypeBinaryCbor,
  // Map-Value callback for dynamic construction.
  kUwValueTypeCallbackMap,
  // Array-Value callback for dynamic construction.
  kUwValueTypeCallbackArray,
} UwValueType;

struct UwMapValue_;

typedef struct UwValueCallbackMapContext_ UwValueCallbackMapContext;

/**
 * Callback for use in incremental map-value construction.  With uw_value_map,
 * the entire map must be structurally configured at compile time to be encoded.
 * The callback approach allows dynamically sized maps and can lower the memory
 * overhead of statically building the UwMapValue structure.
 *
 * The CallbackMap value is configured as:
 *  UwValue callback_map =
 *      uw_value_callback_map(&your_callback_func, data, length);
 *
 * The callback will be invoked with the indexes 0 to length - 1 with the
 * context data and you can encode the appropriate data:
 *  UwStatus your_callback_func(UwValueCallbackMapContext* context,
 *                              const void* data,
 *                              size_t index) {
 *    const MyStruct* foo = (const MyStruct*)context_data;

 *    UwMapValue map_value = {.key = uw_value_int(foo[index].key),
 *                            .value = uw_value_int(foo[index].value)};

 *    return uw_value_callback_map_append(callback_context, &map_value);
 *  }
 */
typedef UwStatus (*UwValueCallbackMap)(UwValueCallbackMapContext* context,
                                       const void* data,
                                       size_t index);

typedef struct UwValueCallbackArrayContext_ UwValueCallbackArrayContext;

/**
 * Callback for use in incremental array-value construction.  Matches the
 * callback-map in purpose and design.
 *
 * The CallbackArray value is configured as:
 *  UwValue callback_array =
 *      uw_value_callback_array(&your_callback_func, data, length);
 *
 * The callback will be invoked with the indexes 0 to length - 1 with the
 * context data and you can encode the appropriate data:
 *  UwStatus your_callback_func(UwValueCallbackArrayContext* context,
 *                              const void* data,
 *                              size_t index) {
 *    const MyStruct* foo = (const MyStruct*)context_data;

 *    UwValue array_value = uw_value_int(foo[index].value);

 *    return uw_value_callback_array_append(callback_context, &value);
 *  }
 */
typedef UwStatus (*UwValueCallbackArray)(UwValueCallbackArrayContext* context,
                                         const void* data,
                                         size_t index);

typedef struct UwValue_ {
  UwValueType type;
  size_t length;
  union {
    bool bool_value;
    int int_value;
    int64_t int64_value;
    float float_value;
    double double_value;
    const uint8_t* byte_string_value;
    const char* string_value;
    const struct UwMapValue_* map_value;
    const struct UwValue_* array_value;
    const void* binary_cbor_value;
    struct {
      UwValueCallbackMap callback;
      void* context;
    } callback_map;
    struct {
      UwValueCallbackArray callback;
      void* context;
    } callback_array;
  } value;
} UwValue;

typedef struct UwMapValue_ {
  UwValue key;
  UwValue value;
} UwMapValue;

UwValue uw_value_int(int value);

UwValue uw_value_int64(int64_t value);

UwValue uw_value_byte_array(const uint8_t byte_array[], size_t len);

UwValue uw_value_utf8_string_with_length(const char str[], size_t len);

static inline UwValue uw_value_utf8_string(const char str[]) {
  return uw_value_utf8_string_with_length(str, strlen(str));
}

UwValue uw_value_array(const UwValue array_value[], size_t len);

UwValue uw_value_map(const UwMapValue map_value[], size_t len);

UwValue uw_value_float(float value);

UwValue uw_value_double(double value);

UwValue uw_value_bool(bool value);

UwValue uw_value_null();

UwValue uw_value_undefined();

UwValue uw_value_binary_cbor(const uint8_t* buffer, size_t len);

UwValue uw_value_callback_map(UwValueCallbackMap callback,
                              void* context,
                              size_t len);

UwValue uw_value_callback_array(UwValueCallbackArray callback,
                                void* context,
                                size_t len);

/**
 * Computes the number of elements in a dynamically sized UwValue
 * array based on the sizeof result.
 */
static inline size_t uw_value_array_count(const size_t value_array_len) {
  return value_array_len / sizeof(UwValue);
}

/**
 * Computes the number of elements in a dynamically sized UwMapValue
 * array based on the sizeof result.
 */
static inline size_t uw_value_map_count(const size_t map_array_len) {
  return map_array_len / sizeof(UwMapValue);
}

/**
 * Returns true if the two values are equivalent. Does not support
 * callback based UwValues.
 */
bool uw_value_equals(const UwValue* lhs, const UwValue* rhs);

/** Returns true if the value is undefined. */
static inline bool uw_value_is_undefined(const UwValue* value) {
  return value->type == kUwValueTypeUndefined;
}

/**
 * Copies the source UwValue into the destination character array.  UwValue must
 * hold a string value.  If the destination buffer is smaller then the source
 * string, then the source will be truncated.  In all cases, the destination
 * will be null terminated.
 */
void uw_value_copy_string(const UwValue* source,
                          char* destination,
                          size_t destination_size);

/**
 * Encodes the given UwMapValue for a uw_value_callback_map invocation.  Must
 * be called exactly as many times as the length parameter (once per callback
 * invocation).
 */
UwStatus uw_value_callback_map_append(
    UwValueCallbackMapContext* callback_context,
    UwMapValue* map_value);

/**
 * Encodes the given UwValue for a uw_value_callback_array invocation.  Must
 * be called exactly as many times as the length parameter (once per callback
 * invocation).
 */
UwStatus uw_value_callback_array_append(
    UwValueCallbackArrayContext* callback_context,
    UwValue* value);

#endif  // LIBUWEAVE_INCLUDE_UWEAVE_VALUE_H_
