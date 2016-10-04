// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/value_scan.h"

#include "src/buffer.h"
#include "src/log.h"

static const char kParseError[] = "Decoding Error\n";
static const char kAdvanceError[] = "Advance Error\n";

UwStatus uw_value_scan_decode_simple_value_(CborValue* cbor_value,
                                            UwValueType expected_type,
                                            UwValue* value) {
  if (!cbor_value_is_valid(cbor_value)) {
    return UW_STATUS_AND_LOG_DEBUG(kUwStatusValueInvalidInput,
                                   "Invalid cbor value\n");
  }

  if (!cbor_value_is_length_known(cbor_value)) {
    return UW_STATUS_AND_LOG_DEBUG(kUwStatusValueInvalidInput,
                                   "Chunked CBOR values unsupported\n");
  }

  CborType cbor_type = cbor_value_get_type(cbor_value);

  if (expected_type == kUwValueTypeBinaryCbor ||
      expected_type == kUwValueTypeUTF8String ||
      expected_type == kUwValueTypeByteString ||
      (expected_type == kUwValueTypeUnknown &&
       (cbor_type == CborByteStringType || cbor_type == CborTextStringType))) {
    const uint8_t* current_ptr;
    const uint8_t* next_ptr;

    // Extracting the buffer for an BinaryCbor includes the cbor type prefix.
    // When we extract string types, we do not want the type prefix, and hence
    // we decode it by extracting the length and the subtracting it from the
    // head of the next value.
    if (expected_type == kUwValueTypeBinaryCbor) {
      current_ptr = cbor_value->ptr;
      if (cbor_value_advance(cbor_value) != CborNoError) {
        return UW_STATUS_AND_LOG_DEBUG(kUwStatusValueInvalidInput,
                                       kAdvanceError);
      }
      next_ptr = cbor_value->ptr;
    } else {
      if (!(cbor_value_is_byte_string(cbor_value) ||
            cbor_value_is_text_string(cbor_value))) {
        return UW_STATUS_AND_LOG_DEBUG(
            kUwStatusValueTypeMismatch,
            "Unable to extract non-string into string value\n");
      }

      size_t length = 0;
      if (cbor_value_get_string_length(cbor_value, &length)) {
        return UW_STATUS_AND_LOG_DEBUG(
            kUwStatusValueInvalidInput,
            "Unable to extract value string length\n");
      }
      if (cbor_value_advance(cbor_value) != CborNoError) {
        return UW_STATUS_AND_LOG_DEBUG(kUwStatusValueInvalidInput,
                                       kAdvanceError);
      }
      next_ptr = cbor_value->ptr;
      current_ptr = next_ptr - length;
    }

    value->value.binary_cbor_value = current_ptr;
    value->length = next_ptr - current_ptr;
    // Assume a string buffer if unspecified.
    if (expected_type != kUwValueTypeUnknown) {
      value->type = expected_type;
    } else {
      switch (cbor_type) {
        case CborTextStringType:
          value->type = kUwValueTypeUTF8String;
          break;
        case CborByteStringType:
          value->type = kUwValueTypeByteString;
          break;
        default:
          return UW_STATUS_AND_LOG_DEBUG(kUwStatusValueTypeUnsupported,
                                         "Unexpected cbor type\n");
      }
    }
    return kUwStatusSuccess;
  }

  switch (cbor_type) {
    case CborIntegerType: {
      if (expected_type == kUwValueTypeInt ||
          expected_type == kUwValueTypeUnknown) {
        if (cbor_value_get_int(cbor_value, &value->value.int_value)) {
          return UW_STATUS_AND_LOG_DEBUG(kUwStatusValueInvalidInput,
                                         kParseError);
        }
        value->type = kUwValueTypeInt;
        if (cbor_value_advance(cbor_value) != CborNoError) {
          return UW_STATUS_AND_LOG_DEBUG(kUwStatusValueInvalidInput,
                                         kAdvanceError);
        }
        return kUwStatusSuccess;
      } else if (expected_type == kUwValueTypeInt64) {
        if (cbor_value_get_int64(cbor_value, &value->value.int64_value)) {
          return UW_STATUS_AND_LOG_DEBUG(kUwStatusValueInvalidInput,
                                         kParseError);
        }
        value->type = kUwValueTypeInt64;
        if (cbor_value_advance(cbor_value) != CborNoError) {
          return UW_STATUS_AND_LOG_DEBUG(kUwStatusValueInvalidInput,
                                         kAdvanceError);
        }
        return kUwStatusSuccess;
      }
      break;
    }
    case CborFloatType: {
      if (expected_type == kUwValueTypeFloat32 ||
          expected_type == kUwValueTypeUnknown) {
        if (cbor_value_get_float(cbor_value, &value->value.float_value)) {
          return UW_STATUS_AND_LOG_DEBUG(kUwStatusValueInvalidInput,
                                         kParseError);
        }
        value->type = kUwValueTypeFloat32;
        if (cbor_value_advance(cbor_value) != CborNoError) {
          return UW_STATUS_AND_LOG_DEBUG(kUwStatusValueInvalidInput,
                                         kAdvanceError);
        }
        return kUwStatusSuccess;
      }
      break;
    }
    case CborDoubleType: {
      if (expected_type == kUwValueTypeFloat64 ||
          expected_type == kUwValueTypeUnknown) {
        if (cbor_value_get_double(cbor_value, &value->value.double_value)) {
          return UW_STATUS_AND_LOG_DEBUG(kUwStatusValueInvalidInput,
                                         kParseError);
        }
        value->type = kUwValueTypeFloat64;
        if (cbor_value_advance(cbor_value) != CborNoError) {
          return UW_STATUS_AND_LOG_DEBUG(kUwStatusValueInvalidInput,
                                         kAdvanceError);
        }
        return kUwStatusSuccess;
      }
      break;
    }
    case CborBooleanType: {
      if (expected_type == kUwValueTypeBool ||
          expected_type == kUwValueTypeUnknown) {
        if (cbor_value_get_boolean(cbor_value, &value->value.bool_value)) {
          return UW_STATUS_AND_LOG_DEBUG(kUwStatusValueInvalidInput,
                                         kParseError);
        }
        value->type = kUwValueTypeBool;
        if (cbor_value_advance(cbor_value) != CborNoError) {
          return UW_STATUS_AND_LOG_DEBUG(kUwStatusValueInvalidInput,
                                         kAdvanceError);
        }
        return kUwStatusSuccess;
      }
      break;
    }
    case CborNullType: {
      if (expected_type == kUwValueTypeNull ||
          expected_type == kUwValueTypeUnknown) {
        value->type = kUwValueTypeNull;
        if (cbor_value_advance(cbor_value) != CborNoError) {
          return UW_STATUS_AND_LOG_DEBUG(kUwStatusValueInvalidInput,
                                         kAdvanceError);
        }
        return kUwStatusSuccess;
      }
      break;
    }
    case CborUndefinedType: {
      if (expected_type == kUwValueTypeUndefined ||
          expected_type == kUwValueTypeUnknown) {
        value->type = kUwValueTypeUndefined;
        if (cbor_value_advance(cbor_value) != CborNoError) {
          return UW_STATUS_AND_LOG_DEBUG(kUwStatusValueInvalidInput,
                                         kAdvanceError);
        }
        return kUwStatusSuccess;
      }
      break;
    }
    case CborByteStringType:
    case CborTextStringType: {
      // This case would be handled by the if block before this switch statement
      // of the types match.  Here we break out to hit the default type mismatch
      // case.
      break;
    }
    default: {
      return UW_STATUS_AND_LOG_WARN(kUwStatusValueInvalidInput,
                                    "Non-simple type encountered: %d\n",
                                    cbor_type);
    }
  }

  return UW_STATUS_AND_LOG_WARN(kUwStatusValueTypeMismatch,
                                "Expected uw_type: %d cbor_type: %d\n",
                                expected_type, cbor_type);
}

