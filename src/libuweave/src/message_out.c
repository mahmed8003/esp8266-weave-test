// Copyright 2016 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/message_out.h"

#include <string.h>

#include "src/packet_header.h"
#include "src/uw_assert.h"

void uw_message_out_init_(UwMessageOut* message_out, UwBuffer* buffer) {
  memset(message_out, 0, sizeof(UwMessageOut));
  message_out->buffer = buffer;
  uw_message_out_reset_(message_out);
};

void uw_message_out_reset_(UwMessageOut* message_out) {
  uw_buffer_reset(message_out->buffer);
  message_out->state = kUwMessageStateEmpty;
  message_out->type = kUwMessageTypeUnknown;
  message_out->packet_offset = 0;
}

bool uw_message_out_append_uint8_(UwMessageOut* message_out,
                                  uint8_t data) {
  return uw_buffer_append(message_out->buffer, &data, sizeof(data));
}

bool uw_message_out_append_uint16_(UwMessageOut* message_out,
                                   uint16_t data) {
  // TODO(rginda): b/26988273 - Implement proper htons in a shared place.
  // STOPSHIP
  uint8_t bytes[2];
  bytes[0] = (data >> 8) & 0xFF;
  bytes[1] = data & 0xFF;

  return uw_buffer_append(message_out->buffer, &bytes[0], sizeof(bytes));
}

bool uw_message_out_append_bytes_(UwMessageOut* message_out,
                                  const uint8_t* bytes,
                                  size_t bytes_length) {
  return uw_buffer_append(message_out->buffer, bytes, bytes_length);
}

UwMessageState uw_message_out_get_state_(UwMessageOut* message_out) {
  return message_out->state;
}

void uw_message_out_start_(UwMessageOut* message_out,
                           UwMessageType message_type) {
  if (message_out->type != kUwMessageTypeUnknown) {
    UW_ASSERT(false, "Expected message type to be unknown: %i\n",
              (int)message_out->type);
    return;
  }

  if (uw_buffer_get_length(message_out->buffer) != 0) {
    UW_ASSERT(false, "message_out already in use.\n");
    return;
  }

  if (message_out->packet_offset != 0) {
    UW_ASSERT(false, "Expected packed offset of zero.\n");
    return;
  }

  message_out->type = message_type;
}

void uw_message_out_ready_(UwMessageOut* message_out) {
  if (message_out->state != kUwMessageStateEmpty) {
    UW_ASSERT(false, "Expected message_out to be in empty state: %i",
              message_out->state);
    return;
  }

  if (message_out->type == kUwMessageTypeUnknown) {
    UW_ASSERT(false, "Expected the message type to be known.\n");
    return;
  }

  if (message_out->packet_offset != 0) {
    UW_ASSERT(false, "Expected packed offset of zero.\n");
    return;
  }

  message_out->state = kUwMessageStateBusy;
}

void uw_message_out_discard_(UwMessageOut* message_out) {
  if (message_out->state != kUwMessageStateEmpty) {
    UW_ASSERT(false, "Expected message_out to be in empty state: %i",
              message_out->state);
    return;
  }

  if (message_out->type == kUwMessageTypeUnknown) {
    UW_ASSERT(false, "Expected the message type to be known.\n");
    return;
  }

  if (message_out->packet_offset != 0) {
    UW_ASSERT(false, "Expected packed offset of zero.\n");
    return;
  }

  uw_buffer_reset(message_out->buffer);
  message_out->type = kUwMessageTypeUnknown;
}

UwBuffer* uw_message_out_get_buffer_(UwMessageOut* message_out) {
  return message_out->buffer;
}

UwMessageState uw_message_out_get_next_packet_(UwMessageOut* message_out,
                                               UwBuffer* packet_buffer,
                                               size_t max_packet_size,
                                               uint8_t packet_counter) {
  if (uw_buffer_get_length(packet_buffer) != 0) {
    UW_ASSERT(false, "Expected empty packet buffer.\n");
    message_out->state = kUwMessageStateError;
    return message_out->state;
  }

  if (message_out->state != kUwMessageStateBusy) {
    UW_ASSERT(false, "Expected message state to be busy: %i\n",
              (int)message_out->state);
    message_out->state = kUwMessageStateError;
    return message_out->state;
  }

  size_t packet_buffer_size = uw_buffer_get_size(packet_buffer);

  if (packet_buffer_size < max_packet_size) {
    UW_ASSERT(false, "Expected larger packet buffer.\n");
    message_out->state = kUwMessageStateError;
    return message_out->state;
  }

  if (message_out->state == kUwMessageStateError ||
      message_out->state == kUwMessageStateComplete) {
    UW_ASSERT(false, "Can't get next packet, current state: %i\n",
              (int)message_out->state);
    message_out->state = kUwMessageStateError;
    return message_out->state;
  }

  const uint8_t* message_bytes = NULL;
  size_t message_length = 0;
  uw_buffer_get_const_bytes(message_out->buffer, &message_bytes,
                            &message_length);

  // Assume we're not done.
  bool is_last = false;

  // Packet data length is the amount of the remaining message we can fit into
  // the next packet.  We start off assuming we'll fill the packet.  The minus
  // one accounts for the 1 byte header.
  size_t packet_data_length = max_packet_size - 1;

  // How much of the message is left to send?
  size_t message_length_remaining = message_length - message_out->packet_offset;

  // If our assumed packet_data_length is more than what we actually have to
  // send, reduce it to match and set is_last.
  if (message_length_remaining <= packet_data_length) {
    packet_data_length = message_length_remaining;
    is_last = true;
  }

  uint8_t packet_header;

  if (message_out->type == kUwMessageTypeData) {
    packet_header = uw_packet_header_new_data_(
        /* is_first */ message_out->packet_offset == 0,
        is_last, packet_counter);
  } else {
    if (!is_last) {
      UW_ASSERT(false, "Expected control message to fit in a single packet.\n");
      message_out->state = kUwMessageStateError;
      return message_out->state;
    }

    packet_header = uw_packet_header_new_control_(
        uw_message_type_to_header_cmd_(message_out->type), packet_counter);
  }

  if (uw_buffer_append(packet_buffer, &packet_header, 1) &&
      uw_buffer_append(packet_buffer,
                       message_bytes + message_out->packet_offset,
                       packet_data_length)) {
    message_out->packet_offset += packet_data_length;
    message_out->state =
        is_last ? kUwMessageStateComplete : kUwMessageStateBusy;

  } else {
    UW_ASSERT(false, "Error appending packet data.\n");
    message_out->state = kUwMessageStateError;
  }

  return message_out->state;
}
