// Copyright 2016 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/device_channel.h"

#include "src/uw_assert.h"
#include "src/log.h"

static bool handle_message_in_(void* data);
static bool handle_connection_request_(UwDeviceChannel* channel);

void uw_device_channel_init_(UwDeviceChannel* device_channel,
                             UwDeviceChannelHandshakeConfig handshake_config,
                             UwDeviceChannelConnectionResetConfig reset_config,
                             UwBuffer* message_in_buffer,
                             UwBuffer* message_out_buffer,
                             size_t max_packet_size) {
  *device_channel = (UwDeviceChannel){
      .handshake_config = handshake_config, .reset_config = reset_config,
  };

  uw_channel_init_(&device_channel->channel,
                   (UwChannelMessageConfig){.handler = handle_message_in_,
                                            .data = device_channel},
                   message_in_buffer, message_out_buffer, max_packet_size);
}

static void session_reset_(UwDeviceChannel* device_channel) {
  device_channel->did_connection_request = false;
  if (device_channel->reset_config.handler != NULL) {
    device_channel->reset_config.handler(device_channel->reset_config.data);
  }
}

void uw_device_channel_reset_(UwDeviceChannel* device_channel) {
  session_reset_(device_channel);
  uw_channel_reset_(&device_channel->channel);
}

void uw_device_channel_complete_exchange_(UwDeviceChannel* device_channel) {
  uw_channel_reset_messages_(&device_channel->channel);
}

bool uw_device_channel_is_connected(UwDeviceChannel* device_channel) {
  UwChannel* channel = uw_device_channel_get_channel_(device_channel);
  return device_channel->did_connection_request &&
         uw_message_in_get_state_(&channel->message_in) !=
             kUwMessageStateError &&
         uw_message_out_get_state_(&channel->message_out) !=
             kUwMessageStateError;
}

static bool handle_message_in_(void* data) {
  UwDeviceChannel* device_channel = (UwDeviceChannel*)data;
  UwChannel* channel = uw_device_channel_get_channel_(device_channel);
  switch (uw_message_in_get_type_(&channel->message_in)) {
    case kUwMessageTypeData: {
      if (!device_channel->did_connection_request) {
        UW_LOG_WARN("Got data packet before connection request.\n");
        return false;
      }

      // Data types are handled by the channel consumer.
      return true;
    }
    case kUwMessageTypeConnectionRequest: {
      if (device_channel->did_connection_request) {
        UW_LOG_INFO("Resetting open session\n");
        session_reset_(device_channel);
      }
      return handle_connection_request_(device_channel);
    }
    case kUwMessageTypeConnectionConfirm: {
      UW_LOG_ERROR("Unexpected connection confirm packet.\n");
      return false;
    }
    case kUwMessageTypeError: {
      UW_LOG_ERROR("Received connection error control packet.\n");
      return false;
    }
    case kUwMessageTypeUnknown:
    default: {
      UW_LOG_ERROR("Received unknown control packet.\n");
      return false;
    }
  }

  UW_ASSERT(false, "Fell through handle_message_in_");
  return false;
}

static bool handle_connection_request_(UwDeviceChannel* device_channel) {
  UwChannel* channel = uw_device_channel_get_channel_(device_channel);
  UwMessageIn* message_in = &channel->message_in;

  uint16_t min_version = 0;
  if (!uw_message_in_read_uint16_(message_in, &min_version) ||
      min_version != 1) {
    UW_LOG_WARN("Invalid minimum version in connection request: %i\n",
                min_version);
    return false;
  }

  uint16_t max_version = 0;
  if (!uw_message_in_read_uint16_(message_in, &max_version)) {
    UW_LOG_WARN("Missing max_version");
    return false;
  }

  uint16_t max_packet_size = 0;
  if (!uw_message_in_read_uint16_(message_in, &max_packet_size) ||
      max_packet_size < 20) {
    UW_LOG_WARN("Invalid max_packet_size: %i\n", max_packet_size);
    return false;
  }

  if (max_packet_size < channel->max_packet_size) {
    uw_channel_set_max_packet_size_(channel, max_packet_size);
  } else {
    max_packet_size = channel->max_packet_size;
  }

  uw_message_out_start_(&device_channel->channel.message_out,
                        kUwMessageTypeConnectionConfirm);
  uw_message_out_append_uint16_(&device_channel->channel.message_out,
                                min_version);
  uw_message_out_append_uint16_(&device_channel->channel.message_out,
                                max_packet_size);

  bool handshake_result = false;

  UwBuffer connect_request_data;
  if (!uw_message_in_read_remaining_bytes_(message_in, &connect_request_data)) {
    UW_ASSERT(false, "Error reading connection request data.");
  }

  if (device_channel->handshake_config.handler) {
    UwBuffer* connect_confirm_data =
        uw_message_out_get_buffer_(&device_channel->channel.message_out);

    handshake_result = device_channel->handshake_config.handler(
        device_channel->handshake_config.data, &connect_request_data,
        connect_confirm_data);
  }

  if (!handshake_result) {
    UW_LOG_WARN("Handshake failed.\n");
    uw_message_out_discard_(&device_channel->channel.message_out);
    uw_message_out_start_(&device_channel->channel.message_out,
                          kUwMessageTypeError);
  }

  uw_message_out_ready_(&device_channel->channel.message_out);

  // TODO(rginda): Properly distinguish between a channel that got and failed
  // the handshake from one that has never tried to handshake.
  device_channel->did_connection_request = handshake_result;

  return true;
}
