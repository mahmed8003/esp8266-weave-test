// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBUWEAVE_INCLUDE_UWEAVE_CONFIG_H_
#define LIBUWEAVE_INCLUDE_UWEAVE_CONFIG_H_

/**
 * UW_LOG_LEVEL is used to configure the verbosity of uWeave logging.
 * Set this to a number between 0 and 4.  Zero turns off logging.  One through
 * four enables logging for ERROR, WARN, INFO, and DEBUG level logs,
 * respectively.
 */
#ifndef UW_LOG_LEVEL
#ifdef NDEBUG
// By default, no logging in production builds.
#define UW_LOG_LEVEL 0
#else
// Info level logging in debug builds.
#define UW_LOG_LEVEL 3
#endif
#endif

/** The max length of a device name in uweave/settings.h. */
#ifndef UW_SETTINGS_MAX_NAME_LENGTH
#define UW_SETTINGS_MAX_NAME_LENGTH 128
#endif

/**
 * The number of application timer slots for systems that have fixed timer
 * mechanisms that are shared between uWeave and the application.
 */
#ifndef UW_TIMER_USER_SLOTS
#define UW_TIMER_USER_SLOTS 0
#endif

/**
 * The size of the tracing buffer in number of records.
 */
#ifndef UW_TRACE_LOG_ENTRY_COUNT
#define UW_TRACE_LOG_ENTRY_COUNT 256
#endif

/** The size of a request buffer on the BLE transport. */
#ifndef UW_BLE_TRANSPORT_REQUEST_BUFFER_SIZE
#define UW_BLE_TRANSPORT_REQUEST_BUFFER_SIZE 512
#endif

/** The size of a reply buffer. */
#ifndef UW_BLE_TRANSPORT_REPLY_BUFFER_SIZE
#define UW_BLE_TRANSPORT_REPLY_BUFFER_SIZE 512
#endif

/** The size of a single BLE packet. */
#ifndef UW_BLE_PACKET_SIZE
#define UW_BLE_PACKET_SIZE 20
#endif

/** The size of a BLE address. */
#ifndef UW_BLE_ADDRESS_SIZE
#define UW_BLE_ADDRESS_SIZE 6
#endif

/**
 * Number of BLE events to buffer. Ideally a power of two. Currently sized to
 * fit one max-command-size set of packets (512 / UW_BLE_PACKET_SIZE).
 */
#ifndef UW_BLE_EVENT_QUEUE_SIZE
#define UW_BLE_EVENT_QUEUE_SIZE 32
#endif

/** Used by the provider to specify the advertising interval. */
#ifndef UW_BLE_ADVERTISING_INTERVAL_MS
#define UW_BLE_ADVERTISING_INTERVAL_MS 500
#endif

/** Used by the provider to set the minimum connection interval. */
#ifndef UW_BLE_MIN_CONNECTION_INTERVAL_MS
#define UW_BLE_MIN_CONNECTION_INTERVAL_MS 7.5
#endif

/** Used by the provider to set the maximum connection interval. */
#ifndef UW_BLE_MAX_CONNECTION_INTERVAL_MS
#define UW_BLE_MAX_CONNECTION_INTERVAL_MS 1000
#endif

/** Used by the provider to set the supervisor timeout (disconnect detect). */
#ifndef UW_BLE_CONN_SUPERVISION_TIMEOUT_MS
#define UW_BLE_CONN_SUPERVISION_TIMEOUT_MS 3000
#endif

/** Alignment for settings in the uwp_storage api. Must be a power of 2. */
#ifndef UW_STORAGE_ALIGNMENT
#define UW_STORAGE_ALIGNMENT 16
#endif

#ifndef UW_ENABLE_MULTIPAIRING_DEFAULT
#define UW_ENABLE_MULTIPAIRING_DEFAULT 0
#endif

#ifndef UW_IDLE_TIMEOUT_SECONDS
#define UW_IDLE_TIMEOUT_SECONDS 7
#endif

#ifndef UW_UNCONFIGURED_IDLE_TIMEOUT_SECONDS
#define UW_UNCONFIGURED_IDLE_TIMEOUT_SECONDS 120
#endif

#endif  // LIBUWEAVE_INCLUDE_UWEAVE_CONFIG_H_