static UwStatus decode_key_value_(CborValue* iter,
                                  const UwMapFormat format[],
                                  size_t count) {
  UwValue key = {};

  UwStatus key_status =
      uw_value_scan_decode_simple_value_(iter, kUwValueTypeUnknown, &key);
  if (!uw_status_is_success(key_status)) {
    return UW_STATUS_AND_LOG_WARN(kUwStatusValueInvalidInput,
                                  "Refusing to parse complex format key: %d\n",
                                  key_status);
  }

  for (int i = 0; i < count; ++i) {
    if (!uw_value_equals(&key, &format[i].key)) {
      continue;
    }
    if (!uw_value_is_undefined(format[i].value)) {
      return UW_STATUS_AND_LOG_WARN(kUwStatusValueRepeatedMapKey,
                                    "Parsed value already defined.\n");
    }
    return uw_value_scan_decode_simple_value_(iter, format[i].type,
                                              format[i].value);
  }

  if (!cbor_value_is_valid(iter) || (cbor_value_advance(iter) != CborNoError)) {
    return kUwStatusValueInvalidInput;
  }

  return kUwStatusSuccess;
}

UwStatus uw_value_scan_map(const UwBuffer* buffer,
                           const UwMapFormat format[],
                           size_t count) {
  if (uw_buffer_is_null(buffer)) {
    return kUwStatusSuccess;
  }

  const uint8_t* bytes = NULL;
  size_t length = 0;
  uw_buffer_get_const_bytes(buffer, &bytes, &length);

  CborParser parser = {};
  CborValue root = {};
  CborError error = cbor_parser_init(bytes, length, 0, &parser, &root);
  if (error != CborNoError) {
    UW_LOG_WARN("Error initializing parser: %i\n", error);
    return kUwStatusValueInvalidInput;
  }

  if (!cbor_value_is_valid(&root) || !cbor_value_is_map(&root)) {
    UW_LOG_WARN("Expecting parameter value to be a map\n");
    return kUwStatusValueInvalidInput;
  }

  CborValue iter = {};
  if (cbor_value_enter_container(&root, &iter) != CborNoError) {
    UW_LOG_WARN("Failed to enter container\n");
    return kUwStatusValueInvalidInput;
  }

  while (!cbor_value_at_end(&iter)) {
    UwStatus decode_status = decode_key_value_(&iter, format, count);
    if (!uw_status_is_success(decode_status)) {
      return decode_status;
    }
  }
  return kUwStatusSuccess;
}

