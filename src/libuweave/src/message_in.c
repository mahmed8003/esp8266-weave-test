// Copyright 2016 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/message_in.h"

#include <string.h>

#include "src/log.h"
#include "src/packet_header.h"
#include "src/uw_assert.h"

UwMessageType get_message_type_(uint8_t packet_header) {
  if (uw_packet_header_is_data_(packet_header)) {
    return kUwMessageTypeData;
  }

  switch (uw_packet_header_get_command_number_(packet_header)) {
    case kUwPacketHeaderCmdConnectionRequest:
      return kUwMessageTypeConnectionRequest;

    case kUwPacketHeaderCmdConnectionConfirm:
      return kUwMessageTypeConnectionConfirm;

    case kUwPacketHeaderCmdError:
      return kUwMessageTypeError;

    default:
      return kUwMessageTypeUnknown;
  }
}

void uw_message_in_init_(UwMessageIn* message_in, UwBuffer* buffer) {
  memset(message_in, 0, sizeof(UwMessageIn));
  message_in->buffer = buffer;
  uw_message_in_reset_(message_in);
};

void uw_message_in_reset_(UwMessageIn* message_in) {
  uw_buffer_reset(message_in->buffer);
  message_in->state = kUwMessageStateEmpty;
  message_in->type = kUwMessageTypeUnknown;
  message_in->read_pos = 0;
}

UwMessageType uw_message_in_get_type_(UwMessageIn* message_in) {
  return message_in->type;
}

bool uw_message_in_read_uint8_(UwMessageIn* message_in, uint8_t* out) {
  if (message_in->state != kUwMessageStateComplete) {
    UW_ASSERT(false, "Expected message_in to be complete: %i\n",
              message_in->state);
    return false;
  }

  const uint8_t* bytes;
  size_t length;
  uw_buffer_get_const_bytes(message_in->buffer, &bytes, &length);

  if (message_in->read_pos + 1 > length) {
    UW_ASSERT(false, "Read past end of message_in.\n");
    return false;
  }

  *out = bytes[message_in->read_pos];
  message_in->read_pos++;
  return true;
}

bool uw_message_in_read_uint16_(UwMessageIn* message_in, uint16_t* out) {
  if (message_in->state != kUwMessageStateComplete) {
    UW_ASSERT(false, "Expected message_in to be complete: %i\n",
              message_in->state);
    return false;
  }

  const uint8_t* bytes;
  size_t length;
  uw_buffer_get_const_bytes(message_in->buffer, &bytes, &length);

  if (message_in->read_pos + 2 > length) {
    UW_ASSERT(false, "Read past end of message_in.\n");
    return false;
  }

  // TODO(rginda): b/26988273 - Implement proper ntohs in a shared place.
  // STOPSHIP
  *out = (uint16_t)(bytes[message_in->read_pos] << 8 |
                    bytes[message_in->read_pos + 1]);

  message_in->read_pos += 2;

  return true;
}

bool uw_message_in_read_remaining_bytes_(UwMessageIn* message_in,
                                         UwBuffer* out) {
  if (message_in->state != kUwMessageStateComplete) {
    UW_ASSERT(false, "Expected message_in to be complete: %i\n",
              message_in->state);
    return false;
  }

  const uint8_t* bytes;
  size_t bytes_length;
  uw_buffer_get_const_bytes(message_in->buffer, &bytes, &bytes_length);

  size_t read_length = bytes_length - message_in->read_pos;
  uw_buffer_init(out, (uint8_t*)bytes + message_in->read_pos, read_length);
  uw_buffer_set_length_(out, read_length);

  return true;
}

UwMessageState uw_message_in_get_state_(UwMessageIn* message_in) {
  return message_in->state;
}

UwBuffer* uw_message_in_get_buffer_(UwMessageIn* message_in) {
  return message_in->buffer;
}

UwMessageState uw_message_in_append_packet_(UwMessageIn* message_in,
                                            uint8_t packet_header,
                                            const uint8_t* data_bytes,
                                            size_t data_length) {
  bool first_packet;
  bool last_packet;

  UwMessageType type = get_message_type_(packet_header);

  if (type == kUwMessageTypeData) {
    first_packet = uw_packet_header_is_first_(packet_header);
    last_packet = uw_packet_header_is_last_(packet_header);
  } else {
    first_packet = true;
    last_packet = true;
  }

  if (message_in->state == kUwMessageStateComplete) {
    UW_LOG_ERROR("Not ready for a new packet.\n");
    message_in->state = kUwMessageStateError;

  } else if (first_packet && message_in->state != kUwMessageStateEmpty) {
    UW_LOG_ERROR("Not ready for first packet.\n");
    message_in->state = kUwMessageStateError;

  } else if (!first_packet && message_in->state == kUwMessageStateEmpty) {
    UW_LOG_ERROR("Expected first packet.\n");
    message_in->state = kUwMessageStateError;

  } else {
    if (uw_buffer_append(message_in->buffer, data_bytes, data_length)) {
      message_in->type = type;
      message_in->state =
          last_packet ? kUwMessageStateComplete : kUwMessageStateBusy;
    } else {
      UW_LOG_ERROR("Error appending packet data.\n");
      message_in->state = kUwMessageStateError;
    }
  }

  return message_in->state;
}
