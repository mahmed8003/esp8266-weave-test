// Copyright 2016 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBUWEAVE_INCLUDE_UWEAVE_PROVIDER_STORAGE_H_
#define LIBUWEAVE_INCLUDE_UWEAVE_PROVIDER_STORAGE_H_

#include <stdint.h>
#include <stddef.h>

#include "uweave/config.h"
#include "uweave/status.h"

/*
 * The storage provider enables the persistence of settings and cryptographic
 * matter.  It is intended to be simple to implement on MCUs with limited
 * storage support.
 *
 * TODO: A discussion on sizing, maybe some constants (files <4k?).
 */

/**
 * Opaque blob names.  We use the enum rather than strings to simplify provider
 * implementations.
 */
typedef enum {
  kUwStorageFileNameSettings = 0,
  kUwStorageFileNameKeys = 1,
  kUwStorageFileNameCounters = 2,
  // File ids 3-99 are reserved for future uWeave use.  Application developers
  // that would like to share the same name-space can use ids starting at
  // VendorStart.
  kUwStorageFileNameVendorStart = 100,
} UwStorageFileName;

/**
 * Initializes any global state for the storage provider at startup.  No other
 * uw or uwp functions may be invoked.
 */
bool uwp_storage_init();

/**
 * Reads the named blob into the provided buffer and stores the blob length into
 * result_len.  If the size of the actual blob exceeds the buffer, no data is
 * returned.
 */
UwStatus uwp_storage_get(UwStorageFileName name,
                         uint8_t buf[],
                         size_t buf_len,
                         size_t* result_len);

/**
 * Writes the provided buffer into a named blob.  If the write fails, due to
 * errors or power failure, subsequent calls to uwp_storage_get should return
 * the prior version.
 */
UwStatus uwp_storage_put(UwStorageFileName name, uint8_t buf[], size_t buf_len);

/**
 * Aligns a length to a value that is acceptable to the underlying filesystem.
 */
static inline size_t uwp_storage_size_align(size_t size) {
  // Zero pad-out the end of the buffer to match UW_STORAGE_ALIGNMENT.
  if (size & (UW_STORAGE_ALIGNMENT - 1)) {
    size &= ~(UW_STORAGE_ALIGNMENT - 1);
    size += UW_STORAGE_ALIGNMENT;
  }
  return size;
}

#endif  // LIBUWEAVE_INCLUDE_UWEAVE_PROVIDER_STORAGE_H_
