// Copyright 2016 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBUWEAVE_SRC_TRACE_H_
#define LIBUWEAVE_SRC_TRACE_H_

#include "src/log.h"
#include "src/privet_request.h"
#include "src/time.h"
#include "uweave/config.h"
#include "uweave/device.h"
#include "uweave/status.h"

static const size_t kUwTraceDumpMaxEntries = 16;

typedef enum {
  kUwTraceTypeEmpty = 0,
  kUwTraceTypeCallBegin = 1,
  kUwTraceTypeCallEnd = 2,
  kUwTraceTypeAuthResult = 3,
  kUwTraceTypeBleEvent = 4,
  kUwTraceTypeCommandExecute = 5,
  kUwTraceTypeSession = 6,
  kUwTraceTypeFactoryResetBegin = 7,
  kUwTraceTypeFactoryResetEnd = 8,
} UwTraceType;

typedef enum {
  kUwTraceBleEventConnect = 1,
  kUwTraceBleEventDisconnect = 2,
  kUwTraceBleEventDisconnectDrop = 3,
} UwTraceBleEvent;

typedef enum {
  kUwTraceSessionHandshake = 1,
  kUwTraceSessionProcessIn = 2,
  kUwTraceSessionDispatch = 3,
  kUwTraceSessionProcessOut = 4,
  kUwTraceSessionExpiration = 4,
} UwTraceSession;

typedef struct {
  uint8_t mode;
  uint8_t role;
} UwTraceEntryAuth;

typedef struct {
  UwTraceBleEvent event;
  uint8_t state;
} UwTraceEntryBle;

typedef struct {
  uint8_t api_id;
  UwStatus status;
} UwTraceEntryCall;

typedef struct {
  uint16_t trait;
  uint16_t name;
} UwTraceEntryCommandExecute;

typedef struct {
  UwTraceSession type;
  UwStatus status;
} UwTraceEntrySession;

typedef struct {
  uint32_t id;
  time_t timestamp;
  UwTraceType type;
  union {
    UwTraceEntryAuth auth;
    UwTraceEntryBle ble;
    UwTraceEntryCall call;
    UwTraceEntryCommandExecute command_execute;
    UwTraceEntrySession session;
  } u;
} UwTraceEntry;

typedef struct {
  UwTraceEntry entries[UW_TRACE_LOG_ENTRY_COUNT];
  size_t next_index;
  uint32_t next_id;
} UwTraceLog;

void uw_trace_log_get_range_(UwTraceLog* trace_log,
                             size_t* min_id,
                             size_t* max_id);

UwStatus uw_trace_log_encode_to_privet_request_(
    UwTraceLog* trace_log,
    size_t start,
    size_t end,
    UwPrivetRequest* privet_request);

/** Returns the next target trace entry with the id and timestamp populated. */
UwTraceEntry* uw_trace_log_append_(UwDevice* device, UwTraceType type);

static inline void uw_trace_auth_result(UwDevice* device,
                                        uint8_t mode,
                                        uint8_t role) {
  UW_LOG_INFO("auth_result: mode=%d role=%d", mode, role);
  uw_trace_log_append_(device, kUwTraceTypeAuthResult)->u.auth =
      (UwTraceEntryAuth){.mode = mode, .role = role};
}

static inline void uw_trace_ble_event(UwDevice* device,
                                      UwTraceBleEvent event,
                                      uint8_t state) {
  uw_trace_log_append_(device, kUwTraceTypeBleEvent)->u.ble =
      (UwTraceEntryBle){.event = event, .state = state};
}

static inline void uw_trace_call_begin(UwDevice* device, uint16_t api_id) {
  UW_LOG_INFO("call_begin %d\n", api_id);
  uw_trace_log_append_(device, kUwTraceTypeCallBegin)->u.call =
      (UwTraceEntryCall){.api_id = api_id};
}

static inline UwStatus uw_trace_call_end(UwDevice* device,
                                         uint16_t api_id,
                                         UwStatus status) {
  UW_LOG_INFO("call_end %d status=%d\n", api_id, status);
  uw_trace_log_append_(device, kUwTraceTypeCallEnd)->u.call =
      (UwTraceEntryCall){.api_id = api_id, .status = status};
  return status;
}

static inline void uw_trace_command_execute(UwDevice* device,
                                            uint16_t trait,
                                            uint16_t name) {
  uw_trace_log_append_(device, kUwTraceTypeCommandExecute)->u.command_execute =
      (UwTraceEntryCommandExecute){.trait = trait, .name = name};
}

static inline UwStatus uw_trace_session(UwDevice* device,
                                        UwTraceSession type,
                                        UwStatus status) {
  uw_trace_log_append_(device, kUwTraceTypeSession)->u.session =
      (UwTraceEntrySession){.type = type, .status = status};
  return status;
}

#endif  // LIBUWEAVE_SRC_TRACE_H_
