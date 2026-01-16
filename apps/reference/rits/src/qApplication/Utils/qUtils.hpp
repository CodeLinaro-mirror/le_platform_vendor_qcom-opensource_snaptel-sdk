/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file: qUtils.hpp
 *
 * @brief: Implementation for Utilities for qApplication
 */
#ifndef QUTILS_HPP_
#define QUTILS_HPP_

#include <iostream>
#include <stdio.h>
#include <string.h>
#include "v2x_diag.h"
#include "qDiagLogPacketDef.h"
#include <telux/sec/RandomNumberManager.hpp>
#include <telux/sec/SecurityFactory.hpp>

#define V2X_APPS_DIAG_LOG_PKT(type, pbuf, buf_size)                            \
    v2x_diag_log_state_et ec = v2x_diag_log_packet(type, pbuf, buf_size);      \
    if (ERR_SUCCESS != ec && ERR_STATUS_FAIL != ec) {                          \
        printf("%s: send type[0x%x], errcode: %d \n", __FUNCTION__, type, ec); \
    }

class QUtils {
 private:
    void fillVersion(uint32_t *version);
    std::shared_ptr<telux::sec::IRandomNumberManager> rngMgr_ = nullptr;

 public:
    QUtils();
    void initDiagLog();
    void deInitDiagLog();
    int hwTRNGInt(uint32_t &randomNumber);
    int hwTRNGChar(uint8_t &randomNumber);

    template <typename InfoType>
    void sendLogPacket(InfoType *info, v2x_diag_log_packet_et type) {
        this->fillVersion(&info->Version);

        V2X_APPS_DIAG_LOG_PKT(type, info, sizeof(InfoType));
    }
};
#endif
