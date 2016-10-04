// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string.h>

#include "src/buffer.h"
#include "src/log.h"
#include "src/uw_assert.h"

void uw_buffer_init(UwBuffer* buffer, uint8_t* bytes, size_t size) {
  if (bytes == NULL) {
    UW_ASSERT(size == 0, "Expected NULL buffer to have size 0\n");
  }

  memset(buffer, 0, sizeof(UwBuffer));
  buffer->start = bytes;
  buffer->size = size;
  buffer->pos = buffer->start;
}

void uw_buffer_reset(UwBuffer* buffer) {
  buffer->pos = buffer->start;
  if (buffer->start != NULL) {
    memset(buffer->start, 0, buffer->size);
  }
}

bool uw_buffer_is_null(const UwBuffer* buffer) {
  return buffer->start == NULL;
}

void uw_buffer_slice(UwBuffer* buffer,
                     uint8_t* start,
                     size_t length,
                     UwBuffer* dest) {
  UW_ASSERT(buffer->start != NULL, "Attempt to slice a NULL buffer\n");
  UW_ASSERT(start >= buffer->start && start + length <= buffer->pos,
            "Slice out of range\n");

  dest->start = start;
  dest->size = length;
  dest->pos = start + length;
}

bool uw_buffer_append(UwBuffer* buffer,
                      const uint8_t* src_bytes,
                      size_t src_length) {
  if (src_length == 0) {
    return true;
  }

  if (uw_buffer_get_length(buffer) + src_length > buffer->size) {
    return false;
  }

  memcpy(buffer->pos, src_bytes, src_length);
  buffer->pos += src_length;
  return true;
}

size_t uw_buffer_get_size(const UwBuffer* buffer) {
  return buffer->size;
}

size_t uw_buffer_get_length(const UwBuffer* buffer) {
  if (buffer->start == NULL) {
    return 0;
  }

  return buffer->pos - buffer->start;
}

void uw_buffer_set_length_(UwBuffer* buffer, size_t length) {
  UW_ASSERT(buffer->start != NULL, "Attempt to set_length on a NULL buffer\n");
  UW_ASSERT(length <= buffer->size, "Length too long, %zu > %zu", length,
            buffer->size);
  buffer->pos = buffer->start + length;
}

void uw_buffer_get_const_bytes(const UwBuffer* buffer,
                               const uint8_t** bytes,
                               size_t* length) {
  *bytes = buffer->start;
  *length = uw_buffer_get_length(buffer);
}

void uw_buffer_get_bytes_(UwBuffer* buffer, uint8_t** bytes, size_t* size) {
  *bytes = buffer->start;
  *size = buffer->size;
}

// TODO(arnarb): Wrap in debug defines or move to log.h
void uw_buffer_dump_for_debug_(UwBuffer* buffer, char* name) {
  char ascii[17];
  memset(ascii, 0, sizeof(ascii));
  uw_log_simple_("%s : UwBuffer[len=%u, capacity=%u]\n", name,
                 (unsigned)uw_buffer_get_length(buffer),
                 (unsigned)uw_buffer_get_size(buffer));
  uint8_t* p = buffer->start;
  size_t ascii_idx = 0;
  while (p < buffer->pos) {
    if (ascii_idx == 0) {
      uw_log_simple_("  0x%04x: ", (unsigned)(p - buffer->start));
    }
    uw_log_simple_(" %02x", *p);
    ascii[ascii_idx] = *p > 31 && *p < 127 ? *p : '.';
    ++p;
    if (++ascii_idx > 15) {
      uw_log_simple_(" %s\n", ascii);
      memset(ascii, 0, sizeof(ascii));
      ascii_idx = 0;
    }
  }
  if (ascii_idx != 0) {
    // Pad out to the width of a normal buffer.
    for (int i = ascii_idx; i < 16; ++i) {
      uw_log_simple_("   ");
    }
    uw_log_simple_(" %s\n", ascii);
  }
  uw_log_simple_("\n");
}
