/*
 *  Copyright (c) 2021 The Linux Foundation. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of The Linux Foundation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file       Log.hpp
 * @brief      Log class used for logging
 */
#ifndef LOG_HPP
#define LOG_HPP

#include <cstdarg>
#include <cstdio>

extern "C" {
#include <syslog.h>
}

#define LOGI(fmt, args...) httpLog(LOG_NOTICE, "[I][%s:%d] " fmt, __func__, __LINE__, ##args)
#define LOGD(fmt, args...) httpLog(LOG_DEBUG, "[D][%s:%d] " fmt, __func__, __LINE__, ##args)
#define LOGE(fmt, args...) httpLog(LOG_ERR, "[E][%s:%d] " fmt, __func__, __LINE__, ##args)

static int ENABLE_DEBUG = 0;
static int ENABLE_SYSLOG = 0;

inline void httpLog(int level, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    if (level != LOG_DEBUG || ENABLE_DEBUG) {
        vprintf(fmt, args);
        if (ENABLE_SYSLOG) {
            vsyslog(level, fmt, args);
        }
    }
    va_end(args);
}

#endif