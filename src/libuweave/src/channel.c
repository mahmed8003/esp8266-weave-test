// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/channel.h"

#include <string.h>

#include "src/log.h"
#include "src/packet_header.h"
#include "src/uw_assert.h"

void uw_channel_init_(UwChannel* channel,
                      UwChannelMessageConfig message_config,
                      UwBuffer* message_in_buffer,
                      UwBuffer* message_out_buffer,
                      size_t max_packet_size) {
  *channel = (UwChannel){.message_config = message_config,
                         .max_packet_size = max_packet_size,
                         .packet_in_counter = 0,
                         .packet_out_counter = 0};

  uw_message_in_init_(&channel->message_in, message_in_buffer);
  uw_message_out_init_(&channel->message_out, message_out_buffer);
}

void uw_channel_reset_messages_(UwChannel* channel) {
  uw_message_in_reset_(&channel->message_in);
  uw_message_out_reset_(&channel->message_out);
}

void uw_channel_reset_(UwChannel* channel) {
  channel->packet_in_counter = 0;
  channel->packet_out_counter = 0;

  uw_channel_reset_messages_(channel);
}

size_t uw_channel_get_max_packet_size_(UwChannel* channel) {
  return channel->max_packet_size;
}

void uw_channel_set_max_packet_size_(UwChannel* channel,
                                     uint8_t max_packet_size) {
  channel->max_packet_size = max_packet_size;
}

UwMessageIn* uw_channel_get_message_in_(UwChannel* channel) {
  return &channel->message_in;
}

UwMessageOut* uw_channel_get_message_out_(UwChannel* channel) {
  return &channel->message_out;
}

UwMessageState uw_channel_get_in_state_(UwChannel* channel) {
  return uw_message_in_get_state_(&channel->message_in);
}

UwMessageState uw_channel_get_out_state_(UwChannel* channel) {
  return uw_message_out_get_state_(&channel->message_out);
}

bool uw_channel_append_packet_in_(UwChannel* channel, UwBuffer* packet_buffer) {
  const uint8_t* packet_bytes = NULL;
  size_t packet_length = 0;
  uw_buffer_get_const_bytes(packet_buffer, &packet_bytes, &packet_length);

  // Pluck out the header.
  uint8_t packet_header = packet_bytes[0];

  // The rest is data.
  packet_bytes++;
  packet_length--;

  // Confirm the packet_counter.
  uint8_t packet_counter = uw_packet_header_get_counter_(packet_header);
  if (packet_counter != channel->packet_in_counter) {
    UW_LOG_ERROR("Unexpected packet counter %d != %d.\n", packet_counter,
                 channel->packet_in_counter);
    return false;
  }

  channel->packet_in_counter = (channel->packet_in_counter + 1) % 8;

  // Append the packet and check the new state.
  UwMessageState message_state = uw_message_in_append_packet_(
      &channel->message_in, packet_header, packet_bytes, packet_length);
  if (message_state == kUwMessageStateError) {
    return false;
  }

  if (message_state == kUwMessageStateComplete) {
    if (channel->message_config.handler != NULL) {
      return channel->message_config.handler(channel->message_config.data);
    }
  }

  return true;
}

bool uw_channel_get_next_packet_out_(UwChannel* channel,
                                     UwBuffer* packet_buffer) {
  UwMessageState message_state = uw_message_out_get_next_packet_(
      &channel->message_out, packet_buffer, channel->max_packet_size,
      channel->packet_out_counter);
  channel->packet_out_counter = (channel->packet_out_counter + 1) % 8;
  return message_state != kUwMessageStateError;
}
