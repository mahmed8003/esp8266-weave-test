// Copyright 2016 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBUWEAVE_INCLUDE_UWEAVE_COUNTER_H_
#define LIBUWEAVE_INCLUDE_UWEAVE_COUNTER_H_

#include <stdint.h>
#include <stddef.h>

/*
 * A UwCounterSet tracks UWeave and application specific counters for debugging
 * a statistics.  Each counter is paired with a UWeave or application-specific
 * id.  The counters are coalesced to storage after a short fixed interval.
 */

typedef uint16_t UwCounterId;
typedef uint32_t UwCounterValue;

typedef struct UwCounterSet_ UwCounterSet;

static const UwCounterValue kUwCounterValueInvalid = -1;

/**
 * Returns the size of memory to allocate for a UwCounterSet with
 * app_counter_count application counters.
 */
size_t uw_counter_set_sizeof(size_t app_counter_count);

/**
 * Initializes a UwCounterSet for the specific set of application counter ids.
 * The list of individual counter ids allows the counter-set to evolve over
 * time while maintaining consistent counter ids that can be non-sequential.
 *
 * The app_counter_counter parameter must match the app_counter_count for
 * uw_counter_set_sizeof().
 */
void uw_counter_set_init(UwCounterSet* counter_set,
                         const UwCounterId app_ids[],
                         size_t app_counter_count);

#endif  // LIBUWEAVE_INCLUDE_UWEAVE_COUNTER_H_
