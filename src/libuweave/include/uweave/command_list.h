// Copyright 2016 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBUWEAVE_INCLUDE_UWEAVE_COMMAND_LIST_H_
#define LIBUWEAVE_INCLUDE_UWEAVE_COMMAND_LIST_H_

#include <stddef.h>
#include <stdint.h>

/**
 * Holds the set of current and recent commands.
 *
 * In the Privet model, there can be a single active command, a number of
 * deferred commands, and a history of recent commands.
 *
 * A deferred command can be used when an operation will have enough delay to
 * block other operations and the client should check back for the result some
 * number of seconds later.
 *
 * Applications that do not require deferred commands need a minimum of one
 * UwCommand in the UwCommandList.
 */
typedef struct UwCommandList_ UwCommandList;

/**
 * Returns the required size of memory allocation for the UwDevice command
 * buffers.
 *
 * The command_count is the total sum of immediate and deferred
 * commands that can be active at one time.  These can be viewed by
 * /commands/list.
 *
 * The maximum_response_len is the longest application-response
 * that can be returned.  This is bound by the transport buffer size minus the
 * header overhead.  In the case of BLE (UW_BLE_TRANSPORT_REPLY_BUFFER_SIZE),
 * this is around 500 bytes.
 *
 * If the application will only ever send short responses, then the length can
 * be shortened to a conservative bound of the reply length to save memory.  If
 * the maximum_response_length is shorter than the actual response, the command
 * will return an error to the client.
 */
size_t uw_command_list_sizeof(int32_t command_count,
                              size_t maximum_response_lenth);

/**
 * Initializes the command_list with the buffer allocated from the specified
 * command count and maximum_response length.
 */
void uw_command_list_init(UwCommandList* command_list,
                          int32_t command_count,
                          size_t maximum_response_len);

#endif  // LIBUWEAVE_INCLUDE_UWEAVE_COMMAND_LIST_H_
