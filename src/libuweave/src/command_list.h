// Copyright 2016 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBUWEAVE_SRC_COMMAND_LIST_H_
#define LIBUWEAVE_SRC_COMMAND_LIST_H_

#include "uweave/command_list.h"
#include "uweave/command.h"

struct UwCommandList_ {
  size_t count;
  uint32_t command_id_sequence;
  UwCommand* commands;
};

UwCommand* uw_command_list_get_free_or_evict_(UwCommandList* command_list);

UwCommand* uw_command_list_get_command_by_id(UwCommandList* command_list,
                                             uint32_t command_id);

#endif  // LIBUWEAVE_SRC_COMMAND_LIST_H_
