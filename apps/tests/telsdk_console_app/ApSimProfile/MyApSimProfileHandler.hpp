/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef MYAPSIMPROFILEHANDLER_HPP
#define MYAPSIMPROFILEHANDLER_HPP

#include <telux/common/CommonDefines.hpp>

class MyApSimProfileCallback {
 public:
    static void onResponseCallback(telux::common::ErrorCode error);
};

#endif  // MYAPSIMPROFILEHANDLER_HPP
