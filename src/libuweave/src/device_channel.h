// Copyright 2016 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBUWEAVE_SRC_DEVICE_CHANNEL_H_
#define LIBUWEAVE_SRC_DEVICE_CHANNEL_H_

#include "src/buffer.h"
#include "src/channel.h"

typedef bool (*UwDeviceChannelMessageExchangeHandler)(void* data,
                                                      UwBuffer* request,
                                                      UwBuffer* reply);

typedef struct {
  UwDeviceChannelMessageExchangeHandler handler;
  void* data;
} UwDeviceChannelHandshakeConfig;

typedef bool (*UwDeviceChannelConnectionResetHandler)(void* data);

typedef struct {
  UwDeviceChannelConnectionResetHandler handler;
  void* data;
} UwDeviceChannelConnectionResetConfig;

/**
 * Provides the device-oriented server packet dispatching channel for the
 * embedded devices.
 *
 * Handles the outer handshake and packet size negotiation.  Inner message
 * handling is passed to the handshake_config callback which is responsible for
 * additional handshaking (encryption) as well as dispatching uWeave payload
 * data to the UwDevice handlers.
 */
typedef struct {
  UwDeviceChannelHandshakeConfig handshake_config;
  UwDeviceChannelConnectionResetConfig reset_config;
  UwChannel channel;
  // True if we've ever received a connection request.
  bool did_connection_request;
} UwDeviceChannel;

void uw_device_channel_init_(UwDeviceChannel* device_channel,
                             UwDeviceChannelHandshakeConfig handshake_config,
                             UwDeviceChannelConnectionResetConfig reset_config,
                             UwBuffer* message_in_buffer,
                             UwBuffer* message_out_buffer,
                             size_t max_packet_size);

void uw_device_channel_reset_(UwDeviceChannel* device_channel);

/** Completes the current message exchange.
 *
 * Resets the message buffers and potentially modifies other state.
 */
void uw_device_channel_complete_exchange_(UwDeviceChannel* device_channel);

/** Returns true if the channel is connected. */
bool uw_device_channel_is_connected(UwDeviceChannel* channel);

/** Returns the UwChannel object for this channel. */
static inline UwChannel* uw_device_channel_get_channel_(
    UwDeviceChannel* device_channel) {
  return &device_channel->channel;
}

#endif  // LIBUWEAVE_SRC_DEVICE_CHANNEL_H_
