// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/ble_transport.h"

#include <string.h>

#include "src/ble_advertising.h"
#include "src/counters.h"
#include "src/device_channel.h"
#include "src/message_in.h"
#include "src/message_out.h"
#include "src/device.h"
#include "src/log.h"
#include "src/privet_request.h"
#include "src/service.h"
#include "src/session.h"
#include "src/time.h"
#include "src/uw_assert.h"
#include "uweave/config.h"
#include "uweave/gatt.h"
#include "uweave/provider/ble.h"
#include "uweave/status.h"

static const uint16_t kUwCharacteristicSize = 20;

typedef enum {
  kUwBleTransportStateDisconnected = 0,
  kUwBleTransportStateConnected,
} UwBleTransportState;

struct UwBleTransport_ {
  UwService service;
  UwDevice* device;

  UwSession session;

  UwBleTransportState connection_state;
  UwBleOpaqueConnectionHandle opaque_connection_handle;

  time_t last_activity_time;

  // Channel resources.
  UwDeviceChannel device_channel;
  uint8_t read_data[UW_BLE_TRANSPORT_REQUEST_BUFFER_SIZE];
  uint8_t write_data[UW_BLE_TRANSPORT_REQUEST_BUFFER_SIZE];
  UwBuffer read_buffer;
  UwBuffer write_buffer;
};

static bool handshake_exchange_handler_(void* data,
                                        UwBuffer* connection_request_data,
                                        UwBuffer* connection_confirm_data);

static bool connection_reset_handler_(void* data);

static bool service_start_handler_();
static bool service_stop_handler_();
static bool service_event_handler_();

static bool create_service_(UwBleTransport* transport);
static bool start_advertising_();

bool uw_ble_transport_init(UwBleTransport* transport, UwDevice* device) {
  assert(transport != NULL && device != NULL);

  memset(transport, 0, sizeof(UwBleTransport));
  transport->device = device;

  // Inbound packets will be assembled into a message in this buffer.
  uw_buffer_init(&transport->read_buffer, transport->read_data,
                 sizeof(transport->read_data));

  // Outbound packets will come from the message stored in this buffer.
  uw_buffer_init(&transport->write_buffer, transport->write_data,
                 sizeof(transport->write_data));

  uw_session_init_(&transport->session, device);

  // The connection request can negotiate message size smaller than
  // UW_BLE_PACKET_SIZE, but never larger.
  uw_device_channel_init_(
      &transport->device_channel,
      (UwDeviceChannelHandshakeConfig){.handler = handshake_exchange_handler_,
                                       .data = (void*)transport},
      (UwDeviceChannelConnectionResetConfig){
          .handler = connection_reset_handler_, .data = (void*)transport},
      &transport->read_buffer, &transport->write_buffer, UW_BLE_PACKET_SIZE);

  uw_service_init_(&transport->service, service_start_handler_,
                   service_event_handler_, service_stop_handler_, transport);

  uw_device_register_service_(transport->device, &transport->service);

  UwSettings* settings = uw_device_get_settings(transport->device);
  settings->supports_ble_40 = true;

  return true;
}

void uw_ble_transport_notify_work(UwBleTransport* ble_transport) {
  if (ble_transport->device != NULL) {
    uw_device_notify_work(ble_transport->device);
  }
}

void uw_ble_transport_notify_activity(UwBleTransport* ble_transport) {
  ble_transport->last_activity_time = uw_time_get_uptime_seconds_();
}

size_t uw_ble_transport_sizeof() {
  return sizeof(UwBleTransport);
}

UwSession* uw_ble_transport_get_session_(UwBleTransport* ble_transport) {
  return &ble_transport->session;
}

static bool handshake_exchange_handler_(void* data,
                                        UwBuffer* request,
                                        UwBuffer* reply) {
  UwBleTransport* ble_transport = (UwBleTransport*)data;
  return uw_session_handshake_exchange_(&ble_transport->session, request,
                                        reply);
}

