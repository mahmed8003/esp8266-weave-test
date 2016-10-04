// Copyright 2016 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/command.h"

#include "src/log.h"
#include "src/privet_defines.h"
#include "src/value.h"
#include "src/value_scan.h"

void uw_command_init_(UwCommand* command, uint8_t* buffer, size_t buffer_len) {
  *command = (UwCommand){};
  uw_buffer_init(&command->reply_buffer, buffer, buffer_len);
}

uint32_t uw_command_get_trait(UwCommand* command) {
  return command->trait_id;
}

uint32_t uw_command_get_name(UwCommand* command) {
  return command->name_id;
}

bool uw_command_get_param_int(UwCommand* command,
                              int param_key,
                              int* param_value) {
  if (command->execute_request == NULL ||
      uw_buffer_is_null(&command->execute_request->param_buffer)) {
    UW_LOG_WARN("Execute request has no param (has_request=%d).\n",
                command->execute_request != NULL);
    return false;
  }

  UwValue value = uw_value_undefined();

  UwMapFormat format[] = {
      {.key = uw_value_int(param_key),
       .type = kUwValueTypeInt,
       .value = &value},
  };

  UwStatus scan_status =
      uw_value_scan_map(&command->execute_request->param_buffer, format,
                        uw_value_scan_map_count(sizeof(format)));
  if (!uw_status_is_success(scan_status)) {
    UW_LOG_WARN("Error parsing parameters for key %d (status=%d)\n", param_key,
                scan_status);
    return false;
  }

  if (uw_value_is_undefined(&value)) {
    UW_LOG_WARN("Value not found for key %d\n", param_key);
    return false;
  }

  *param_value = value.value.int_value;
  return true;
}

UwBuffer* uw_command_get_param_buffer(UwCommand* command) {
  if (command->execute_request == NULL) {
    return NULL;
  }
  return &command->execute_request->param_buffer;
}

UwStatus uw_commannd_set_reply_buffer_(UwCommand* command,
                                       int state,
                                       const UwValue* result) {
  // Declare enough UwMapValue slots for any command reply.
  UwMapValue privet_results[5];
  size_t privet_results_count = 0;
  switch (state) {
    // Commands that are done have no command_id field and a result field (which
    // may be empty).
    case PRIVET_COMMAND_OBJ_VALUE_STATE_DONE: {
      privet_results[privet_results_count++] =
          (UwMapValue){.key = uw_value_int(PRIVET_COMMAND_OBJ_KEY_STATE),
                       .value = uw_value_int(state)};
      privet_results[privet_results_count++] = (UwMapValue){
          .key = uw_value_int(PRIVET_COMMAND_OBJ_KEY_RESULT),
          .value = (result != NULL ? *result : uw_value_map(NULL, 0))};
      break;
    }
    // Commands that are in progress or queued have a command_id field and no
    // result or error fields.
    case PRIVET_COMMAND_OBJ_VALUE_STATE_IN_PROGRESS:
    case PRIVET_COMMAND_OBJ_VALUE_STATE_QUEUED: {
      privet_results[privet_results_count++] =
          (UwMapValue){.key = uw_value_int(PRIVET_COMMAND_OBJ_KEY_STATE),
                       .value = uw_value_int(state)};
      privet_results[privet_results_count++] =
          (UwMapValue){.key = uw_value_int(PRIVET_COMMAND_OBJ_KEY_COMMAND_ID),
                       .value = uw_value_int(command->command_id)};
      break;
    }
    // Commands that are in error have no command_id field and an error field.
    case PRIVET_COMMAND_OBJ_VALUE_STATE_ERROR: {
      privet_results[privet_results_count++] =
          (UwMapValue){.key = uw_value_int(PRIVET_COMMAND_OBJ_KEY_STATE),
                       .value = uw_value_int(state)};
      privet_results[privet_results_count++] = (UwMapValue){
          .key = uw_value_int(PRIVET_COMMAND_OBJ_KEY_ERROR),
          .value = (result != NULL ? *result : uw_value_map(NULL, 0))};
      break;
    }
    // Commands that are in cancelled have only the state field.
    case PRIVET_COMMAND_OBJ_VALUE_STATE_CANCELLED: {
      privet_results[privet_results_count++] =
          (UwMapValue){.key = uw_value_int(PRIVET_COMMAND_OBJ_KEY_STATE),
                       .value = uw_value_int(state)};
      break;
    }
    default: {
      UW_LOG_WARN("Unknown command state: %d\n", state);
      break;
    }
  }

  UwValue result_map = uw_value_map(privet_results, privet_results_count);

  // Privet error codes and application error codes are returned separately.
  // This returns the request as a success at the Privet layer, but potentially
  // as an error at the application layer.
  UwStatus status =
      uw_value_encode_value_to_buffer_(&command->reply_buffer, &result_map);
  if (status == kUwStatusValueEncodingOutOfSpace) {
    return kUwStatusPrivetResponseTooLarge;
  }
  return status;
}

UwStatus uw_command_reply_with_value(UwCommand* command,
                                     const UwValue* results) {
  return uw_commannd_set_reply_buffer_(
      command, PRIVET_COMMAND_OBJ_VALUE_STATE_DONE, results);
}

UwStatus uw_command_reply_empty(UwCommand* command) {
  return uw_commannd_set_reply_buffer_(
      command, PRIVET_COMMAND_OBJ_VALUE_STATE_DONE, NULL);
}

UwStatus uw_command_reply_with_error(UwCommand* command, const UwValue* error) {
  return uw_commannd_set_reply_buffer_(
      command, PRIVET_COMMAND_OBJ_VALUE_STATE_ERROR, error);
}

UwStatus uw_command_reply_with_error_code(UwCommand* command,
                                          int32_t error_code,
                                          const char* message) {
  UwMapValue error_map[] = {
      {.key = uw_value_int(PRIVET_RPC_ERROR_KEY_CODE),
       .value = uw_value_int(error_code)},
      {.key = uw_value_int(PRIVET_RPC_ERROR_KEY_MESSAGE),
       .value = uw_value_utf8_string(message)},
  };

  UwValue error =
      uw_value_map(error_map, uw_value_map_count(sizeof(error_map)));
  return uw_command_reply_with_error(command, &error);
}

UwStatus uw_command_has_required_role(UwCommand* command, UwRole role) {
  if (command->execute_request == NULL) {
    return kUwStatusInvalidArgument;
  }
  return uw_session_role_at_least(
      uw_privet_request_get_session_(command->execute_request->privet_request),
      role);
}
