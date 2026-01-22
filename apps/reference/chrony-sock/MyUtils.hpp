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

void chronylog(int level, const char *fmt, ...);

#define LOGI(fmt, args...) chronylog(LOG_NOTICE, "[I][%s:%d] " fmt, __func__, __LINE__, ##args)
#define LOGD(fmt, args...) chronylog(LOG_DEBUG, "[D][%s:%d] " fmt, __func__, __LINE__, ##args)
#define LOGE(fmt, args...) chronylog(LOG_ERR, "[E][%s:%d] " fmt, __func__, __LINE__, ##args)

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