UwStatus uw_value_scan_map_with_value(const UwValue* binary_cbor_value,
                                      const UwMapFormat format[],
                                      size_t count) {
  if (binary_cbor_value->type != kUwValueTypeBinaryCbor) {
    UW_LOG_WARN("Expected binary_cbor_value to be an binary_cbor\n");
    return kUwStatusValueTypeMismatch;
  }

  UwBuffer buffer;
  uint8_t* buffer_start = (uint8_t*)binary_cbor_value->value.binary_cbor_value;
  uw_buffer_init(&buffer, buffer_start, binary_cbor_value->length);
  if (buffer_start != NULL) {
    uw_buffer_set_length_(&buffer, binary_cbor_value->length);
  }

  return uw_value_scan_map(&buffer, format, count);
}

UwStatus uw_value_array_iterator_init(UwValueArrayIterator* array_iter,
                                      const UwValue* binary_cbor_value) {
  if (binary_cbor_value->type != kUwValueTypeBinaryCbor) {
    return kUwStatusValueTypeMismatch;
  }

  *array_iter = (UwValueArrayIterator){};

  CborError error = cbor_parser_init(binary_cbor_value->value.binary_cbor_value,
                                     binary_cbor_value->length, 0,
                                     &array_iter->parser, &array_iter->root);

  if (error != CborNoError) {
    return kUwStatusInvalidInput;
  }

  if (!cbor_value_is_valid(&array_iter->root) ||
      !cbor_value_is_array(&array_iter->root)) {
    UW_LOG_WARN("Expecting parameter value to be a array\n");
    return kUwStatusValueInvalidInput;
  }

  error = cbor_value_get_array_length(&array_iter->root, &array_iter->size);
  if (error != CborNoError) {
    return kUwStatusInvalidInput;
  }

  error = cbor_value_enter_container(&array_iter->root, &array_iter->iter);
  if (error != CborNoError) {
    return kUwStatusInvalidInput;
  }

  return kUwStatusSuccess;
}

UwStatus uw_value_array_iterator_get_next(UwValueArrayIterator* array_iter,
                                          UwValueType type,
                                          UwValue* value) {
  return uw_value_scan_decode_simple_value_(&array_iter->iter, type, value);
}

UwStatus uw_value_scan_map_lookup_(const UwValue* binary_cbor_value,
                                   const UwValue key,
                                   UwValueType expected_type,
                                   UwValue* value) {
  UwMapFormat singleton = {.key = key, .type = expected_type, .value = value};
  return uw_value_scan_map_with_value(binary_cbor_value, &singleton, 1);
}
