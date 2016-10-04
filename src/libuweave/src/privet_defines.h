// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBUWEAVE_SRC_PRIVET_DEFINES_H_
#define LIBUWEAVE_SRC_PRIVET_DEFINES_H_

/* Keys used in the root of a request/reply. */
#define PRIVET_RPC_KEY_VERSION 0
#define PRIVET_RPC_KEY_API_ID 1
#define PRIVET_RPC_KEY_REQUEST_ID 2
#define PRIVET_RPC_KEY_ERROR 3
#define PRIVET_RPC_KEY_PARAMS 16
#define PRIVET_RPC_KEY_RESULT 17

/* Keys used in an error object.
 *
 * The error codes correspond to UwStatus values defined in
 * include/uweave/status.h
 */
#define PRIVET_RPC_ERROR_KEY_CODE 4
#define PRIVET_RPC_ERROR_KEY_MESSAGE 5
#define PRIVET_RPC_ERROR_KEY_DATA 6

/* Expected value of the Privet RPC version. */
#define PRIVET_RPC_VALUE_VERSION 2

/* Keys used in the params/result section of a request/reply, respectively */
#define PRIVET_API_KEY_VERSION 0

/* The Privet API version we support and return in replies, not to be confused
 * with Privet RPC. */
#define PRIVET_API_VALUE_VERSION 3

/* Fields used in the param of an auth command. */
#define PRIVET_AUTH_KEY_MODE 0
#define DEPRECATED_PRIVET_AUTH_KEY_ROLE 1
#define PRIVET_AUTH_KEY_AUTH_CODE 2
#define PRIVET_AUTH_KEY_GEN_REFRESH_TOKEN 3

#define PRIVET_AUTH_MODE_VALUE_ANONYMOUS 0
#define PRIVET_AUTH_MODE_VALUE_PAIRING 1
#define PRIVET_AUTH_MODE_VALUE_TOKEN 2

/* Fields used in the response of an auth command. */
#define PRIVET_AUTH_RESPONSE_KEY_ROLE 0
#define PRIVET_AUTH_RESPONSE_KEY_TIME 1
#define PRIVET_AUTH_RESPONSE_KEY_TIME_STATUS 2

/* Fields used in the response of an accesscontrol/claim command. */
#define PRIVET_ACCESS_CONTROL_CLAIM_RESPONSE_KEY_CLIENT_TOKEN 0

/* Fields used in the request of an accesscontrol/claim command. */
#define PRIVET_ACCESS_CONTROL_CONFIRM_REQUEST_KEY_CLIENT_TOKEN 0

/* Fields used in the /debug call. */
#define PRIVET_MAGIC_DEBUG_TRAIT 0xffff

/* Debug trait command names. */
#define PRIVET_DEBUG_NAME_METRICS 0
#define PRIVET_DEBUG_NAME_TRACE_QUERY 1
#define PRIVET_DEBUG_NAME_TRACE_DUMP 2

/* Sub-parameters of the debug call. */
#define PRIVET_DEBUG_KEY_TRACE_DUMP_PARAMETERS 0

/* /debug/trace_dump parameters */
#define PRIVET_DEBUG_TRACE_DUMP_KEY_START 0
#define PRIVET_DEBUG_TRACE_DUMP_KEY_END 1

/* /debug response keys. */
#define PRIVET_DEBUG_RESPONSE_KEY_METRICS 0
#define PRIVET_DEBUG_RESPONSE_KEY_TRACE_QUERY_RESULT 1
#define PRIVET_DEBUG_RESPONSE_KEY_TRACE_DUMP_RESULT 2

/* /debug/metrics entry keys. */
#define PRIVET_DEBUG_METRICS_KEY_GENERATION_ID 0
#define PRIVET_DEBUG_METRICS_KEY_GENERATION_TIMESTAMP 1
#define PRIVET_DEBUG_METRICS_KEY_TIMESTAMP_NOW 2
#define PRIVET_DEBUG_METRICS_KEY_METRICS 3
#define PRIVET_DEBUG_METRICS_KEY_VENDOR_METRICS 4

/* /debug/trace_query result keys. */
#define PRIVET_DEBUG_QUERY_RESULT_KEY_FIRST 0
#define PRIVET_DEBUG_QUERY_RESULT_KEY_LAST 1

/* /debug result keys. */
/* /debug/trace_dump returns an array of entries. */
#define PRIVET_DEBUG_TRACE_DUMP_RESULT_KEY_DUMP 0

/* /debug/trace_dump entries */
#define PRIVET_DEBUG_TRACE_DUMP_RESULT_ENTRY_KEY_TYPE 0
#define PRIVET_DEBUG_TRACE_DUMP_RESULT_ENTRY_KEY_TIMESTAMP 1
#define PRIVET_DEBUG_TRACE_DUMP_RESULT_ENTRY_KEY_ID 2
#define PRIVET_DEBUG_TRACE_DUMP_RESULT_ENTRY_KEY_PARAMS 3

