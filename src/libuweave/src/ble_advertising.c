// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/ble_advertising.h"
#include "src/uw_assert.h"
#include "uweave/provider/ble.h"

// Official BLE Google Manufacturer Data ID.
static const uint16_t kGoogleId_ = 0xE000;

// Id of the Privet payload field in the advertising packet.
static const uint8_t kFieldPrivetDataTag_ = 0x0D;
static const uint8_t kFieldPublicIdTag_ = 0x0E;

typedef struct __attribute__((__packed__)) {
  uint8_t privet_tag;
  uint8_t device_class[2];
  uint8_t model_id[3];
  uint8_t capabilities[2];
} AdvertisingLayout;

typedef struct __attribute__((__packed__)) {
  uint8_t type;
  uint8_t public_id[4];
} PublicIdLayout;

bool uw_ble_advertising_update_data_(UwDevice* device) {
  UwBleAdvertisingData advertising_data;
  uw_ble_advertising_get_data_(device, &advertising_data);

  UwSettings* settings = uw_device_get_settings(device);
  if (!uwp_ble_set_advertising_data(settings->name, kGoogleId_,
                                    &advertising_data.bytes[0],
                                    sizeof(advertising_data.bytes))) {
    UW_ASSERT(false, "Failed to set advertising data\n");
    return false;
  }

  return true;
}

void uw_ble_advertising_get_data_(UwDevice* device,
                                  UwBleAdvertisingData* data) {
  UwSettings* settings = uw_device_get_settings(device);
  UwDeviceCrypto* device_crypto = &device->device_crypto;
  *data = (UwBleAdvertisingData){};

  uint8_t caps = 0;
  if (settings->supports_wifi_24ghz) {
    caps |= kUwBleAdvertisingFlagWiFi24Ghz;
  }
  if (settings->supports_wifi_50ghz) {
    caps |= kUwBleAdvertisingFlagWiFi50Ghz;
  }
  if (settings->supports_ble_40) {
    caps |= kUwBleAdvertisingFlagBle40;
  }

  uint8_t pos = 0;

  data->bytes[pos] = sizeof(AdvertisingLayout);
  pos++;

  memcpy(&data->bytes[pos],
         &((AdvertisingLayout){
             .privet_tag = kFieldPrivetDataTag_,
             .device_class = {settings->device_class[0],
                              settings->device_class[1]},
             .model_id = {settings->model_id[0], settings->model_id[1],
                          settings->model_id[2]},
             .capabilities = {uw_device_is_setup(device)
                                  ? 0
                                  : kUwBleAdvertisingFlagNeedsWeaveRegistration,
                              caps}}),
         sizeof(AdvertisingLayout));

  pos += sizeof(AdvertisingLayout);
  data->bytes[pos] = sizeof(PublicIdLayout);
  pos++;

  memcpy(&data->bytes[pos], &((PublicIdLayout){
                                .type = kFieldPublicIdTag_,
                                .public_id = {device_crypto->device_id[0],
                                              device_crypto->device_id[1],
                                              device_crypto->device_id[2],
                                              device_crypto->device_id[3]},
                            }),
         sizeof(PublicIdLayout));
}
