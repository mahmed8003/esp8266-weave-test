// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/value.h"

#include <string.h>

#include "src/log.h"
#include "src/privet_defines.h"

// This is the only error currently possible other than calling
// encode_simple_type with an invalid type, which is not done.
#define CBOR_AS_STATUS(value_type, cbor_error)                              \
  (UW_STATUS_AND_LOG_WARN(kUwStatusValueEncodingOutOfSpace,                 \
                          "Encoding failure: value_type=%d, cbor_err=%d\n", \
                          value_type, cbor_error))

struct UwValueCallbackMapContext_ {
  CborEncoder* map_encoder;
};

struct UwValueCallbackArrayContext_ {
  CborEncoder* array_encoder;
};

UwValue uw_value_int(int value) {
  return (UwValue){.type = kUwValueTypeInt, .value.int_value = value};
}

UwValue uw_value_int64(int64_t value) {
  return (UwValue){.type = kUwValueTypeInt64, .value.int64_value = value};
}

UwValue uw_value_byte_array(const uint8_t byte_array[], size_t len) {
  return (UwValue){.type = kUwValueTypeByteString,
                   .length = len,
                   .value.byte_string_value = byte_array};
}

UwValue uw_value_utf8_string_with_length(const char str[], size_t len) {
  return (UwValue){
      .type = kUwValueTypeUTF8String, .length = len, .value.string_value = str};
}

UwValue uw_value_array(const UwValue array_value[], size_t len) {
  return (UwValue){.type = kUwValueTypeArray,
                   .length = len,
                   .value.array_value = array_value};
}

UwValue uw_value_map(const UwMapValue map_value[], size_t len) {
  return (UwValue){
      .type = kUwValueTypeMap, .length = len, .value.map_value = map_value};
}

UwValue uw_value_float(float value) {
  return (UwValue){.type = kUwValueTypeFloat32, .value.float_value = value};
}

UwValue uw_value_double(double value) {
  return (UwValue){.type = kUwValueTypeFloat64, .value.double_value = value};
}

UwValue uw_value_bool(bool value) {
  return (UwValue){.type = kUwValueTypeBool, .value.bool_value = value};
}

UwValue uw_value_null() {
  return (UwValue){.type = kUwValueTypeNull};
}

UwValue uw_value_undefined() {
  return (UwValue){.type = kUwValueTypeUndefined};
}

UwValue uw_value_binary_cbor(const uint8_t* buffer, size_t len) {
  return (UwValue){.type = kUwValueTypeBinaryCbor,
                   .value.binary_cbor_value = buffer,
                   .length = len};
}

UwValue uw_value_callback_map(UwValueCallbackMap callback,
                              void* context,
                              size_t len) {
  return (UwValue){
      .type = kUwValueTypeCallbackMap,
      .value.callback_map = {.callback = callback, .context = context},
      .length = len};
}

UwValue uw_value_callback_array(UwValueCallbackArray callback,
                                void* context,
                                size_t len) {
  return (UwValue){
      .type = kUwValueTypeCallbackArray,
      .value.callback_array = {.callback = callback, .context = context},
      .length = len};
}

