#Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
#SPDX-License-Identifier: BSD-3-Clause-Clear

LIB_DIR=$(cd $(dirname "${BASH_SOURCE[0]}")/../lib && pwd)
BIN_DIR=$(cd $(dirname "${BASH_SOURCE[0]}") && pwd)
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$LIB_DIR/
export PATH=$PATH:$BIN_DIR/
