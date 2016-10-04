// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBUWEAVE_SRC_BUFFER_H_
#define LIBUWEAVE_SRC_BUFFER_H_

#include "uweave/buffer.h"

struct UwBuffer_ {
  uint8_t* start;
  uint8_t* pos;
  size_t size;
};

/**
 * Get a non-const pointer into the backing bytes.  Used to write directly
 * to the buffer without going through uw_buffer_append().
 */
void uw_buffer_get_bytes_(UwBuffer* buffer, uint8_t** bytes, size_t* length);

/**
 * Set the length in use.
 *
 * If you use uw_buffer_get_bytes_ for raw write access to the backing store,
 * don't forget to update this.
 */
void uw_buffer_set_length_(UwBuffer* buffer, size_t length);

/**
 * Prints a buffer using uw_log_simple_
 */
void uw_buffer_dump_for_debug_(UwBuffer* buffer, char* name);

#endif  // LIBUWEAVE_SRC_BUFFER_H_
