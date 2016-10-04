// Copyright 2016 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBUWEAVE_SRC_COMMAND_H_
#define LIBUWEAVE_SRC_COMMAND_H_

#include "uweave/command.h"
#include "src/buffer.h"
#include "src/execute_request.h"

/**
 * The state of the command.
 *
 * Not intended to match the privet definition.
 * Ordered by decreasing eviction preference.
 */
typedef enum {
  kUwCommandStateEmpty,         // Empty, no status or results.
  kUwCommandStateDone,          // Synchronously completed.
  kUwCommandStateAsyncQueried,  // Asynchronously completed and result read.
  kUwCommandStateError,         // Completed in error.
  kUwCommandStateCancelled,     // Asynchronous command was cancelled.
  kUwCommandStateCompletedMarker = kUwCommandStateCancelled,
  // States below here will be preferentially kept unless an eviction is
  // required.
  kUwCommandStateAsyncDone,        // Asynchronously completed.
  kUwCommandStateCancelRequested,  // Asynchronous command should be abandoned,
                                   // if possible.
  kUwCommandStateAsyncInProgress,  // Asynchronously executing.
} UwCommandState;

struct UwCommand_ {
  // Copy of the request's trait_id.
  uint32_t trait_id;
  // Copy of the request's name_id.
  uint32_t name_id;
  // Unique identifier for this command, set by the CommandList.
  uint32_t command_id;
  // System-tick of assignment, set by the CommandList.
  uint32_t tick_stamp;
  // Pointer to the parser state of the command.  Not available once a command
  // has been deferred.
  UwExecuteRequest* execute_request;
  // Holds the command-level results reply.
  UwBuffer reply_buffer;
  // At the end for struct-packing purposes (16 bit enums).
  UwCommandState state;
};

void uw_command_init_(UwCommand* command, uint8_t* buffer, size_t buffer_len);

UwStatus uw_commannd_set_reply_buffer_(UwCommand* command,
                                       int state,
                                       const UwValue* result);

/**
 * Returns whether a command has be initialized and allocated by the
 * CommandList.
 */
static inline bool uw_command_is_initialized_(UwCommand* command) {
  return command->command_id > 0;
}

static inline void uw_command_reset_with_request_(
    UwCommand* command,
    UwExecuteRequest* execute_request) {
  command->trait_id = execute_request->trait;
  command->name_id = execute_request->name;
  command->execute_request = execute_request;

  uw_buffer_reset(&command->reply_buffer);
}

static inline void uw_command_mark_error_(UwCommand* command) {
  command->state = kUwCommandStateError;
}

static inline void uw_command_mark_done_(UwCommand* command) {
  command->state = kUwCommandStateDone;
}

static inline void uw_command_mark_deferred_(UwCommand* command) {
  command->execute_request = NULL;
  command->state = kUwCommandStateAsyncInProgress;
}

/**
 * Returns a UwValue that represent the provided value.  If the buffer is empty,
 * we return an empty map.
 */
static inline UwValue uw_command_reply_value_(UwCommand* command) {
  // The UwValue must not reference stack data.
  if (uw_buffer_get_length(&command->reply_buffer) > 0) {
    const uint8_t* bytes;
    size_t len;
    uw_buffer_get_const_bytes(&command->reply_buffer, &bytes, &len);
    return uw_value_binary_cbor(bytes, len);
  } else {
    return uw_value_map(NULL, 0);
  }
}

#endif  // LIBUWEAVE_SRC_COMMAND_H_
