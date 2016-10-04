// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBUWEAVE_INCLUDE_UWEAVE_UWEAVE_H_
#define LIBUWEAVE_INCLUDE_UWEAVE_UWEAVE_H_

/**
 * @fileoverview This header is intended to included by third party code that
 *     wants to get the entire public Weave API (minus the provider headers) in
 *     a single include.  It should not be included by first-party uWeave code.
 */

#include "uweave/base.h"
#include "uweave/ble_transport.h"
#include "uweave/buffer.h"
#include "uweave/command.h"
#include "uweave/command_list.h"
#include "uweave/config.h"
#include "uweave/device.h"
#include "uweave/gatt.h"
#include "uweave/pairing_type.h"
#include "uweave/session.h"
#include "uweave/settings.h"
#include "uweave/state_reply.h"
#include "uweave/status.h"
#include "uweave/value.h"
#include "uweave/value_scan.h"
#include "uweave/value_traits.h"

#endif  // LIBUWEAVE_INCLUDE_UWEAVE_UWEAVE_H_
