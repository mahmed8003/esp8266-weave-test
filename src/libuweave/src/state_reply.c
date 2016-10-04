// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/state_reply.h"

#include "src/buffer.h"
#include "src/log.h"
#include "src/privet_defines.h"
#include "src/value.h"
#include "tinycbor/src/cbor.h"

static UwStatus trait_encoder_(UwValueCallbackMapContext* context,
                               const void* data,
                               size_t index) {
  const UwComponentTraits* component = (const UwComponentTraits*)data;
  UwMapValue trait_value = {
      .key = uw_value_int(component->traits[index].trait),
      .value = uw_value_map(component->traits[index].state,
                            component->traits[index].length)};
  return uw_value_callback_map_append(context, &trait_value);
}

static UwStatus component_encoder_(UwValueCallbackMapContext* context,
                                   const void* data,
                                   size_t index) {
  const UwComponentTraits* components = (const UwComponentTraits*)data;
  const UwComponentTraits* component = &components[index];

  UwMapValue state_map[] = {
      {.key = uw_value_int(PRIVET_STATE_KEY_COMPONENT_STATE),
       .value = uw_value_callback_map(&trait_encoder_, (void*)component,
                                      component->length)},
  };

  UwMapValue component_value = {
      .key = uw_value_int(component->component),
      .value = uw_value_map(state_map, uw_value_map_count(sizeof(state_map)))};

  return uw_value_callback_map_append(context, &component_value);
}

void uw_state_reply_init_(UwStateReply* state_reply,
                          UwPrivetRequest* privet_request) {
  memset(state_reply, 0, sizeof(UwStateReply));

  state_reply->privet_request = privet_request;
}

bool uw_state_reply_set_state(UwStateReply* state_reply,
                              int64_t fingerprint,
                              const UwComponentTraits components[],
                              size_t component_len) {
  UwMapValue result[] = {
      {.key = uw_value_int(PRIVET_STATE_KEY_FINGERPRINT),
       .value = uw_value_int(fingerprint)},
      {.key = uw_value_int(PRIVET_STATE_KEY_COMPONENTS),
       .value = uw_value_callback_map(&component_encoder_, (void*)components,
                                      component_len)},
  };

  UwValue result_value =
      uw_value_map(result, uw_value_map_count(sizeof(result)));

  return uw_privet_request_reply_privet_ok_(state_reply->privet_request,
                                            &result_value);
}