/* param values for the trace entries. */
/* AuthResult */
#define PRIVET_DEBUG_TRACE_DUMP_RESULT_ENTRY_PARAMS_KEY_AUTH_MODE 0
#define PRIVET_DEBUG_TRACE_DUMP_RESULT_ENTRY_PARAMS_KEY_AUTH_ROLE 1
/* Ble */
#define PRIVET_DEBUG_TRACE_DUMP_RESULT_ENTRY_PARAMS_KEY_BLE_EVENT 0
#define PRIVET_DEBUG_TRACE_DUMP_RESULT_ENTRY_PARAMS_KEY_BLE_STATE 1
/* CallBegin/End */
#define PRIVET_DEBUG_TRACE_DUMP_RESULT_ENTRY_PARAMS_KEY_CALL_API_ID 0
#define PRIVET_DEBUG_TRACE_DUMP_RESULT_ENTRY_PARAMS_KEY_CALL_STATUS 1
/* CommandExecute */
#define PRIVET_DEBUG_TRACE_DUMP_RESULT_ENTRY_PARAMS_KEY_COMMAND_EXECUTE_TRAIT 0
#define PRIVET_DEBUG_TRACE_DUMP_RESULT_ENTRY_PARAMS_KEY_COMMAND_EXECUTE_NAME 1
/* Session */
#define PRIVET_DEBUG_TRACE_DUMP_RESULT_ENTRY_PARAMS_KEY_SESSION_TYPE 0
#define PRIVET_DEBUG_TRACE_DUMP_RESULT_ENTRY_PARAMS_KEY_SESSION_STATUS 1

/* Fields used in the param of an execute command. */
#define PRIVET_EXECUTE_KEY_ID 0
#define PRIVET_EXECUTE_KEY_TRAIT 0
#define PRIVET_EXECUTE_KEY_NAME 1
#define PRIVET_EXECUTE_KEY_PARAM 2

/* Fields used in an command object, which is returned in
 * several commands, including /commands/{execute,status,list,cancel}. */
#define PRIVET_COMMAND_OBJ_KEY_API_ID 0
#define PRIVET_COMMAND_OBJ_KEY_PARAMS 1
#define PRIVET_COMMAND_OBJ_KEY_STATE 4
#define PRIVET_COMMAND_OBJ_KEY_COMMAND_ID 5
#define PRIVET_COMMAND_OBJ_KEY_RESULT 17
#define PRIVET_COMMAND_OBJ_KEY_ERROR 18
#define PRIVET_COMMAND_OBJ_KEY_PROGRESS 19

/* Values used in PRIVET_COMMAND_OBJ_KEY_STATE */
#define PRIVET_COMMAND_OBJ_VALUE_STATE_DONE 0
#define PRIVET_COMMAND_OBJ_VALUE_STATE_IN_PROGRESS 1
#define PRIVET_COMMAND_OBJ_VALUE_STATE_ERROR 2
#define PRIVET_COMMAND_OBJ_VALUE_STATE_QUEUED 3
#define PRIVET_COMMAND_OBJ_VALUE_STATE_CANCELLED 4

/* Fields used in the response of an info reply. */
#define PRIVET_INFO_KEY_VERSION 0
#define PRIVET_INFO_KEY_AUTH 2
#define PRIVET_INFO_KEY_MODEL_MANIFEST_ID 3
#define PRIVET_INFO_KEY_DEVICE_ID 4
#define PRIVET_INFO_KEY_NAME 6
#define PRIVET_INFO_KEY_TIMESTAMP 10
#define PRIVET_INFO_KEY_TIME_STATUS 11
#define PRIVET_INFO_KEY_BUILD 21

/* Fields used in the authentication object in the response of an info reply. */
#define PRIVET_INFO_AUTH_KEY_MODE 0
#define PRIVET_INFO_AUTH_KEY_PAIRING 1
#define PRIVET_INFO_AUTH_KEY_CRYPTO 2
#define PRIVET_INFO_AUTH_VALUE_PAIRING_PIN 0
#define PRIVET_INFO_AUTH_VALUE_PAIRING_EMBEDDED 1
#define PRIVET_INFO_AUTH_VALUE_CRYPTO_SPAKE_P224 0

/* Values used for the time status of an info reply. */
#define PRIVET_INFO_TIME_STATUS_VALUE_OK 0
#define PRIVET_INFO_TIME_STATUS_VALUE_DEGRADED 1
#define PRIVET_INFO_TIME_STATUS_VALUE_INVALID 2

/* Fields used in a /pairing/start request. */
#define PRIVET_PAIRING_START_KEY_PAIRING 0
#define PRIVET_PAIRING_START_KEY_CRYPTO 1

/* Fields used in a /pairing/start reply. */
#define PRIVET_PAIRING_START_KEY_SESSION_ID 0
#define PRIVET_PAIRING_START_KEY_DEVICE_COMMITMENT 1

/* Fields used in a /pairing/confirm request. */
#define PRIVET_PAIRING_CONFIRM_KEY_SESSION_ID 0
#define PRIVET_PAIRING_CONFIRM_KEY_CLIENT_COMMITMENT 1
#define PRIVET_PAIRING_CONFIRM_KEY_TIMESTAMP 2
#define PRIVET_PAIRING_CONFIRM_TIMESTAMP_MAP_KEY_TIMESTAMP 0

/* Fields used in a /pairing/confirm reply. */
#define PRIVET_PAIRING_CONFIRM_KEY_ENCRYPTED_TOKENS 0
#define PRIVET_PAIRING_CONFIRM_KEY_PAIRING_CAT_MACAROON 0
#define PRIVET_PAIRING_CONFIRM_KEY_SAT_MACAROON 1

/* Fields used in the response of a state reply. */
#define PRIVET_STATE_KEY_FINGERPRINT 0
#define PRIVET_STATE_KEY_COMPONENTS 1

#define PRIVET_STATE_KEY_COMPONENT_STATE 0

/* Fields used in the request/response for setup/? commands. */
#define PRIVET_SETUP_KEY_VERSION 0
#define PRIVET_SETUP_KEY_NAME 1
#define PRIVET_SETUP_KEY_TIMESTAMP 4

#define PRIVET_SETUP_NAME_MAX_LENGTH 32

#endif  // LIBUWEAVE_SRC_PRIVET_DEFINES_H_
