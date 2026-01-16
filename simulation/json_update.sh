#!/bin/bash

# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

BIN_DIR=$(cd $(dirname "${BASH_SOURCE[0]}") && pwd)
source /$BIN_DIR/setup_simulation.sh

telsdk_event_injector -f json_update -e modify $@