UwStatus uw_value_encode_value_(CborEncoder* encoder, const UwValue* item) {
  CborError err;
  switch (item->type) {
    case kUwValueTypeInt: {
      err = cbor_encode_int(encoder, item->value.int_value);
      if (err) {
        return CBOR_AS_STATUS(item->type, err);
      }
      break;
    }
    case kUwValueTypeInt64: {
      err = cbor_encode_int(encoder, item->value.int64_value);
      if (err) {
        return CBOR_AS_STATUS(item->type, err);
      }
      break;
    }
    case kUwValueTypeFloat32: {
      err = cbor_encode_floating_point(encoder, CborFloatType,
                                       &item->value.float_value);
      if (err) {
        return CBOR_AS_STATUS(item->type, err);
      }
      break;
    }
    case kUwValueTypeFloat64: {
      err = cbor_encode_floating_point(encoder, CborDoubleType,
                                       &item->value.double_value);
      if (err) {
        return CBOR_AS_STATUS(item->type, err);
      }
      break;
    }
    case kUwValueTypeBool: {
      err = cbor_encode_boolean(encoder, item->value.bool_value);
      if (err) {
        return CBOR_AS_STATUS(item->type, err);
      }
      break;
    }
    case kUwValueTypeNull: {
      err = cbor_encode_null(encoder);
      if (err) {
        return CBOR_AS_STATUS(item->type, err);
      }
      break;
    }
    case kUwValueTypeUndefined: {
      err = cbor_encode_undefined(encoder);
      if (err) {
        return CBOR_AS_STATUS(item->type, err);
      }
      break;
    }
    case kUwValueTypeByteString: {
      err = cbor_encode_byte_string(encoder, item->value.byte_string_value,
                                    item->length);
      if (err) {
        return CBOR_AS_STATUS(item->type, err);
      }
      break;
    }
    case kUwValueTypeUTF8String: {
      err = cbor_encode_text_string(encoder, item->value.string_value,
                                    item->length);
      if (err) {
        return CBOR_AS_STATUS(item->type, err);
      }
      break;
    }
    case kUwValueTypeArray: {
      size_t array_count = item->length;
      CborEncoder array_encoder;
      cbor_encoder_create_array(encoder, &array_encoder, array_count);
      for (int i = 0; i < array_count; ++i) {
        // Value only for the array.
        UwStatus item_status =
            uw_value_encode_value_(&array_encoder, &item->value.array_value[i]);
        if (!uw_status_is_success(item_status)) {
          return item_status;
        }
      }
      cbor_encoder_close_container(encoder, &array_encoder);
      break;
    }
    case kUwValueTypeMap: {
      size_t struct_count = item->length;
      CborEncoder struct_encoder;
      cbor_encoder_create_map(encoder, &struct_encoder, struct_count);
      for (int i = 0; i < struct_count; ++i) {
        UwStatus kv_status = uw_value_encode_map_value_(
            &struct_encoder, &item->value.map_value[i]);
        if (!uw_status_is_success(kv_status)) {
          return kv_status;
        }
      }
      cbor_encoder_close_container(encoder, &struct_encoder);
      break;
    }
    case kUwValueTypeBinaryCbor: {
      // TODO: Add to tinycbor.
      if (encoder->end - encoder->ptr <= item->length) {
        return kUwStatusValueEncodingOutOfSpace;
      }
      memcpy(encoder->ptr, item->value.binary_cbor_value, item->length);
      encoder->ptr += item->length;
      break;
    }
    case kUwValueTypeCallbackMap: {
      if (item->value.callback_map.callback == NULL) {
        return kUwStatusInvalidArgument;
      }

      CborEncoder map_encoder;
      cbor_encoder_create_map(encoder, &map_encoder, item->length);
      UwValueCallbackMapContext context = {.map_encoder = &map_encoder};
      for (int i = 0; i < item->length; ++i) {
        UwStatus result = item->value.callback_map.callback(
            &context, item->value.callback_map.context, i);
        if (!uw_status_is_success(result)) {
          return result;
        }
      }
      cbor_encoder_close_container(encoder, &map_encoder);
      break;
    }
    case kUwValueTypeCallbackArray: {
      if (item->value.callback_array.callback == NULL) {
        return kUwStatusInvalidArgument;
      }

      CborEncoder array_encoder;
      cbor_encoder_create_array(encoder, &array_encoder, item->length);
      UwValueCallbackArrayContext context = {.array_encoder = &array_encoder};
      for (int i = 0; i < item->length; ++i) {
        UwStatus result = item->value.callback_array.callback(
            &context, item->value.callback_array.context, i);
        if (!uw_status_is_success(result)) {
          return result;
        }
      }
      cbor_encoder_close_container(encoder, &array_encoder);
      break;
    }
    default: {
      return UW_STATUS_AND_LOG_WARN(
          kUwStatusValueEncodingTypeUnsupported,
          "Saw unexpected value type %d on encoding\n", item->type);
    }
  }
  return kUwStatusSuccess;
}

UwStatus uw_value_encode_value_to_buffer_(UwBuffer* buffer,
                                          const UwValue* item) {
  uint8_t* bytes;
  size_t size;
  uw_buffer_get_bytes_(buffer, &bytes, &size);
  CborEncoder encoder;
  cbor_encoder_init(&encoder, bytes, size, 0);
  UwStatus status = uw_value_encode_value_(&encoder, item);
  if (uw_status_is_success(status)) {
    uw_buffer_set_length_(buffer, encoder.ptr - bytes);
  }
  return status;
}

