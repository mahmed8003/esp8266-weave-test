// Copyright 2016 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBUWEAVE_INCLUDE_UWEAVE_COMMAND_H_
#define LIBUWEAVE_INCLUDE_UWEAVE_COMMAND_H_

#include "uweave/buffer.h"
#include "uweave/session.h"
#include "uweave/status.h"
#include "uweave/value.h"

typedef struct UwCommand_ UwCommand;

/**
 * Gets the trait, which acts as a scope or namespace for `name`.
 */
uint32_t uw_command_get_trait(UwCommand* command);

/**
 * Gets the name of the command being executed.
 */
uint32_t uw_command_get_name(UwCommand* command);

/**
 * Gets a UwBuffer pointing to the raw request parameter bytes for this
 * request.
 */
UwBuffer* uw_command_get_param_buffer(UwCommand* command);

/**
 * Gets an integer parameter value from the request parameters.
 *
 * @param command The target command.
 * @param parameter_key The map-key of the parameter.
 * @param parameter_value Out-pointer to receive the value.
 *
 * @return true if the value was found and populated.
 */
bool uw_command_get_param_int(UwCommand* command,
                              int param_key,
                              int* param_value);

/**
 * Sends an immediate "done" reply for the command.
 *
 * If there is an error encoding the results, the error will be propagated.
 *
 * @param results A nullable results value to be returned in the request.
 *   If the results value is NULL, the uWeave reply will contain an explicit
 *   "results: {}" entry.
 */
UwStatus uw_command_reply_with_value(UwCommand* command, const UwValue* value);

/**
 * Sends an immediate "done" reply for the command without any parameters.
 */
UwStatus uw_command_reply_empty(UwCommand* command);

/**
 * Replies with an application level error of an arbitrary structure.
 */
UwStatus uw_command_reply_with_error(UwCommand* command, const UwValue* error);

/**
 * Replies with an application level error with a specific code/message.
 */
UwStatus uw_command_reply_with_error_code(UwCommand* command,
                                          int32_t error_code,
                                          const char* message);

/**
 * Ensures the current connection has the required role (or higher).  Returns
 * kUwStatusSuccess if the connection met the required access level.  If the
 * command handler returns a non-success error, it will be replied to as a
 * privet-level error.
 */
UwStatus uw_command_has_required_role(UwCommand* command, UwRole role);

#endif  // LIBUWEAVE_INCLUDE_UWEAVE_COMMAND_H_
