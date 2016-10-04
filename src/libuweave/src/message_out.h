// Copyright 2016 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBUWEAVE_SRC_MESSAGE_OUT_H_
#define LIBUWEAVE_SRC_MESSAGE_OUT_H_

#include "src/buffer.h"
#include "src/message.h"

/**
 * Representation of an outbound message to be sent as a series of packets.
 *
 * This class is used in conjunction with UwChannel.
 *
 * The message starts in the empty state and transitions to the busy state
 * when the first get_next_packet call is made.  The message is copied out
 * in packet-sized bites until the entire message is ready and the state
 * transitions to complete.  Once in the complete state, the message
 * must be reset before it can be reused.
 */
typedef struct {
  UwBuffer* buffer;
  UwMessageState state;
  UwMessageType type;
  size_t packet_offset;  // The position of the start of the next packet.
} UwMessageOut;

void uw_message_out_init_(UwMessageOut* message_out, UwBuffer* buffer);

/** Resets the message state to empty and clears the associated buffer. */
void uw_message_out_reset_(UwMessageOut* message_out);

/**
 * Returns the current state of this message.
 *
 * Error: The message is invalid and must be reset.
 * Empty: The client application is free to write into the message buffer.
 * Busy: The message has at least one packet's worth of data left to send.
 * Complete: All packets of the message have been retrieved.  The message must
 *   be reset before the next message can be processed.
 */
UwMessageState uw_message_out_get_state_(UwMessageOut* message_out);

/**
 * Appends one byte to the message payload.
 */
bool uw_message_out_append_uint8_(UwMessageOut* message_out,
                                  uint8_t data);

/**
 * Appends a uint16_t to the message payload, in network order.
 */
bool uw_message_out_append_uint16_(UwMessageOut* message_out,
                                   uint16_t data);
/**
 * Appends an arbitrary number of bytes to the message payload in the order
 * given.
 */
bool uw_message_out_append_bytes_(UwMessageOut* message_out,
                                  const uint8_t* bytes,
                                  size_t bytes_length);

/**
 * Marks the start of assembly of the outbound message.
 *
 * Performs some debug assertions and sets the message_type as supplied.
 * State will remain Empty until uw_message_out_ready_ is called.
 */
void uw_message_out_start_(UwMessageOut* message_out,
                           UwMessageType message_type);

/**
 * Marks the out message as ready to send.
 */
void uw_message_out_ready_(UwMessageOut* message_out);

/**
 * Cancels a uw_message_out_start_.
 */
void uw_message_out_discard_(UwMessageOut* message_out);

/** Returns a pointer the message buffer passed into init_(). */
UwBuffer* uw_message_out_get_buffer_(UwMessageOut* message_out);

/**
 * Copies the next portion of the message into the given packet buffer.
 *
 * The packet_buffer must be at least as large as max_packet_size.
 *
 * The max_packet_size is independent of the packet_buffer size so that it's
 * possible to negotiate an mtu smaller than the pre-allocated packet_buffer.
 */
UwMessageState uw_message_out_get_next_packet_(UwMessageOut* message_out,
                                               UwBuffer* packet_buffer,
                                               size_t max_packet_size,
                                               uint8_t packet_counter);

#endif  // LIBUWEAVE_SRC_MESSAGE_OUT_H_
