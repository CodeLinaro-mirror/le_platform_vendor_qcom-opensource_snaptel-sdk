/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "MyUtils.hpp"
#include <stdio.h>
#include <string>

#ifdef TELSDK_FEATURE_COMMONLOG_SUPPORT_ENABLED
#include <clog.h>
#include <vector>
#include <atomic>

extern std::atomic<clog_context_t> gClogCtx;
extern std::atomic<bool> gClogRegistered;

static clog_level_t convertToClogLevel(int level) {
    switch (level) {
        case LOG_EMERG:
        case LOG_ALERT:
        case LOG_CRIT:
            return CLOG_LEVEL_FATAL;
        case LOG_ERR:
            return CLOG_LEVEL_ERROR;
        case LOG_WARNING:
            return CLOG_LEVEL_WARN;
        case LOG_NOTICE:
        case LOG_INFO:
            return CLOG_LEVEL_INFO;
        case LOG_DEBUG:
            return CLOG_LEVEL_DEBUG;
        default:
            return CLOG_LEVEL_ERROR;
    }
}
#endif

bool enableDebug  = false;
bool enableSyslog = false;

void chronylog(int level, const char *file, int line, const char *func, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

#ifdef TELSDK_FEATURE_COMMONLOG_SUPPORT_ENABLED
    clog_context_t ctx = gClogCtx.load();
    if (gClogRegistered && ctx != nullptr) {
        va_list args_copy;
        va_copy(args_copy, args);
        int size = vsnprintf(NULL, 0, fmt, args_copy);
        va_end(args_copy);

        if (size >= 0) {
            std::vector<char> buffer(size + 1);
            va_copy(args_copy, args);
            vsnprintf(buffer.data(), buffer.size(), fmt, args_copy);
            va_end(args_copy);
            va_end(args);
            (void)clog_log(ctx, file, line, convertToClogLevel(level), "%s", buffer.data());
            return;
        }
        // Fall through to legacy logging if formatting failed
    }
#endif
    // Legacy logging: syslog or printf
    if (level != LOG_DEBUG || enableDebug) {
        if (!enableSyslog) {
            printf("[%s:%d] ", func, line);
            vprintf(fmt, args);
        } else {
            std::string prefixedFmt
                = std::string("[") + func + ":" + std::to_string(line) + "] " + fmt;
            vsyslog(level, prefixedFmt.c_str(), args);
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
