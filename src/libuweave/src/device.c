// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/device.h"

#include "src/access_control_request.h"
#include "src/auth_request.h"
#include "src/ble_advertising.h"
#include "src/command.h"
#include "src/command_list.h"
#include "src/counters.h"
#include "src/debug_request.h"
#include "src/execute_request.h"
#include "src/info_request.h"
#include "src/log.h"
#include "src/pairing_request.h"
#include "src/privet_defines.h"
#include "src/privet_request.h"
#include "src/service.h"
#include "src/session.h"
#include "src/settings.h"
#include "src/setup_request.h"
#include "src/state_reply.h"
#include "src/trace.h"
#include "src/uw_assert.h"
#include "uweave/config.h"
#include "uweave/provider/ble.h"
#include "uweave/provider/crypto.h"
#include "uweave/provider/storage.h"
#include "uweave/provider/time.h"
#include "uweave/settings.h"
#include "uweave/status.h"

size_t uw_device_sizeof() {
  return sizeof(UwDevice);
}

// TODO(ederle) - We need to return a status/error code from device init.
void uw_device_init(UwDevice* device,
                    UwSettings* settings,
                    UwDeviceHandlers* device_handlers,
                    UwCommandList* command_list,
                    UwCounterSet* counter_set) {
  UW_LOG_DEBUG("Initializing device\n");

  *device = (UwDevice){.settings = settings,
                       .device_handlers = device_handlers,
                       .work_state = kUwDeviceWorkStateIdle,
                       .command_list = command_list,
                       .counter_set = counter_set};

  uwp_crypto_init();
  uwp_time_init();
  uwp_storage_init();
  uwp_ble_init();

  uw_settings_read_from_storage_(settings);

  uw_device_crypto_init_(&device->device_crypto);

  if (device->settings->supported_pairing_types == 0) {
    UW_LOG_WARN("Device has no supported pairing types.\n");
  }

  if (counter_set != NULL) {
    uw_counter_set_read_from_storage_(counter_set);
  }
}

bool uw_device_is_setup(UwDevice* device) {
  // TODO(rginda): Also check that /setup completed successfully.
  return device->device_crypto.has_client_authz_key;
}

void uw_device_register_service_(UwDevice* device, UwService* service) {
  if (device->first_service == NULL) {
    device->first_service = service;
  } else {
    uw_service_register_next_(device->first_service, service);
  }
}

UwSettings* uw_device_get_settings(UwDevice* device) {
  return device->settings;
}

void uw_device_set_context(UwDevice* device, void* user_context) {
  device->context = user_context;
}

void* uw_device_get_context(UwDevice* device) {
  return device->context;
}

void uw_device_start(UwDevice* device) {
  UW_LOG_INFO("Starting device: %s\n", device->settings->name);
  if (device->first_service != NULL) {
    uw_service_start_(device->first_service);
  }
}

UwDeviceWorkState uw_device_handle_events(UwDevice* device) {
  bool work_remaining = false;
  if (device->first_service != NULL) {
    work_remaining |= uw_service_handle_events_(device->first_service);
  }
  if (device->counter_set != NULL) {
    uw_counter_set_try_coalesce_(device->counter_set);
  }
  device->work_state =
      work_remaining ? kUwDeviceWorkStateBusy : kUwDeviceWorkStateIdle;
  return device->work_state;
}

void uw_device_stop(UwDevice* device) {
  UW_LOG_INFO("Stopping device: %s\n", device->settings->name);
  if (device->first_service != NULL) {
    uw_service_stop_(device->first_service);
  }
}

void uw_device_factory_reset(UwDevice* device) {
  uw_trace_log_append_(device, kUwTraceTypeFactoryResetBegin);
  UW_LOG_INFO("Factory resetting the device\n");
  uw_device_crypto_reset_(&device->device_crypto);
  uw_ble_advertising_update_data_(device);
  UW_LOG_INFO("Factory reset complete\n");
  uw_trace_log_append_(device, kUwTraceTypeFactoryResetEnd);
  if (device->counter_set) {
    uw_counter_set_increment_uw_counter_(device->counter_set,
                                         kUwInternalCounterFactoryReset);
  }
}

void uw_device_increment_app_counter(UwDevice* device, UwCounterId id) {
  if (device->counter_set == NULL) {
    assert(false);
    return;
  }
  uw_counter_set_increment_app_counter_(device->counter_set, id);
}

UwCounterValue uw_device_get_app_counter(UwDevice* device, UwCounterId id) {
  if (device->counter_set == NULL) {
    assert(false);
    return kUwCounterValueInvalid;
  }
  return uw_counter_set_get_app_counter_(device->counter_set, id);
}

void uw_device_increment_uw_counter_(UwDevice* device, UwCounterId id) {
  if (device->counter_set == NULL) {
    assert(false);
    return;
  }
  uw_counter_set_increment_uw_counter_(device->counter_set, id);
}

