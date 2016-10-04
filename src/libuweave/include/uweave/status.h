// Copyright 2016 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBUWEAVE_INCLUDE_UWEAVE_STATUS_H_
#define LIBUWEAVE_INCLUDE_UWEAVE_STATUS_H_

#include <stdbool.h>

/**
 * Common status results for uWeave operations.
 *
 * These values are packed into uint16_t for embedded devices and should be kept
 * under 16384.
 */
typedef enum {
  // Generic status codes.
  kUwStatusSuccess = 0,
  kUwStatusNotFound = 1,
  kUwStatusInvalidInput = 2,
  kUwStatusTooLong = 3,
  kUwStatusInvalidArgument = 4,
  kUwStatusCommandNotFound = 5,
  // 6-9 Reserved for future generic use.

  // Device Crypto/Auth errors.
  kUwStatusDeviceCryptoNoKeys = 10,
  kUwStatusAuthenticationRequired = 11,
  kUwStatusAuthenticationFailed = 12,
  kUwStatusInsufficientRole = 13,
  kUwStatusPairingRequired = 14,
  kUwStatusVerificationFailed = 15,
  kUwStatusCryptoRandomNumberFailure = 16,
  kUwStatusSessionExpired = 17,
  kUwStatusCryptoIncomingMessageInvalid = 18,
  kUwStatusCryptoEncryptionFailed = 19,
  kUwStatusTimeRequired = 20,
  kUwStatusEncryptionRequired = 21,
  // 22-29 Reserved for future crypto/auth use.

  // Privet errors
  kUwStatusPrivetNotFound = 50,
  kUwStatusPrivetInvalidParam = 51,
  kUwStatusPrivetParseError = 52,
  kUwStatusPrivetResponseTooLarge = 53,
  // 54-60 Reserved for future Privet use.

  // Value encoding and decoding errors.
  kUwStatusValueInvalidInput = 100,
  kUwStatusValueRepeatedMapKey = 101,
  kUwStatusValueTypeMismatch = 102,
  kUwStatusValueTypeUnsupported = 103,
  kUwStatusValueEncodingTypeUnsupported = 104,
  kUwStatusValueEncodingOutOfSpace = 105,
  // 106-109 Reserved for future Value use.

  // Storage provider errors.
  kUwStatusStorageError = 110,
  kUwStatusStorageNotFound = 111,
  kUwStatusStorageBufferTooSmall = 112,
  kUwStatusStorageFileTooLarge = 113,
  kUwStatusStorageNoAvailableSpace = 114,
  kUwStatusStorageAlignmentError = 115,
  kUwStatusStorageVerifyError = 116,
  kUwStatusStorageNoWritableSpace = 117,
  // 118-129 Reserved for future storage use.

  // /commands/execute related errors.
  kUwStatusCommandNoAvailableBuffers = 130,
  // 131-139 Reserved for future command error use.

  // /pairing/{start,confirm} errors.
  kUwStatusPairingPinCodeTypeUnsupported = 140,
  kUwStatusPairingEmbeddedCodeTypeUnsupported = 141,
  kUwStatusPairingPinCodeGenerationFailed = 142,
  kUwStatusPairingEmbeddedCodeProviderFailed = 143,
  kUwStatusPairingEmbeddedCodeAppendFailed = 144,
  kUwStatusPairingResetRequired = 145,
  // 146-149 Reserved for future pairing error use.
} UwStatus;

static inline bool uw_status_is_success(UwStatus status) {
  return status == kUwStatusSuccess;
}

#endif  // LIBUWEAVE_INCLUDE_UWEAVE_STATUS_H_
