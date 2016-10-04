// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "uweave/value_traits.h"

#include "uweave/value.h"

UwComponentTraitState uw_component_trait_state(int trait,
                                               const UwMapValue* state,
                                               size_t length) {
  UwComponentTraitState result = {
      .trait = trait, .state = state, .length = length};
  return result;
}

UwComponentTraits uw_component_traits(int component,
                                      const UwComponentTraitState* traits,
                                      size_t length) {
  UwComponentTraits result = {
      .component = component, .traits = traits, .length = length};
  return result;
}
