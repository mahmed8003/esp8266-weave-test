// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBUWEAVE_INCLUDE_UWEAVE_PROVIDER_LOG_H_
#define LIBUWEAVE_INCLUDE_UWEAVE_PROVIDER_LOG_H_

#include <stdarg.h>

void uwp_log_vprintf(const char* format, va_list ap);

#endif  // LIBUWEAVE_INCLUDE_UWEAVE_PROVIDER_LOG_H_
