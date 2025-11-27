#!/bin/bash

# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

supervisord -c /etc/supervisord.conf && /bin/bash
