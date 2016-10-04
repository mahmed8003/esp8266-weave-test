// Copyright 2016 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBUWEAVE_SRC_MESSAGE_IN_H_
#define LIBUWEAVE_SRC_MESSAGE_IN_H_

#include "src/buffer.h"
#include "src/message.h"

/**
 * Representation of an inbound message received as a series of packets.
 *
 * This class is used in conjunction with UwChannel.
 *
 * The message starts in the empty state and transitions to the busy state
 * when the first append_packet call is made.  When the final packet is appended
 * the state transitions to complete.  Once in the complete state, the message
 * must be reset before it can be reused.
 */
typedef struct {
  UwBuffer* buffer;
  UwMessageState state;
  UwMessageType type;
  size_t read_pos;  // Used and updated by uw_message_in_read_* methods.
} UwMessageIn;

void uw_message_in_init_(UwMessageIn* message_in, UwBuffer* buffer);

/** Resets the message state to empty and clears the associated buffer. */
void uw_message_in_reset_(UwMessageIn* message_in);

/** Returns the type of the message. */
UwMessageType uw_message_in_get_type_(UwMessageIn* message_in);

/**
 * Returns a uint8_t from the current message position and advances the
 * message position.
 */
bool uw_message_in_read_uint8_(UwMessageIn* message_in, uint8_t* out);

/**
 * Converts the two bytes at the current message position from network order
 * into a uint16_t and advances the message position.
 */
bool uw_message_in_read_uint16_(UwMessageIn* message_in, uint16_t* out);

/**
 * Initializes the given UwBuffer object to point to the remaining bytes,
 * starting at the current message position
 *
 * The resulting UwBuffer object shares its backing store with this
 * object's buffer.
 */
bool uw_message_in_read_remaining_bytes_(UwMessageIn* message_in,
                                         UwBuffer* out);

/**
 * Returns the current state of this message.
 *
 * Error: The message is invalid and must be reset.
 * Empty: The message buffer is empty.
 * Busy: At least one packet of the message has been written, but more
 *   are expected.
 * Complete: All packets of the message have been written.  Clients are free
 *   to read the message buffer.  The message must be reset before the next
 *   message can be processed.
 */
UwMessageState uw_message_in_get_state_(UwMessageIn* message_in);

/** Returns a pointer the message buffer passed into init_(). */
UwBuffer* uw_message_in_get_buffer_(UwMessageIn* message_in);

/**
 * Appends the next packet to this message and returns the new message state.
 */
UwMessageState uw_message_in_append_packet_(UwMessageIn* message_in,
                                            uint8_t packet_header,
                                            const uint8_t* data_bytes,
                                            size_t data_length);

#endif  // LIBUWEAVE_SRC_MESSAGE_IN_H_
