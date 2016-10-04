// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBUWEAVE_INCLUDE_UWEAVE_DEVICE_H_
#define LIBUWEAVE_INCLUDE_UWEAVE_DEVICE_H_

#include "uweave/base.h"
#include "uweave/buffer.h"
#include "uweave/command.h"
#include "uweave/command_list.h"
#include "uweave/counters.h"
#include "uweave/settings.h"
#include "uweave/state_reply.h"
#include "uweave/status.h"

/**
 * Public but opaque UwDevice structure.
 */
typedef struct UwDevice_ UwDevice;

typedef enum {
  kUwDeviceWorkStateIdle,
  kUwDeviceWorkStateBusy
} UwDeviceWorkState;

/**
 * Returns the size of a UwDevice struct, so the client can allocate an
 * appropriate buffer.
 */
size_t uw_device_sizeof();

/**
 * A callback that a device should implement to respond to a /commands/execute
 * API call.
 *
 * Application-level errors should be returned via the uw_command_reply_error
 * interface.  The result should be kUwStatusSuccess unless an encoding or
 * permission error occurs.
 */
typedef UwStatus (*UwDeviceCommandExecuteHandler)(UwDevice* device,
                                                  UwCommand* command);

/**
 * A notification that work is available to process and the processor should
 * execute the main loop/wake up.  This is likely to be called from an interrupt
 * handler and should be kept brief.
 */
typedef void (*UwDeviceNotifyWorkHandler)(UwDevice* device);

/**
 * A callback that a device should implement to respond to a /state API call.
 */
typedef bool (*UwDeviceStateHandler)(UwDevice* device,
                                     UwStateReply* state_reply);

typedef struct UwDeviceHandlers_ {
  UwDeviceCommandExecuteHandler execute_handler;
  UwDeviceNotifyWorkHandler notify_handler;
  UwDeviceStateHandler state_handler;
} UwDeviceHandlers;

/**
 * Initialize a device struct provided by the caller.
 *
 * @param device The device to initialize.
 * @param settings A settings object for the new device.
 * @param device_handlers A collection of function pointers to handle callbacks.
 * @param command_list The command buffers used for execution replies and
 *        deferred commands.
 * @param counter_set The nullable set of counters to track device and
 *        application metrics.
 */
void uw_device_init(UwDevice* device,
                    UwSettings* settings,
                    UwDeviceHandlers* device_handlers,
                    UwCommandList* command_list,
                    UwCounterSet* counter_set);

/**
 * Get the UwSettings struct from a device.
 */
UwSettings* uw_device_get_settings(UwDevice* device);

/**
 * Returns true if the device has successfully completed the setup process.
 */
bool uw_device_is_setup(UwDevice* device);

/**
 * Set the optional user-data pointer on the device.
 */
void uw_device_set_context(UwDevice* device, void* user_context);

/**
 * Get the optional user-data pointer on the device.
 */
void* uw_device_get_context(UwDevice* device);

/**
 * Start uWeave.
 *
 * The host application should call this when uWeave setup is done to get things
 * started.
 */
void uw_device_start(UwDevice* device);

/**
 * Service uWeave.
 *
 * The host application should call this as often as possible after starting
 * the device.
 *
 * Returns a UwDeviceWorkState to indicate whether the loop is idle and the
 * device can be put in low power mode or more work is available.
 */
UwDeviceWorkState uw_device_handle_events(UwDevice* device);

/**
 * Notify the device that new work is available.
 *
 * When running in low power mode, the host application is responsible for
 * managing the device's power mode. This call can be used from interrupt
 * handlers or provider code to signal the host application through the
 * UwDeviceNotifyWorkHandler in the UwDeviceHandlers struct specified in
 * uw_device_init.
 */
void uw_device_notify_work(UwDevice* device);

/**
 * Stop uWeave.
 *
 * The host application may call this to shut down uWeave on the device.
 * TODO(rginda): Is it ok to call uw_device_start again later?
 */
void uw_device_stop(UwDevice* device);

/**
 * Reset identifiers and crypto keys for a fresh pairing.
 */
void uw_device_factory_reset(UwDevice* device);

void uw_device_increment_app_counter(UwDevice* device, UwCounterId id);
uint32_t uw_device_get_app_counter(UwDevice* device, UwCounterId id);

#endif  // LIBUWEAVE_INCLUDE_UWEAVE_DEVICE_H_
