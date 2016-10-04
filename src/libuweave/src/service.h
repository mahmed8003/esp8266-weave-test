// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBUWEAVE_SRC_SERVICE_H_
#define LIBUWEAVE_SRC_SERVICE_H_

#include "uweave/base.h"

typedef bool (*UwServiceHandler)(void* data);

typedef struct UwService_ {
  UwServiceHandler start_handler;
  UwServiceHandler event_handler;
  UwServiceHandler stop_handler;
  void* service_data;
  struct UwService_* next_service;
} UwService;

void uw_service_init_(UwService* service,
                      UwServiceHandler start_handler,
                      UwServiceHandler event_handler,
                      UwServiceHandler stop_handler,
                      void* service_data);

UwServiceHandler uw_service_get_next_(UwService* service);

void uw_service_register_next_(UwService* service, UwService* next_service);

void uw_service_start_(UwService* service);

bool uw_service_handle_events_(UwService* service);

void uw_service_stop_(UwService* service);

#endif  // LIBUWEAVE_SRC_SERVICE_H_
