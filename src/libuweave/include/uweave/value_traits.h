// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBUWEAVE_INCLUDE_UWEAVE_VALUE_TRAITS_H_
#define LIBUWEAVE_INCLUDE_UWEAVE_VALUE_TRAITS_H_

#include <stddef.h>
#include <stdint.h>

struct UwMapValue_;

/**
 * Defines a UwValue-style object to specify the state values of a component
 * trait.  See state_reply.h for an example.
 */
typedef struct {
  int trait;
  const struct UwMapValue_* state;
  size_t length;
} UwComponentTraitState;

UwComponentTraitState uw_component_trait_state(int trait,
                                               const struct UwMapValue_* state,
                                               size_t length);

/**
 * Defines a UwValue-style object to specify the traits of a component See
 * state_reply.h for an example.
 */
typedef struct {
  int component;
  const UwComponentTraitState* traits;
  size_t length;
} UwComponentTraits;

UwComponentTraits uw_component_traits(int component,
                                      const UwComponentTraitState* traits,
                                      size_t length);

/**
 * Computes the number of elements in a dynamically sized UwComponentTraitState
 * array based on the sizeof result.
 */
static inline size_t uw_component_trait_state_count(const size_t state_len) {
  return state_len / sizeof(UwComponentTraitState);
}

/**
 * Computes the number of elements in a dynamically sized UwComponentTraits
 * array based on the sizeof result.
 */
static inline size_t uw_component_traits_count(const size_t component_len) {
  return component_len / sizeof(UwComponentTraits);
}

#endif  // LIBUWEAVE_INCLUDE_UWEAVE_VALUE_TRAITS_H_
