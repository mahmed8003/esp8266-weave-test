// Copyright 2016 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBUWEAVE_SRC_PACKET_HEADER_H_
#define LIBUWEAVE_SRC_PACKET_HEADER_H_

#include <stdbool.h>
#include <stdint.h>

/** Packet header bits positions. */
static const uint8_t kUwPacketHeaderControlPacket = 1 << 7;
static const uint8_t kUwPacketHeaderFirstPacket = 1 << 3;
static const uint8_t kUwPacketHeaderLastPacket = 1 << 2;

typedef enum {
  kUwPacketHeaderCmdConnectionRequest = 0x0,
  kUwPacketHeaderCmdConnectionConfirm = 0x1,
  kUwPacketHeaderCmdError = 0x2,
} UwPacketHeaderCmd;

static inline bool uw_packet_header_is_control_(uint8_t header) {
  return header & kUwPacketHeaderControlPacket;
}

static inline bool uw_packet_header_is_data_(uint8_t header) {
  return !uw_packet_header_is_control_(header);
}

static inline bool uw_packet_header_is_first_(uint8_t header) {
  return header & kUwPacketHeaderFirstPacket;
}

static inline bool uw_packet_header_is_last_(uint8_t header) {
  return header & kUwPacketHeaderLastPacket;
}

static inline uint8_t uw_packet_header_get_counter_(uint8_t header) {
  return header >> 4 & 0x07;
}

static inline uint8_t uw_packet_header_get_command_number_(uint8_t header) {
  return header & 0x0f;
}

static inline uint8_t uw_packet_header_new_data_(bool is_first,
                                                 bool is_last,
                                                 uint8_t counter) {
  return (((counter & 0x07) << 4) |
          (is_first ? kUwPacketHeaderFirstPacket : 0) |
          (is_last ? kUwPacketHeaderLastPacket : 0));
}

static inline uint8_t uw_packet_header_new_control_(UwPacketHeaderCmd command,
                                                    uint8_t counter) {
  return (0x80 | ((counter & 0x07) << 4) | (command & 0xf));
}

#endif  // LIBUWEAVE_SRC_PACKET_HEADER_H_
