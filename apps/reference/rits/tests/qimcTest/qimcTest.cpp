/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file: qimcTest.cpp
 *
 * @brief: Implements Qimc Class as a command line tool.
 *
 */

#include "qimc.hpp"
#include "json.h"

Qimc *qimc      = nullptr;
bool stopReport = false;
bool isResPath  = false;

/**
 * @brief Feature for periodic reports; run by a thread
 *
 */
void startPeriodicReport(uint32_t interval_ms, string resPath) {
    if (!qimc) {
        std::cerr << "invalid qimc\n";
        return;
    }
    FILE *reportFile;
    // open file one time to clear it
    if (isResPath) {
        reportFile = std::fopen(resPath.c_str(), "w");
        if (reportFile != NULL) {
            std::fclose(reportFile);
        }
        reportFile = std::fopen(resPath.c_str(), "a");
    } else {
        reportFile = std::fopen(RES_FILE, "w");
        if (reportFile != NULL) {
            std::fclose(reportFile);
        }
        reportFile = std::fopen(RES_FILE, "a");
    }
    if (reportFile == NULL) {
        std::cerr << "Periodic report log file not opened successfully. Returning.\n";
        return;
    }

    int timer_misses = 0;
    uint64_t exp;
    ssize_t s;
    int timerfd;
    struct itimerspec its = {0};
    std::cout << "Starting periodic report at " << interval_ms << "\n";
    timerfd = timerfd_create(CLOCK_MONOTONIC, 0);
    if (timerfd < 0) {
        return;
    }

    /* Start the timer */
    its.it_value.tv_sec  = interval_ms / 1000;
    its.it_value.tv_nsec = (interval_ms % 1000) * 1000000;
    its.it_interval      = its.it_value;

    if (timerfd_settime(timerfd, 0, &its, NULL) < 0) {
        close(timerfd);
        return;
    }
    struct timeval currTime;
    gettimeofday(&currTime, NULL);
    time_t startTime = currTime.tv_sec;
    timespec ts;
    // build default command
    string command              = "{\"blob\":5, \"timestamp\": true}";
    struct json_object *jsonTmp = nullptr;
    auto ret                    = 0;
    json_object *reqPeriodic    = nullptr;
    json_object *resPeriodic    = nullptr;
    string path                 = "./req.json";
    reqPeriodic                 = json_object_from_file(path.c_str());
    // get report
    while (!stopReport) {
        // should be based on an existing provided command but maybe we can just do blob for now
        // also document timestamp here and provide both timestamp off client and server to user
        resPeriodic = qimc->sendAndGetResponse(reqPeriodic);
        if (resPeriodic) {
            // add the timestamp we received the response before writing to log file
            timespec_get(&ts, TIME_UTC);
            const int64_t nanoTime = (ts.tv_sec * BILLION) + ts.tv_nsec;
            json_object_object_add(
                resPeriodic, "timestamp_client", json_object_new_int64(nanoTime));
            int64_t servNanoTime = 0;
            // get the timestamp from the original response
            for (auto key : QMonitorJson::kStr) {
                if (json_object_object_get_ex(resPeriodic, key, &jsonTmp)) {
                    if (strcmp(key, "timestamp") == 0) {
                        servNanoTime = std::stoull(string(json_object_to_json_string(jsonTmp)));
                    }
                }
            }
            json_object_object_add(
                resPeriodic, "timestamp_diff", json_object_new_int64(nanoTime - servNanoTime));
            ret = 0;
            // convert json object to json string
            const char *jsonResStr = json_object_to_json_string(resPeriodic);
            // append to open log file
            fprintf(reportFile, "%s\n", jsonResStr);
        }
        s = read(timerfd, &exp, sizeof(exp));
        if (s == sizeof(uint64_t) && exp > 1) {
            timer_misses += (exp - 1);
        }
    }
    std::fclose(reportFile);
}

int main(int argc, const char **argv) {
    std::thread reportThr;
    string jsonResPath;
    qimc                    = new Qimc(Qimc::loadArgs(argc, argv));
    bool periodicReport     = false;
    uint32_t reportInterval = 100;
    for (int i = 1; i < argc; i++) {
        const char c = argv[i][1];
        switch (c) {
            case 'm':
                periodicReport = true;
                reportInterval = atoi(argv[++i]);
                break;
            case 'r':
                isResPath   = true;
                jsonResPath = string(argv[++i]);
                break;
        }
    }

    if (periodicReport) {
        reportThr = std::thread(startPeriodicReport, reportInterval, jsonResPath);
        // wait for input here and close correspondingly
        std::cout << "Input q or Q character and then ENTER to quit qimcTest: \n";
        char ch[2];
        while (fgets(ch, 2, stdin) != NULL) {
            if (ch[0] == 'q' || ch[0] == 'Q') {
                std::cout << "Quit key was input\n";
                break;
            }
            std::cout << "Input q or Q character and then ENTER to quit qimcTest: \n";
        }
        std::cout << "Quitting qimc\n";
        stopReport = true;
        if (reportThr.joinable()) {
            reportThr.join();
        }
    }
    return 0;
}
