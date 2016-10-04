// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBUWEAVE_INCLUDE_UWEAVE_SETTINGS_H_
#define LIBUWEAVE_INCLUDE_UWEAVE_SETTINGS_H_

#include "uweave/config.h"
#include "uweave/embedded_code.h"
#include "uweave/pairing_callback.h"
#include "uweave/pairing_type.h"

#include <stdbool.h>
#include <stdint.h>

/**
 * Structure used to hold device-specific settings.
 */
typedef struct UwSettings_ {
  // Devices own the definition of these settings, uWeave won't change them.
  const char* firmware_version;
  const char* oem_name;
  const char* model_name;
  // The model ID and device class comprise the model manifest ID, and they
  // should be printable characters.
  char model_id[3];
  char device_class[2];
  // Bitfield of the UwPairingType enum values that the device supports for
  // pairing.
  uint8_t supported_pairing_types;
  // Pairing callbacks are called when the pairing process begins and ends. For
  // example, if one of the supported pairing types is kUwPairingTypePinCode,
  // populate this struct to handle displaying and hiding the pairing PIN.
  UwPairingCallback pairing_callback;
  // If one of the supported pairing types is kUwPairingTypeEmbeddedCode,
  // populate this struct with the code itself or a callback to get the code.
  UwEmbeddedCode embedded_code;
  bool supports_wifi_24ghz;
  bool supports_wifi_50ghz;
  bool supports_ble_40;

  // Whether the user should be able to re-pair the device without a reset
  // trigger.
  bool enable_multipairing;
  // Persisted by uWeave.  The application sets the default value on startup.
  // The device overrides the value with the stored value, if present.  Changes
  // to the name via the /setup API updates the struct and persists the name.
  char name[UW_SETTINGS_MAX_NAME_LENGTH];
} UwSettings;

#endif  // LIBUWEAVE_INCLUDE_UWEAVE_SETTINGS_H_
