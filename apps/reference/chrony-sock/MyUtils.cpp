/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "MyUtils.hpp"

bool enableDebug  = false;
bool enableSyslog = false;

void chronylog(int level, const char *fmt, ...) {
    va_list args;

    va_start(args, fmt);
    if (level != LOG_DEBUG || enableDebug) {
        if (!enableSyslog) {
            vprintf(fmt, args);
        } else {
            vsyslog(level, fmt, args);
        }
    }
    va_end(args);
}

void AsyncThread::joinAllThreads() {
    std::deque<std::shared_future<void>> tmp;
    std::lock_guard<std::mutex> lock(mutex_);
    if (not exit_) {
        exit_ = true;
        purgeCompletedFutures();
        tmp = threadQueue_;
        threadQueue_.clear();
    }
}

void AsyncThread::addToQueue(std::shared_future<void> &f) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (not exit_) {
        purgeCompletedFutures();
        threadQueue_.push_back(f);
    }
}

void AsyncThread::purgeCompletedFutures() {
    auto itr = std::begin(threadQueue_);
    // Iterate from head of queue and remove if the task is complete
    while (itr != std::end(threadQueue_)) {
        bool doRemove = false;
        if (itr->valid()) {
            // If the task has already completed, we can remove it.
            if (std::future_status::ready == itr->wait_for(std::chrono::seconds(0))) {
                itr->get();
                doRemove = true;
            }
        } else {
            // If the task is invalid, assume it's also complete and remove it as well.
            doRemove = true;
        }
        if (doRemove) {
            itr = threadQueue_.erase(itr);
        } else {
            ++itr;
        }
    }
}