static bool connection_reset_handler_(void* data) {
  UwBleTransport* ble_transport = (UwBleTransport*)data;
  // Clear the data in the session.
  uw_session_start_valid_(uw_ble_transport_get_session_(ble_transport));
  return true;
}

static bool service_start_handler_(UwBleTransport* transport) {
  UW_LOG_INFO("Starting BLE transport\n");
  create_service_(transport);
  start_advertising_(transport);
  return true;
}

static bool service_stop_handler_(UwBleTransport* transport) {
  UW_LOG_INFO("Stopping BLE transport\n");
  return true;
}

static void connect_(UwBleTransport* transport,
                     UwBleOpaqueConnectionHandle connection_handle) {
  uw_session_start_valid_(&transport->session);
  transport->opaque_connection_handle = connection_handle;
  transport->connection_state = kUwBleTransportStateConnected;
  uw_trace_ble_event(transport->device, kUwTraceBleEventConnect, 0);
  uw_device_increment_uw_counter_(transport->device,
                                  kUwInternalCounterBleConnect);
}

static void disconnect_(UwBleTransport* transport) {
  bool is_connected =
      (transport->connection_state == kUwBleTransportStateConnected);

  if (is_connected) {
    uwp_ble_disconnect(transport->opaque_connection_handle);
  }

  uw_trace_ble_event(transport->device, kUwTraceBleEventDisconnect,
                     is_connected);
  uw_device_increment_uw_counter_(transport->device,
                                  kUwInternalCounterBleDisconnect);

  uw_device_channel_reset_(&transport->device_channel);
  // uw_session_invalidate must follow uw_device_channel reset because
  // device_channel_reset starts a valid session.
  uw_session_invalidate_(&transport->session);
  transport->connection_state = kUwBleTransportStateDisconnected;
}

typedef enum {
  kHandlerStateError = 0,
  kHandlerStateComplete = 1,
  kHandlerStateInProgress = 2,
  kHandlerStateWait = 3,
  kHandlerStateDisconnect = 4,
} HandlerState;

/**
 * Attempts to send a packet if there is space available.
 *
 * Returns kHandlerStateInProgress if there is more data to send.
 * Returns kHandlerStateWait if we should wait to send more data.
 * Returns kHandlerStateComplete when the message is complete and the channel
 * can be reset.
 * Returns kHandlerStateError on error if the value should be reset.
 */
static HandlerState try_to_send_packet_(UwChannel* channel,
                                        UwBleTransport* transport) {
  if (!uwp_ble_can_write_packet()) {
    return kHandlerStateWait;
  }

  UwBleEvent event = {};
  UwBuffer packet_buffer;
  uw_buffer_init(&packet_buffer, event.packet.data, sizeof(event.packet.data));
  if (!uw_channel_get_next_packet_out_(channel, &packet_buffer)) {
    UW_LOG_WARN("Failed to get next packet\n");
    return kHandlerStateError;
  }

  // Send next packet.
  event.packet.data_length = uw_buffer_get_length(&packet_buffer);
  event.connection_handle = transport->opaque_connection_handle;
  if (!uwp_ble_write_packet(&event)) {
    UW_LOG_WARN("Failed to write packet\n");
    return kHandlerStateError;
  }

  UwMessageState out_state = uw_channel_get_out_state_(channel);
  return (out_state == kUwMessageStateBusy) ? kHandlerStateInProgress
                                            : kHandlerStateComplete;
}

