// Copyright 2016 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBUWEAVE_INCLUDE_UWEAVE_PAIRING_TYPE_H_
#define LIBUWEAVE_INCLUDE_UWEAVE_PAIRING_TYPE_H_

/**
 * The type of pairing modes the device supports. Enum values must be powers of
 * 2 to fit in the supported_pairing_types bitfield in UwSettings.
 */
typedef enum {
  kUwPairingTypeNone = 0,
  // During PIN code pairing, uWeave will generate a random 4-digit numeric
  // PIN, 0000-9999, and it is the provider's job to display the PIN between the
  // time that uwp_pairing_begin() and uwp_pairing_end() is called.
  kUwPairingTypePinCode = (1 << 0),
  // The device will come packaged with an embedded code printed on a sticker,
  // piece of paper in the box, hidden on the device itself, etc. uWeave will
  // use the embedded_code field of the UwSettings struct.
  kUwPairingTypeEmbeddedCode = (1 << 1)
} UwPairingType;

#endif  // LIBUWEAVE_INCLUDE_UWEAVE_PAIRING_TYPE_H_