void uw_value_copy_string(const UwValue* source,
                          char* destination,
                          size_t destination_size) {
  int end = destination_size - 1;
  if (end > source->length) {
    end = source->length;
  }
  strncpy(destination, source->value.string_value, end);
  destination[end] = '\0';
}

UwStatus uw_value_encode_map_value_(CborEncoder* encoder,
                                    const UwMapValue* map_item) {
  UwStatus key_status = uw_value_encode_value_(encoder, &map_item->key);
  if (!uw_status_is_success(key_status)) {
    return key_status;
  }
  return uw_value_encode_value_(encoder, &map_item->value);
}

UwStatus uw_value_callback_map_append(
    UwValueCallbackMapContext* callback_context,
    UwMapValue* map_value) {
  return uw_value_encode_map_value_(callback_context->map_encoder, map_value);
}

UwStatus uw_value_callback_array_append(
    UwValueCallbackArrayContext* callback_context,
    UwValue* array_value) {
  return uw_value_encode_value_(callback_context->array_encoder, array_value);
}

/**
 * Returns true if rhs contains all elements of lhs.  Used to compute map
 * equality by doing a symmetric contains all without an actual hash map.  The
 * maps should not have duplicate values which may be enforced by the cbor
 * implementation.
 */
static bool uw_value_map_contains_all_(const UwValue* lhs, const UwValue* rhs) {
  if (lhs->length != rhs->length) {
    return false;
  }

  size_t count = lhs->length;
  for (int i = 0; i < count; ++i) {
    bool matched = false;
    for (int j = 0; j < count; ++j) {
      // Optimize for the case that the two structures are identically ordered.
      int offset_j = i + j;
      if (offset_j >= count) {
        offset_j -= count;
      }
      if (uw_value_equals(&lhs->value.map_value[i].key,
                          &rhs->value.map_value[offset_j].key)) {
        matched = uw_value_equals(&lhs->value.map_value[i].value,
                                  &rhs->value.map_value[offset_j].value);
        break;
      }
    }
    if (!matched) {
      return false;
    }
  }
  return true;
}

bool uw_value_equals(const UwValue* lhs, const UwValue* rhs) {
  if (lhs->type != rhs->type) {
    return false;
  }

  switch (lhs->type) {
    case kUwValueTypeNull: {
      return true;
    }
    case kUwValueTypeUndefined: {
      return true;
    }
    case kUwValueTypeInt: {
      return lhs->value.int_value == rhs->value.int_value;
    }
    case kUwValueTypeInt64: {
      return lhs->value.int64_value == rhs->value.int64_value;
    }
    case kUwValueTypeFloat32: {
      return lhs->value.float_value == rhs->value.float_value;
    }
    case kUwValueTypeFloat64: {
      return lhs->value.double_value == rhs->value.double_value;
    }
    case kUwValueTypeBool: {
      return lhs->value.bool_value == rhs->value.bool_value;
    }
    case kUwValueTypeByteString: {
      if (lhs->length != rhs->length) {
        return false;
      }
      return memcmp(lhs->value.byte_string_value, rhs->value.byte_string_value,
                    lhs->length) == 0;
    }
    case kUwValueTypeUTF8String: {
      if (lhs->length != rhs->length) {
        return false;
      }
      return strncmp(lhs->value.string_value, rhs->value.string_value,
                     lhs->length) == 0;
    }
    case kUwValueTypeArray: {
      if (lhs->length != rhs->length) {
        return false;
      }
      for (int i = 0; i < lhs->length; ++i) {
        if (!uw_value_equals(&lhs->value.array_value[i],
                             &rhs->value.array_value[i])) {
          return false;
        }
      }
      return true;
    }
    case kUwValueTypeMap: {
      return uw_value_map_contains_all_(rhs, lhs) &&
             uw_value_map_contains_all_(lhs, rhs);
    }
    case kUwValueTypeBinaryCbor: {
      return (rhs->length == lhs->length) &&
             (memcmp(rhs->value.binary_cbor_value, lhs->value.binary_cbor_value,
                     rhs->length) == 0);
    }
    default: {
      UW_LOG_WARN("comparing unknown value type %d\n", lhs->type);
      return false;
    }
  }
}
