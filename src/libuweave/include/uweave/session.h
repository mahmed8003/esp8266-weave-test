// Copyright 2016 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBUWEAVE_INCLUDE_SESSION_H_
#define LIBUWEAVE_INCLUDE_SESSION_H_

#include "uweave/status.h"

typedef struct UwSession_ UwSession;

typedef enum {
  // We re-use the "auto" value used in the /auth request to specify
  // unauthenticated.
  kUwRoleUnspecified = 0,
  // Order is important in the roles below. Keep them sorted from
  // most-privileged (first) to least-privileged (last).
  kUwRoleOwner = 2,
  kUwRoleManager = 8,
  kUwRoleUser = 14,
  kUwRoleViewer = 20,
} UwRole;

/** Returns true if the role of the session is at least minimum_role. */
UwStatus uw_session_role_at_least(UwSession* session, UwRole minimum_role);

#endif  // LIBUWEAVE_INCLUDE_SESSION_H_
