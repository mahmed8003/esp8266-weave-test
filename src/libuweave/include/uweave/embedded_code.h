// Copyright 2016 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBUWEAVE_INCLUDE_UWEAVE_EMBEDDED_CODE_H_
#define LIBUWEAVE_INCLUDE_UWEAVE_EMBEDDED_CODE_H_

#include <stdbool.h>
#include <stddef.h>

typedef enum {
  kUwEmbeddedCodeSourceNone,
  kUwEmbeddedCodeSourceSettings,
  kUwEmbeddedCodeSourceCallback
} UwEmbeddedCodeSource;

/** Callback function that gets an embedded code. */
typedef bool (*UwEmbeddedCodeGet)(char** code);

typedef struct {
  UwEmbeddedCodeSource source;
  union {
    // If the source is kUwEmbeddedCodeSourceSettings, read the code directly
    // from this pointer in the settings.
    char* embedded_code_str;
    // If the source is kUwEmbeddedCodeSourceCallback, call this callback to get
    // the code.
    UwEmbeddedCodeGet callback_func;
  } u;
} UwEmbeddedCode;

#endif  // LIBUWEAVE_INCLUDE_UWEAVE_EMBEDDED_CODE_H_
