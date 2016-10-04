// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBUWEAVE_SRC_SESSION_H_
#define LIBUWEAVE_SRC_SESSION_H_

#include "uweave/session.h"

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#include "src/channel_encryption.h"
#include "src/crypto_spake.h"
#include "src/uw_assert.h"
#include "uweave/device.h"
#include "uweave/status.h"

/**
 * Tracks a given connection/session's state for authentication and validity.
 */
// TODO(jmccullough): Interrupt safety for racing disconnect/connect and /auth
// calls.
struct UwSession_ {
  // The device that the session is associated with.
  UwDevice* device;
  // Whether this session is connected/valid.
  bool valid;
  // Whether the client called /auth with a /pairing token and is authorized to
  // invoke /accessControl.
  bool access_control_authorized;
  // The currently authenticated role.
  UwRole role;
  // The SPAKE state the device uses during /pairing/start and /pairing/confirm
  // to generate the shared key.
  UwSpakeState server_spake_state;
  // The commitment that is in response to /pairing/start.
  uint8_t device_commitment_buf[UW_SPAKE_P224_POINT_SIZE];
  // The session ID established during pairing.
  uint32_t pairing_session_id;
  // Time when the session expires as seconds from the unix epoch.
  time_t expiration_time;
  // The encryption layer state
  UwChannelEncryptionState crypto_state;
};

void uw_session_init_(UwSession* session, UwDevice* device);

/** Returns true if the session is alive and valid. */
bool uw_session_is_valid_(UwSession* session);
/** Returns true if the session has successfully authenticated. */
bool uw_session_is_authenticated_(UwSession* session);

/**
 * Conducts the initial handshake message exchange.
 *
 * Handshake offer is given in the `request` buffer, response should be appended
 * to the `reply` buffer.  The reply must be no longer than 13 bytes.
 */
bool uw_session_handshake_exchange_(UwSession* session,
                                    UwBuffer* request,
                                    UwBuffer* reply);

/**
 * Conducts a normal message exchange.
 *
 * Inbound message is given in the `request` buffer, response should be appended
 * to the `reply` buffer.  There are no size restrictions on the reply other
 * than the available space in the reply buffer.
 */
UwStatus uw_session_message_exchange_(UwSession* session,
                                      UwBuffer* request,
                                      UwBuffer* reply);

/** Clears a session on client disconnect or timeout. */
void uw_session_invalidate_(UwSession* session);
/** Marks a new session as valid. */
void uw_session_start_valid_(UwSession* session);

/** Sets the authenticated role of the session. */
void uw_session_set_role_(UwSession* session, UwRole role);

/** Gets the device associated with this session. */
UwDevice* uw_session_get_device_(UwSession* session);

/**
 * Returns true if the client has gone through a /pairing -> /auth sequence and
 * is authorized to call /accessControl/claim.
 */
bool uw_session_is_access_control_authorized(UwSession* session);

/**
 * Sets whether the client has gone through a /pairing -> /auth sequence.
 */
void uw_session_set_access_control_authorized(UwSession* session, bool value);

/**
 * Returns true if the session is encrypted.
 */
static inline bool uw_session_is_secure(UwSession* session) {
  return uw_channel_encryption_is_encrypted_(&session->crypto_state);
}

/**
 * Checks whether the session has expired yet. Returns kUwStatusSessionExpired
 * if the session has expired and kUwStatusSuccess if the session is still OK.
 */
UwStatus uw_session_check_expiration_(UwSession* session);

#endif  // LIBUWEAVE_SRC_SESSION_H_