static void handle_message_exchange_(UwBleTransport* ble_transport) {
  UwChannel* channel =
      uw_device_channel_get_channel_(&ble_transport->device_channel);
  UwMessageIn* message_in = uw_channel_get_message_in_(channel);
  UwBuffer* buffer_in = uw_message_in_get_buffer_(message_in);

  UwMessageOut* message_out = uw_channel_get_message_out_(channel);
  UwBuffer* buffer_out = uw_message_out_get_buffer_(message_out);

  uw_message_out_start_(message_out, kUwMessageTypeData);

  UwStatus status = uw_session_message_exchange_(&ble_transport->session,
                                                 buffer_in, buffer_out);

  if (!uw_status_is_success(status)) {
    UW_LOG_ERROR("Error exchanging message: %d. Disconnecting.\n",
                 status);
    disconnect_(ble_transport);
    return;
  }

  if (uw_buffer_get_length(buffer_out) == 0) {
    uw_message_out_discard_(message_out);
    UW_LOG_INFO("No response data returned from command, status: %d.\n",
                status);
    return;
  }

  uw_message_out_ready_(message_out);
}

/**
 * Attempts to read an event from the provider.
 *
 * Returns kHandlerStateInProgress if a packet was read.
 * Returns kHandlerStateWait if we should sleep until a new packet notification.
 * Returns kHandlerStateComplete if all processing for this operation has
 * completed and the channel can be reset.
 * Returns kHandlerStateDisconnect if a disconnect event was observed.
 * Returns kHandlerStateError on error if the connection should be reset.
 */
static HandlerState try_to_read_event_(UwDeviceChannel* device_channel,
                                       UwBleTransport* transport) {
  UwChannel* channel = uw_device_channel_get_channel_(device_channel);
  UwBleEvent event = {};
  // Read events until we are connected and have data to pass on.
  while (true) {
    if (!uwp_ble_read_event(&event)) {
      return kHandlerStateWait;
    }
    if (transport->connection_state == kUwBleTransportStateDisconnected) {
      if (event.event_type != kUwBleEventTypeConnection) {
        // Skip packets until a connection starts.
        UW_LOG_WARN("Dropping packet while in disconnected state\n");
        uw_trace_ble_event(transport->device, kUwTraceBleEventDisconnectDrop,
                           0);
        continue;
      }
      connect_(transport, event.connection_handle);
    } else if (transport->connection_state == kUwBleTransportStateConnected) {
      if (event.event_type == kUwBleEventTypeData) {
        if (event.connection_handle != transport->opaque_connection_handle) {
          UW_LOG_WARN("Dropping packet with mismatched handle [%d != %d]\n",
                      event.connection_handle,
                      transport->opaque_connection_handle);
          continue;
        }
        // Continue to the handler.
        break;
      } else if (event.event_type == kUwBleEventTypeDisconnection) {
        return kHandlerStateDisconnect;
      } else {
        return kHandlerStateError;
      }
    }
  }

  if (uw_channel_get_in_state_(channel) == kUwMessageStateEmpty) {
    transport->opaque_connection_handle = event.connection_handle;
  } else if (transport->opaque_connection_handle != event.connection_handle) {
    UW_LOG_WARN("Unexpected session handle %d\n", event.connection_handle);
    return kHandlerStateWait;
  }

  UwBuffer packet_buffer;
  uw_buffer_init(&packet_buffer, event.packet.data, sizeof(event.packet.data));
  uw_buffer_set_length_(&packet_buffer, event.packet.data_length);

  if (!uw_channel_append_packet_in_(channel, &packet_buffer)) {
    return kHandlerStateError;
  }

  UwMessageState in_state = uw_channel_get_in_state_(channel);
  if (in_state == kUwMessageStateError) {
    return kHandlerStateError;
  }

  if (in_state == kUwMessageStateBusy) {
    return kHandlerStateInProgress;
  }

  if (in_state == kUwMessageStateComplete &&
      uw_message_in_get_type_(&channel->message_in) == kUwMessageTypeData) {
    handle_message_exchange_(transport);
  }

  UwMessageState out_state = uw_channel_get_out_state_(channel);
  if (out_state == kUwMessageStateBusy) {
    // There's a pending reply.
    return kHandlerStateInProgress;
  }

  return kHandlerStateComplete;
}

static time_t idle_time_(UwBleTransport* ble_transport) {
  return uw_time_get_uptime_seconds_() - ble_transport->last_activity_time;
}

