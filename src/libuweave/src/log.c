// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdarg.h>
#include <string.h>

#include "uweave/base.h"
#include "uweave/provider/log.h"

#include "src/log.h"

/**
 * Helper to convert from vararg signature to vprintf.
 */
void call_vprintf_(const char* format, ...) UW_PRINTFLIKE(1, 2);
void call_vprintf_(const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  uwp_log_vprintf(format, ap);
  va_end(ap);
}

void uw_log_append_(const char* source_path,
                    int line,
                    int level,
                    const char* format,
                    ...) {
  const char* basename = strrchr(source_path, '/');
  if (basename != NULL) {
    basename = basename + sizeof(char);
  } else {
    basename = source_path;
  }

  char level_name;
  switch (level) {
    case UW_LOG_LEVEL_ERROR:
      level_name = 'E';
      break;

    case UW_LOG_LEVEL_WARN:
      level_name = 'W';
      break;

    case UW_LOG_LEVEL_INFO:
      level_name = 'I';
      break;

    case UW_LOG_LEVEL_DEBUG:
      level_name = 'D';
      break;

    default:
      level_name = 'L';
  }

  call_vprintf_("[%c %s:%i] ", level_name, basename, line);

  va_list ap;
  va_start(ap, format);
  uwp_log_vprintf(format, ap);
  va_end(ap);
}

void uw_log_simple_(const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  uwp_log_vprintf(format, ap);
  va_end(ap);
}
