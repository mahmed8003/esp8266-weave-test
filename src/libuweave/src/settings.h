// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBUWEAVE_SRC_SETTINGS_H_
#define LIBUWEAVE_SRC_SETTINGS_H_

#include "uweave/settings.h"

#include <stddef.h>
#include <stdint.h>

#include "uweave/status.h"

/**
 * Returns the model manifest ID based on the settings.
 */
void uw_settings_get_model_manifest_id(const UwSettings* settings,
                                       char model_manifest_id[],
                                       size_t model_manifest_id_len);

/**
 * Reads settings from the storage provider.  If the read fails, or if values
 * are not present, the original values in the settings struct are maintained.
 */
UwStatus uw_settings_read_from_storage_(UwSettings* settings);

/**
 * Writes settings to the storage provider.
 */
UwStatus uw_settings_write_to_storage_(UwSettings* settings);

#endif  // LIBUWEAVE_SRC_SETTINGS_H_
