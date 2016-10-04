// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBUWEAVE_INCLUDE_UWEAVE_BASE_H_
#define LIBUWEAVE_INCLUDE_UWEAVE_BASE_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __GNUC__
#define UW_PRINTFLIKE(n_, m_) __attribute__((format(printf, n_, m_)))
#else
#define UW_PRINTFLIKE(n_, m_)
#endif

#endif  // LIBUWEAVE_INCLUDE_UWEAVE_BASE_H_