UwCounterValue uw_device_get_uw_counter_(UwDevice* device, UwCounterId id) {
  if (device->counter_set == NULL) {
    assert(false);
    return kUwCounterValueInvalid;
  }
  return uw_counter_set_get_uw_counter_(device->counter_set, id);
}

UwStatus uw_device_message_exchange_(UwDevice* device,
                                     UwSession* session,
                                     UwBuffer* request,
                                     UwBuffer* reply) {
  UwPrivetRequest privet_request = {};
  uw_privet_request_init_(&privet_request, request, reply, session);
  return uw_device_dispatch_request_(device, &privet_request);
}

UwStatus uw_device_dispatch_request_(UwDevice* device,
                                     UwPrivetRequest* privet_request) {
  uw_device_increment_uw_counter_(device, kUwInternalCounterPrivetDispatch);

  if (!uw_privet_request_parse_(privet_request)) {
    uw_privet_request_reply_privet_error_(privet_request,
                                          kUwStatusPrivetInvalidParam,
                                          /* error message */ NULL,
                                          /* error data */ NULL);
    return UW_STATUS_AND_LOG_WARN(kUwStatusPrivetInvalidParam,
                                  "Error parsing privet request\n");
  }

  UwStatus session_expiration_status =
      uw_session_check_expiration_(privet_request->session);
  if (!uw_status_is_success(session_expiration_status)) {
    uw_privet_request_reply_privet_error_(privet_request,
                                          session_expiration_status,
                                          /* error message */ NULL,
                                          /* error data */ NULL);
    return UW_STATUS_AND_LOG_WARN(session_expiration_status,
                                  "Session expiration failure: %d\n",
                                  session_expiration_status);
  }

  UwPrivetRequestApiId api_id = uw_privet_request_get_api_id_(privet_request);

  uw_trace_call_begin(device, api_id);
  // Breaking from the loop results in a successful completion.
  switch (api_id) {
    case kUwPrivetRequestApiIdInfo: {
      // Call is not privileged, but returns filtered results depending on the
      // role.
      uw_info_request_set_info_(privet_request, device);
      break;
    }
    case kUwPrivetRequestApiIdPairingStart: {
      UwStatus pairing_start_status = uw_pairing_start_reply_(privet_request);
      if (!uw_status_is_success(pairing_start_status)) {
        uw_privet_request_reply_privet_error_(privet_request,
                                              pairing_start_status,
                                              /* error message */ NULL,
                                              /* error data */ NULL);
        // TODO(jmccullough): Resolve the fact that this will disconnect before
        // sending the error by cleaning up and preserving the session, or
        // forcing a close after the response.
        return uw_trace_call_end(device, api_id, pairing_start_status);
      }
      break;
    }
    case kUwPrivetRequestApiIdPairingConfirm: {
      UwStatus pairing_confirm_status =
          uw_pairing_confirm_reply_(privet_request, &device->device_crypto);
      if (!uw_status_is_success(pairing_confirm_status)) {
        uw_privet_request_reply_privet_error_(privet_request,
                                              pairing_confirm_status,
                                              /* error message */ NULL,
                                              /* error data */ NULL);
        // TODO(jmccullough): Resolve the fact that this will disconnect before
        // sending the error by cleaning up and preserving the session, or
        // forcing a close after the response.
        return uw_trace_call_end(device, api_id, pairing_confirm_status);
      }
      break;
    }
    case kUwPrivetRequestApiIdAuth: {
      // Call is not privileged.
      UwStatus auth_status = uw_auth_request_handler_(device, privet_request);
      if (!uw_status_is_success(auth_status)) {
        uw_trace_call_end(device, api_id, auth_status);
        return uw_privet_request_reply_privet_error_(privet_request,
                                                     auth_status,
                                                     /* error message */ NULL,
                                                     /* error data */ NULL);
      }
      break;
    }
    case kUwPrivetRequestApiIdState: {
      if (!uw_privet_request_is_secure(privet_request)) {
        return uw_trace_call_end(device, api_id, kUwStatusEncryptionRequired);
      }

      // Require viewer role
      if (!uw_status_is_success(
              uw_privet_request_has_required_role_or_reply_error_(
                  privet_request, kUwRoleViewer))) {
        break;
      }

      if (device->device_handlers == NULL ||
          device->device_handlers->state_handler == NULL) {
        UW_LOG_WARN("No state handler defined.\n");
        break;
      }

      UwStateReply state_reply = {};
      uw_state_reply_init_(&state_reply, privet_request);
      device->device_handlers->state_handler(device, &state_reply);
      break;
    }
    case kUwPrivetRequestApiIdExecute: {
      // Parse the request, then check for the debug trait, then try user
      // handlers.
      UwExecuteRequest execute_request = {};
      UwStatus request_status =
          uw_execute_request_init_(&execute_request, privet_request);
      if (!uw_status_is_success(request_status)) {
        uw_trace_call_end(device, api_id, request_status);
        return uw_privet_request_reply_privet_error_(privet_request,
                                                     request_status,
                                                     /* error message */ NULL,
                                                     /* error data */ NULL);
      }

      uw_trace_command_execute(device, execute_request.trait,
                               execute_request.name);

      // Special case for the debug trait.
      if (execute_request.trait == PRIVET_MAGIC_DEBUG_TRAIT) {
        UwStatus debug_status =
            uw_debug_command_request_(device, &execute_request);
        if (!uw_status_is_success(debug_status)) {
          uw_trace_call_end(device, api_id, debug_status);
          return uw_privet_request_reply_privet_error_(
              privet_request, debug_status, NULL, NULL);
        }
        break;
      }

      // Require a secure connection.  It is up to the app handler to enforce
      // roles.
      if (!uw_privet_request_is_secure(privet_request)) {
        return uw_trace_call_end(device, api_id, kUwStatusEncryptionRequired);
      }

      if (device->device_handlers == NULL ||
          device->device_handlers->execute_handler == NULL) {
        UW_LOG_WARN("No execute handler defined.\n");
        break;
      }

      UwCommand* command =
          uw_command_list_get_free_or_evict_(device->command_list);

      if (command == NULL) {
        return uw_trace_call_end(device, api_id,
                                 kUwStatusCommandNoAvailableBuffers);
      }

      uw_command_reset_with_request_(command, &execute_request);

      UwStatus execute_status =
          device->device_handlers->execute_handler(device, command);
      if (!uw_status_is_success(execute_status)) {
        uw_command_mark_error_(command);
        uw_trace_call_end(device, api_id, execute_status);
        return uw_privet_request_reply_privet_error_(privet_request,
                                                     execute_status,
                                                     /* error message */ NULL,
                                                     /* error data */ NULL);
      } else {
        // TODO(jmccullough): Deal with deferral.
        UwValue reply_value = uw_command_reply_value_(command);
        UwStatus reply_status =
            uw_privet_request_reply_privet_ok_(privet_request, &reply_value);
        if (uw_status_is_success(reply_status)) {
          uw_command_mark_done_(command);
        } else {
          uw_command_mark_error_(command);
        }
        break;
      }
      break;
    }
    case kUwPrivetRequestApiIdSetup: {
      if (!uw_privet_request_is_secure(privet_request)) {
        return uw_trace_call_end(device, api_id, kUwStatusEncryptionRequired);
      }
      // Require manager role for setup
      UwStatus role_status =
          uw_privet_request_has_required_role_or_reply_error_(privet_request,
                                                              kUwRoleManager);
      if (uw_status_is_success(role_status)) {
        UwStatus setup_status =
            uw_setup_request_(privet_request, device->settings);
        if (!uw_status_is_success(setup_status)) {
          uw_trace_call_end(device, api_id, setup_status);
          // Ultimate result is serializing the error message.
          return uw_privet_request_reply_privet_error_(privet_request,
                                                       setup_status,
                                                       /* error_message */ NULL,
                                                       /* data */ NULL);
        }
        break;
      }
      break;
    }
    case kUwPrivetRequestApiIdAccessControlClaim: {
      UwStatus claim_status =
          uw_access_control_request_claim_(device, privet_request);
      if (!uw_status_is_success(claim_status)) {
        return uw_privet_request_reply_privet_error_(
            privet_request, claim_status, /* error message */ NULL,
            /* error data */ NULL);
      }
      break;
    }
    case kUwPrivetRequestApiIdAccessControlConfirm: {
      UwStatus confirm_status =
          uw_access_control_request_confirm_(device, privet_request);
      if (!uw_status_is_success(confirm_status)) {
        return uw_privet_request_reply_privet_error_(
            privet_request, confirm_status, /* error message */ NULL,
            /* error data */ NULL);
      }
      break;
    }
    default: {
      uw_privet_request_reply_privet_error_(privet_request,
                                            kUwStatusPrivetNotFound,
                                            /* error message */ NULL,
                                            /* error data */ NULL);
      return uw_trace_call_end(device, api_id, kUwStatusPrivetNotFound);
    }
  }

  return uw_trace_call_end(device, api_id, kUwStatusSuccess);
}

void uw_device_notify_work(UwDevice* device) {
  // Only notify once per idle->work transition.
  if (device->work_state == kUwDeviceWorkStateBusy) {
    return;
  }
  device->work_state = kUwDeviceWorkStateBusy;
  if (device->device_handlers == NULL ||
      device->device_handlers->notify_handler == NULL) {
    return;
  }
  device->device_handlers->notify_handler(device);
}
