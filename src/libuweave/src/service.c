// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/service.h"

#include "src/log.h"
#include "uweave/base.h"

void uw_service_init_(UwService* service,
                      UwServiceHandler start_handler,
                      UwServiceHandler event_handler,
                      UwServiceHandler stop_handler,
                      void* service_data) {
  service->start_handler = start_handler;
  service->event_handler = event_handler;
  service->stop_handler = stop_handler;
  service->service_data = service_data;
  service->next_service = NULL;
}

void uw_service_register_next_(UwService* service, UwService* next_service) {
  UwService* current_service = service;
  while (current_service->next_service != NULL) {
    current_service = current_service->next_service;
  }

  current_service->next_service = next_service;
}

void uw_service_start_(UwService* service) {
  if (service->start_handler != NULL)
    service->start_handler(service->service_data);

  if (service->next_service != NULL) {
    uw_service_start_(service->next_service);
  }
}

bool uw_service_handle_events_(UwService* service) {
  bool work_remaining = false;
  if (service->event_handler != NULL) {
    work_remaining |= service->event_handler(service->service_data);
  }

  if (service->next_service != NULL) {
    work_remaining |= uw_service_handle_events_(service->next_service);
  }
  return work_remaining;
}

void uw_service_stop_(UwService* service) {
  if (service->stop_handler != NULL)
    service->stop_handler(service->service_data);

  if (service->next_service != NULL) {
    uw_service_stop_(service->next_service);
  }
}
