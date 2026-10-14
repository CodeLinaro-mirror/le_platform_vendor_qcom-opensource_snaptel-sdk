/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file: MyUtils.hpp
 *
 * @brief: Provides utility functions.
 */

#ifndef MY_UTILS_HPP__
#define MY_UTILS_HPP__

#include <memory>
#include <deque>
#include <future>
#include <mutex>
#include <syslog.h>
#include <stdarg.h>

extern bool enableDebug;
extern bool enableSyslog;

void chronylog(int level, const char *file, int line, const char *func, const char *fmt, ...);

#define LOGI(fmt, ...) chronylog(LOG_NOTICE, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define LOGD(fmt, ...) chronylog(LOG_DEBUG, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define LOGE(fmt, ...) chronylog(LOG_ERR, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

class AsyncThread {
 public:
    /*Join existing threads from myself, it should NOT be called from 1 of my own thread context*/
    void joinAllThreads();

    /*Add a new future into the thread queue, it will be executed sometime later, currently only
     *support furture constructed by std::launch::async, not support std::launch::deferred*/
    void addToQueue(std::shared_future<void> &f);

 private:
    /*Remove completed futures from the thread queue*/
    void purgeCompletedFutures();

    std::deque<std::shared_future<void>> threadQueue_;
    /*Lock to protect when access the above threadQueue_*/
    std::mutex mutex_;
    bool exit_ = false;
};

#endif
