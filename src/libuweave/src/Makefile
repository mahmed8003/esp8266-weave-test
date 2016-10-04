# Copyright 2015 The Weave Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

#
# Makefile for making the main uWeave library
#

# Source files can be excluded from the build using the environment
#     C_SOURCE_EXCLUDES variable, for example:
# C_SOURCE_EXCLUDES="skipped_source.c development_file.c"
#
# The variable must be defined on the make command-line or exported to take
# effect.

DEPTH = ..

ARCH ?= host

include $(DEPTH)/devices/$(ARCH)/build/Makefile.common

.DEFAULT_GOAL := $(UWEAVE_STATIC_LIB)