static bool service_event_handler_(UwBleTransport* transport) {
  UwDeviceChannel* device_channel = &transport->device_channel;
  UwChannel* channel = uw_device_channel_get_channel_(device_channel);

  UwMessageState out_state = uw_channel_get_out_state_(channel);
  UwMessageState in_state = uw_channel_get_in_state_(channel);

  if (transport->connection_state == kUwBleTransportStateConnected) {
    time_t timeout = uw_device_is_setup(transport->device)
                         ? UW_IDLE_TIMEOUT_SECONDS
                         : UW_UNCONFIGURED_IDLE_TIMEOUT_SECONDS;
    if (idle_time_(transport) > timeout) {
      UW_LOG_WARN("Disconnecting after idle timeout\n");
      disconnect_(transport);
      return false;
    }
  }

  if (in_state != kUwMessageStateComplete) {
    // If the reply is not in progress, the read channel is either empty or
    // reading.
    switch (try_to_read_event_(device_channel, transport)) {
      case kHandlerStateComplete: {
        // If the read completes the command, then we clear the channel for the
        // next command.
        uw_device_channel_complete_exchange_(device_channel);
        break;
      }
      case kHandlerStateInProgress: {
        break;
      }
      case kHandlerStateWait: {
        // Wait for new packets if we aren't complete.
        return false;
      }
      case kHandlerStateDisconnect:
      // Fallthrough intended.
      case kHandlerStateError: {
        UW_LOG_WARN("Disconnecting\n");
        disconnect_(transport);
        break;
      }
    }
    in_state = uw_channel_get_in_state_(channel);
    out_state = uw_channel_get_out_state_(channel);
  }

  // If either channel is in an error state, or there is no reply available,
  // reset the channel and check for more work.
  if (in_state == kUwMessageStateError || out_state == kUwMessageStateError ||
      (in_state == kUwMessageStateComplete &&
       out_state == kUwMessageStateEmpty)) {
    uw_device_channel_reset_(device_channel);
    // Check for immediately available work.
    return true;
  }

  if (in_state == kUwMessageStateComplete && out_state == kUwMessageStateBusy) {
    switch (try_to_send_packet_(channel, transport)) {
      case kHandlerStateComplete: {
        uw_device_channel_complete_exchange_(device_channel);
        break;
      }
      case kHandlerStateInProgress: {
        break;
      }
      case kHandlerStateWait: {
        // TODO(jmccullough): Do we get notify signal on packet-sent?  Could
        // sleep in between.
        break;
      }
      case kHandlerStateDisconnect:
      // Fall through intended.
      case kHandlerStateError: {
        disconnect_(transport);
        break;
      }
    }
    // Check for new data to handle after.
    return true;
  }

  // More work to do.
  return true;
}

static bool create_service_(UwBleTransport* transport) {
  // Build the service structures.
  UwBleCharacteristic txChar = {};
  txChar.uuid = UwClientTxCharacteristicUuid;
  txChar.value_length = kUwCharacteristicSize;
  txChar.properties = kUwBlePropertyOptionWrite;

  UwBleCharacteristic rxChar = {};
  rxChar.uuid = UwClientRxCharacteristicUuid;
  rxChar.value_length = kUwCharacteristicSize;
  rxChar.properties = kUwBlePropertyOptionRead | kUwBlePropertyOptionIndicate;

  UwBleCharacteristic characteristics[] = {txChar, rxChar};

  UwBleService service = {};
  service.uuid = UwServiceUuid;
  service.characteristics = characteristics;
  service.characteristics_count =
      sizeof(characteristics) / sizeof(UwBleCharacteristic);

  return uwp_ble_create_service(&service, transport);
}

static bool start_advertising_(UwBleTransport* transport) {
  uw_ble_advertising_update_data_(transport->device);
  UW_LOG_INFO("Starting advertising\n");
  return uwp_ble_advertising_start();
}
