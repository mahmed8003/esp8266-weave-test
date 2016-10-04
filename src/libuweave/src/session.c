// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/session.h"

#include "src/buffer.h"
#include "src/counters.h"
#include "src/device.h"
#include "src/time.h"
#include "uweave/status.h"

#include <string.h>
#include <time.h>

#define CHANNEL_VERBOSE_NONE 0
#define CHANNEL_VERBOSE_PLAINTEXT 1
#define CHANNEL_VERBOSE_ENCRYPTED 2

#ifndef NDEBUG
#ifndef CHANNEL_VERBOSE
#define CHANNEL_VERBOSE CHANNEL_VERBOSE_PLAINTEXT
#endif
#else
#define CHANNEL_VERBOSE CHANNEL_VERBOSE_NONE
#endif

void uw_session_init_(UwSession* session, UwDevice* device) {
  *session = (UwSession){.device = device};
}

bool uw_session_is_valid_(UwSession* session) {
  return session->valid;
}

bool uw_session_is_authenticated_(UwSession* session) {
  return session->role != kUwRoleUnspecified;
}

void uw_session_set_role_(UwSession* session, UwRole role) {
  session->role = role;
}

UwDevice* uw_session_get_device_(UwSession* session) {
  return session->device;
}

bool uw_session_handshake_exchange_(UwSession* session,
                                    UwBuffer* request,
                                    UwBuffer* reply) {
  if (!session->valid) {
    UW_ASSERT(false, "Attempt to handshake on invalid session.\n");
    return false;
  }

#if CHANNEL_VERBOSE > CHANNEL_VERBOSE_PLAINTEXT
  uw_buffer_dump_for_debug_(request, "Connection request data");
#endif
  if (!uw_channel_encryption_init_(&session->crypto_state, request, reply)) {
    UW_ASSERT(false, "Encryption handshake failed.\n");
    uw_trace_session(session->device, kUwTraceSessionHandshake,
                     kUwStatusInvalidArgument);
    uw_device_increment_uw_counter_(session->device,
                                    kUwInternalCounterSessionHandshakeFailure);
    return false;
  }
#if CHANNEL_VERBOSE > CHANNEL_VERBOSE_PLAINTEXT
  uw_buffer_dump_for_debug_(reply, "Connection confirmation data");
#endif

  return true;
}

UwStatus uw_session_message_exchange_(UwSession* session,
                                      UwBuffer* request,
                                      UwBuffer* reply) {
  if (!session->valid) {
    UW_LOG_ERROR("Attempt to exchange message on invalid session.\n");
    return kUwStatusInvalidArgument;
  }

#if CHANNEL_VERBOSE > CHANNEL_VERBOSE_PLAINTEXT
  uw_buffer_dump_for_debug_(request, "Incoming message before decryption");
#endif

  UwStatus in_status = uw_channel_encryption_process_in_(
      &session->crypto_state, &session->device->device_crypto, request, reply);
  if (!uw_status_is_success(in_status)) {
    UW_LOG_ERROR("Encryption layer failed to handle incoming message.\n");
    uw_device_increment_uw_counter_(session->device,
                                    kUwInternalCounterSessionDecryptionFailure);
    return uw_trace_session(session->device, kUwTraceSessionProcessIn,
                            in_status);
  }

  if (uw_buffer_get_length(reply) > 0) {
    // The encryption layer is doing its own signaling (e.g. handshake)
#if CHANNEL_VERBOSE > CHANNEL_VERBOSE_PLAINTEXT
    uw_buffer_dump_for_debug_(reply, "Outgoing handshake response");
#endif
    return kUwStatusSuccess;
  }

#if CHANNEL_VERBOSE > CHANNEL_VERBOSE_NONE
  uw_buffer_dump_for_debug_(request, "Incoming message after decryption");
#endif

  // Dispatch the decrypted message to the device
  UwStatus dispatch_status =
      uw_device_message_exchange_(session->device, session, request, reply);
  if (!uw_status_is_success(dispatch_status)) {
    UW_LOG_ERROR("Device could not handle message.\n");
    return uw_trace_session(session->device, kUwTraceSessionDispatch,
                            dispatch_status);
  }

#if CHANNEL_VERBOSE > CHANNEL_VERBOSE_NONE
  uw_buffer_dump_for_debug_(reply, "Outgoing message before encryption");
#endif

  // Encrypt the outgoing message
  UwStatus out_status =
      uw_channel_encryption_process_out_(&session->crypto_state, reply);
  if (!uw_status_is_success(out_status)) {
    UW_LOG_ERROR("Encryption layer failed to handle outgoing message.\n");
    uw_device_increment_uw_counter_(session->device,
                                    kUwInternalCounterSessionEncryptionFailure);
    return uw_trace_session(session->device, kUwTraceSessionProcessOut,
                            out_status);
  }
#if CHANNEL_VERBOSE > CHANNEL_VERBOSE_PLAINTEXT
  uw_buffer_dump_for_debug_(reply, "Outgoing message after encryption");
#endif

  return dispatch_status;
}

void uw_session_invalidate_(UwSession* session) {
  *session = (UwSession){.device = session->device, .role = kUwRoleUnspecified};
}

void uw_session_start_valid_(UwSession* session) {
  *session = (UwSession){
      .device = session->device, .valid = true, .role = kUwRoleUnspecified};
}

UwStatus uw_session_role_at_least(UwSession* session, UwRole minimum_role) {
  if (session->valid) {
    if (minimum_role == kUwRoleUnspecified) {
      return kUwStatusSuccess;
    }

    if (session->role == kUwRoleUnspecified) {
      return kUwStatusAuthenticationRequired;
    }

    return (minimum_role >= session->role) ? kUwStatusSuccess
                                           : kUwStatusInsufficientRole;
  }

  return kUwStatusAuthenticationRequired;
}

bool uw_session_is_access_control_authorized(UwSession* session) {
  return session->valid && session->access_control_authorized &&
         uw_status_is_success(uw_session_role_at_least(session, kUwRoleOwner));
}

void uw_session_set_access_control_authorized(UwSession* session, bool value) {
  if (session->valid) {
    session->access_control_authorized = value;
  }
}

UwStatus uw_session_check_expiration_(UwSession* session) {
  // Session must be valid to check expiration.
  if (!uw_session_is_valid_(session)) {
    return kUwStatusAuthenticationRequired;
  }

  // Allow the session if the expiration time is not set.
  if (session->expiration_time == 0) {
    return kUwStatusSuccess;
  }

  time_t current_time = uw_time_get_timestamp_seconds_();
  if (current_time > session->expiration_time) {
    return uw_trace_session(session->device, kUwTraceSessionExpiration,
                            kUwStatusSessionExpired);
  }

  return kUwStatusSuccess;
}
