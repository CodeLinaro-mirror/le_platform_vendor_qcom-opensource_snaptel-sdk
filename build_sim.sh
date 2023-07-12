#Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
#SPDX-License-Identifier: BSD-3-Clause-Clear

#!/bin/bash
# Additional checks to see if we are in the right directory to be done
ROOTFS=$2

if [ "$ROOTFS" == "" ] ; then
   ROOTFS=$PWD/rootfs
fi

export ROOTFS

make $1
