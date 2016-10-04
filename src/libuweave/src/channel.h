// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBUWEAVE_SRC_CHANNEL_H_
#define LIBUWEAVE_SRC_CHANNEL_H_

#include "src/message.h"
#include "src/message_in.h"
#include "src/message_out.h"
#include "src/buffer.h"
#include "src/channel_encryption.h"

typedef bool (*UwChannelMessageHandler)(void* data);

typedef struct {
  UwChannelMessageHandler handler;
  void* data;
} UwChannelMessageConfig;

/**
 * Baseline packet channel for uWeave.
 *
 * Assembles incoming packets into the message_in and splits up outgoing packets
 * from message_out.  Sets the packet counter on out-bound packets and ensures
 * correct packet numbering on inbound packets.
 *
 * Provides flexible event handling via the UwChannelMessageHandler callback,
 * which is invoked when an incoming message is complete.
 */
typedef struct {
  UwChannelMessageConfig message_config;
  size_t max_packet_size;

  uint8_t packet_in_counter;
  UwMessageIn message_in;

  uint8_t packet_out_counter;
  UwMessageOut message_out;
} UwChannel;

/**
 * Creates a channel with the specified read buffer and handlers.
 *
 * The message_config handler is invoked when a packet completes re-assembly in
 * the message_in_buffer.
 */
void uw_channel_init_(UwChannel* channel,
                      UwChannelMessageConfig message_config,
                      UwBuffer* message_in_buffer,
                      UwBuffer* message_out_buffer,
                      size_t max_packet_size);

/**
 * Resets the buffer state for an individual command but preserves the packet
 * counter.  For use between commands in the same connection.
 */
void uw_channel_reset_messages_(UwChannel* channel);

/**
 * Resets the channel state without affecting the current max_packet_size. For
 * use between connections.
 */
void uw_channel_reset_(UwChannel* channel);

/** Gets the maximum size of a packet. */
size_t uw_channel_get_max_packet_size_(UwChannel* channel);

/** Sets the maximum size of a packet. */
void uw_channel_set_max_packet_size_(UwChannel* channel,
                                     uint8_t max_packet_size);

/** Returns the UwMessageIn object for this channel. */
UwMessageIn* uw_channel_get_message_in_(UwChannel* channel);

/** Returns the UwMessageOut object for this channel. */
UwMessageOut* uw_channel_get_message_out_(UwChannel* channel);

/** Returns the state of the inbound message. */
UwMessageState uw_channel_get_in_state_(UwChannel* channel);

/** Returns the state of the outbound message. */
UwMessageState uw_channel_get_out_state_(UwChannel* channel);

/**
 * Appends the given packet buffer to the current inbound message.
 *
 * This function returns false on error, which indicates the channel is in an
 * undefined state.  The underlying transport should be reset and the channel
 * should be discarded or re-initialized.
 */
bool uw_channel_append_packet_in_(UwChannel* channel, UwBuffer* packet_buffer);

/**
 * Writes the next chunk of the current outbound message to the given packet
 * buffer.
 *
 * This function returns false on error, which indicates the channel is in an
 * undefined state.  The underlying transport should be reset and the channel
 * should be discarded or re-initialized.
 */
bool uw_channel_get_next_packet_out_(UwChannel* channel,
                                     UwBuffer* packet_buffer);

#endif  // LIBUWEAVE_SRC_CHANNEL_H_
