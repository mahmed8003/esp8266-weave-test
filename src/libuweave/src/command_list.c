// Copyright 2016 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/command_list.h"
#include "src/command.h"
#include "src/log.h"
#include "src/time.h"

size_t uw_command_list_sizeof(int32_t command_count,
                              size_t maximum_response_lenth) {
  return sizeof(UwCommandList) +
         (sizeof(UwCommand) + maximum_response_lenth) * command_count;
}

void uw_command_list_init(UwCommandList* command_list,
                          int32_t command_count,
                          size_t maximum_response_len) {
  uint8_t* buf = (uint8_t*)command_list;

  UwCommand* command_array = (UwCommand*)(buf + sizeof(UwCommandList));

  uint8_t* command_buffers =
      buf + sizeof(UwCommandList) + (sizeof(UwCommand) * command_count);

  *command_list =
      (UwCommandList){.count = command_count, .commands = command_array};

  for (size_t i = 0; i < command_count; ++i) {
    uw_command_init_(&command_list->commands[i],
                     command_buffers + (i * maximum_response_len),
                     maximum_response_len);
  }
}

static inline bool is_complete_(UwCommand* command) {
  // The enum is defined is eviction preference order, and all completed states
  // are below kUwCommandStateCancelled.
  return (command->state <= kUwCommandStateCompletedMarker);
}

static inline bool is_preferred_(UwCommand* lhs, UwCommand* rhs) {
  // Prefer empty commands.
  if (lhs->state == kUwCommandStateEmpty &&
      rhs->state != kUwCommandStateEmpty) {
    return true;
  }
  // If in a very done state, break ties by command_id.
  if (is_complete_(lhs) && is_complete_(rhs)) {
    return lhs->command_id < rhs->command_id;
  }
  return lhs->state < rhs->state;
}

UwCommand* uw_command_list_get_free_or_evict_(UwCommandList* command_list) {
  if (command_list == NULL) {
    UW_LOG_ERROR("Attempting to get command from NULL command_list.");
    return NULL;
  }

  UwCommand* candidate = NULL;
  for (size_t i = 0; i < command_list->count; ++i) {
    UwCommand* current = &command_list->commands[i];
    if (candidate == NULL) {
      if (is_complete_(current)) {
        candidate = current;
      }
    } else {
      if (is_preferred_(current, candidate)) {
        candidate = current;
      }
    }
  }
  if (candidate == NULL) {
    // TODO(jmccullough): eviction.
    return NULL;
  }
  candidate->command_id = ++command_list->command_id_sequence;
  candidate->tick_stamp = uw_time_get_uptime_seconds_();
  return candidate;
}

UwCommand* uw_command_list_get_command_by_id(UwCommandList* command_list,
                                             uint32_t command_id) {
  if (command_list == NULL) {
    UW_LOG_ERROR("Attempting to get command from NULL command_list.");
    return NULL;
  }

  for (size_t i = 0; i < command_list->count; ++i) {
    if (!uw_command_is_initialized_(&command_list->commands[i])) {
      continue;
    }
    if (command_list->commands[i].command_id == command_id) {
      return &command_list->commands[i];
    }
  }

  return NULL;
}
