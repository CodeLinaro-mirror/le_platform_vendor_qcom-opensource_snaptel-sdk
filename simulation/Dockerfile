#Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
#SPDX-License-Identifier: BSD-3-Clause-Clear

# Download base image ubuntu 18.04
FROM ubuntu:18.04

ARG ROOTFS=rootfs

# LABEL about the custom image
LABEL description="This is custom docker image for SDK simulation users"

RUN export DEBIAN_FRONTEND=noninteractive

RUN apt-get -qq update
RUN apt-get -y install sudo
RUN apt-get update --fix-missing

RUN apt-get -y install build-essential
RUN apt-get -y install nano
RUN apt-get -y install net-tools
RUN apt-get -y install python3
RUN apt-get -y install python
RUN apt-get -y install gdb
RUN apt-get -y install iproute2

copy $ROOTFS/include /usr/include
copy $ROOTFS/bin /usr/bin
copy $ROOTFS/lib /usr/lib
copy $ROOTFS/etc /etc
copy $ROOTFS/data /data
copy $ROOTFS/share /usr/share

RUN chmod +x /etc/init.d/telsdk_simulation_server
RUN chmod o+r /etc/init.d/telsdk_simulation_server
CMD /etc/init.d/telsdk_simulation_server start && /bin/bash

