// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBUWEAVE_INCLUDE_UWEAVE_BUFFER_H_
#define LIBUWEAVE_INCLUDE_UWEAVE_BUFFER_H_

#include "uweave/base.h"

typedef struct UwBuffer_ UwBuffer;

void uw_buffer_init(UwBuffer* buffer, uint8_t* bytes, size_t size);

/**
 * Reset the current position and fill the buffer with '\0'.
 */
void uw_buffer_reset(UwBuffer* buffer);

/**
 * Returns true if the buffer object has no backing bytes.
 */
bool uw_buffer_is_null(const UwBuffer* buffer);

/**
 * Copy src_bytes to the current buffer position.
 *
 * Return `false` if the buffer is not large enough.
 */
bool uw_buffer_append(UwBuffer* buffer,
                      const uint8_t* src_bytes,
                      size_t src_length);

/**
 * Return the max capacity of the buffer.
 */
size_t uw_buffer_get_size(const UwBuffer* buffer);

/**
 * Return the length in use.
 */
size_t uw_buffer_get_length(const UwBuffer* buffer);

/**
 * Return the current buffer bytes and length used.
 */
void uw_buffer_get_const_bytes(const UwBuffer* buffer,
                               const uint8_t** bytes,
                               size_t* length);

/**
 * Initialize the given `dest` buffer to point to a slice of the current
 * buffer.
 */
void uw_buffer_slice(UwBuffer* buffer,
                     uint8_t* start,
                     size_t length,
                     UwBuffer* dest);

#endif  // LIBUWEAVE_INCLUDE_UWEAVE_BUFFER_H_
