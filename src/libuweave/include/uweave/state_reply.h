// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBUWEAVE_INCLUDE_UWEAVE_STATE_REQUEST_H_
#define LIBUWEAVE_INCLUDE_UWEAVE_STATE_REQUEST_H_

#include "uweave/value_traits.h"

typedef struct UwStateReply_ UwStateReply;

/**
 * void set_state(...) {
 *   ++fingerprint;
 * }
 *
 * void handle_state_reply(...) {
 *
 *   UwMapValue single_trait_info[] = {
 *       {.key = uw_value_int(kTraitInfo), .value = uw_value_int(7)}};
 *
 *   UwComponentTraitState trait0[] = {
 *       uw_component_trait_state(kTraitId, trait_info,
 *           uw_map_value_count(sizeof(trait_info)))};
 *
 *   UwComponentTraits component0[] = {
 *     uw_component_traits(1, trait0,
 *                         uw_component_trait_state_count(sizeof(trait0)))};
 *
 *   uw_state_reply_set_state(state_reply, fingerprint, component0,
 *                            uw_component_traits_count(sizeof(component0)));
 * }
 */
bool uw_state_reply_set_state(UwStateReply* state_reply,
                              int64_t fingerprint,
                              const UwComponentTraits components[],
                              size_t component_len);

#endif  // LIBUWEAVE_INCLUDE_UWEAVE_STATE_REQUEST_H_
