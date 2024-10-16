/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file: qMonitorJson.hpp
 *
 * @brief: Monitoring Json and other  helper structs and functions.
 *  */

#pragma once
#include <map>
#include <iostream>
#include <fstream>
#include <string>
using namespace std;

namespace QMonitorJson
{

    enum JsonKeys
    {
        TOTAL_TX,
        TOTAL_RX,
        TOTAL_RSUS,
        TOTAL_RVS,
        RX_FAILS,
        DECODE_FAILS,
        SEC_FAILS,
        MBD_ALERTS,
        TX_BSMS,
        TX_SIGNED_BSMS,
        RX_BSMS,
        RX_SIGNED_BSMS,
        MONITOR_RATE,
        TIMEFRAME,
        TIMESTAMP, // nano since epoch at send time
        JSON_VER,
        QITS_VER,
        TELSDK_VER,
        QMON_VER,
        BLOB,
        CLOSE
    };

    static const char *kStr[] = { //MUST be in the same order as JsonKeys enum.
        "totalTx",
        "totalRx",
        "totalRSUs",
        "totalRVs",
        "rxFails",
        "decodeFails",
        "securityFails",
        "mbdAlerts",
        "txBSMs",
        "txSignedBSMs",
        "rxBSMs",
        "rxSignedBSMs",
        "monitorRate",
        "timeframe",
        "timestamp",
        "jsonVersion",
        "qitsVersion",
        "telsdkVersion",
        "qMonVersion",
        "blob",
        "close"};

    static const map<const char *, int> kMap = []()
    {
        map<const char *, int> m;
        int i = 0;
        for (auto k : kStr)
        {
            m[k] = i;
            i++;
        }
        return m;
    }();

}