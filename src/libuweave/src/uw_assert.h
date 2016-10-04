// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBUWEAVE_SRC_UW_ASSERT_H_
#define LIBUWEAVE_SRC_UW_ASSERT_H_

#include <assert.h>

#include "src/log.h"
#include "src/macro.h"

#ifdef NDEBUG
#define UW_ASSERT(cond_, format_, ...)
#else
#define UW_ASSERT(cond_, format_, ...)    \
  UW_MACRO_BEGIN                          \
  if (!(cond_)) {                         \
    UW_LOG_ERROR(format_, ##__VA_ARGS__); \
    assert((cond_));                      \
  }                                       \
  UW_MACRO_END
#endif

#endif  // LIBUWEAVE_SRC_UW_ASSERT_H_
