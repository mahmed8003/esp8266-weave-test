// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBUWEAVE_SRC_LOG_H_
#define LIBUWEAVE_SRC_LOG_H_

#include <stdarg.h>

#include "uweave/base.h"
#include "uweave/config.h"
#include "uweave/provider/log.h"

#include "src/macro.h"

#define UW_LOG(file_, line_, level_, format_, ...)                            \
  ((level_ <= UW_LOG_LEVEL)                                                   \
       ? uw_log_append_((file_), (line_), (level_), (format_), ##__VA_ARGS__) \
       : 0)

#define UW_LOG_LEVEL_NONE 0
#define UW_LOG_LEVEL_ERROR 1
#define UW_LOG_LEVEL_WARN 2
#define UW_LOG_LEVEL_INFO 3
#define UW_LOG_LEVEL_DEBUG 4

#define UW_LOG_ERROR(format_, ...) \
  UW_LOG(__FILE__, __LINE__, UW_LOG_LEVEL_ERROR, (format_), ##__VA_ARGS__)
#define UW_LOG_WARN(format_, ...) \
  UW_LOG(__FILE__, __LINE__, UW_LOG_LEVEL_WARN, (format_), ##__VA_ARGS__)
#define UW_LOG_INFO(format_, ...) \
  UW_LOG(__FILE__, __LINE__, UW_LOG_LEVEL_INFO, (format_), ##__VA_ARGS__)
#define UW_LOG_DEBUG(format_, ...) \
  UW_LOG(__FILE__, __LINE__, UW_LOG_LEVEL_DEBUG, (format_), ##__VA_ARGS__)

/** Logs the origin of a status value and returns the status. */
// We use the comma operator to discard the log statement value.
#define UW_STATUS_AND_LOG_DEBUG(status_, format_, ...)                     \
  UW_LOG(__FILE__, __LINE__, UW_LOG_LEVEL_DEBUG, (format_), ##__VA_ARGS__) \
  , (status_)

#define UW_STATUS_AND_LOG_WARN(status_, format_, ...)                     \
  UW_LOG(__FILE__, __LINE__, UW_LOG_LEVEL_WARN, (format_), ##__VA_ARGS__) \
  , (status_)

void uw_log_append_(const char* source_path,
                    int line,
                    int level,
                    const char* format,
                    ...) UW_PRINTFLIKE(4, 5);

/** Log raw format without any prefixes. */
void uw_log_simple_(const char* format, ...) UW_PRINTFLIKE(1, 2);

#endif  // LIBUWEAVE_SRC_LOG_H_
