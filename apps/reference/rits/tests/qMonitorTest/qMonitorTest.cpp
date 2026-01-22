/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file: qMontiorTest.cpp
 *
 * @brief: Unit Test for qMonitor
 *
 */

#include "qMonitor.hpp"
#include <signal.h>
#include <iostream>
using namespace std;
QMonitor *qMon;
void signalHandler(int signum) {
    std::cout << "Closing connection" << std::endl;
    qMon->stop();
    exit(signum);
}

int main(int argc, const char **argv) {
    signal(SIGINT, signalHandler);
    qMon = new QMonitor(QMonitor::loadArgs(argc, argv));
    return 0;
}