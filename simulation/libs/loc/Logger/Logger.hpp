/*
 *  Copyright (c) 2020, The Linux Foundation. All rights reserved.
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

#ifndef _LOGGER_HPP_
#define _LOGGER_HPP_

#include <iostream>
#include <string>
#include <sstream>
#include <mutex>
#include <fstream>

#define Error(args...) Logger::getInstance().error(args)
#define Warn(args...) Logger::getInstance().warn(args)
#define Info(args...) Logger::getInstance().info(args)
#define Trace(args...) Logger::getInstance().trace(args)
#define Debug(args...) Logger::getInstance().debug(args)

enum class LoggerLevel {
    DISABLE_ALL = 0,
    ERROR = 1,
    WARN = 2,
    INFO = 3,
    TRACE = 4,
    DEBUG = 5,
};

enum class LoggerType {
    CONSOLE_LOG = 1,
    FILE_LOG = 2,
    CONSOLE_FILE_LOG =3,
};

class Logger {
private:
    LoggerType loggerType_;
    LoggerLevel loggerLevel_;
    std::mutex fileMutex_;
    std::ofstream logFile_;
    Logger();
    Logger(const Logger &) = delete;
    Logger &operator=(const Logger &) = delete;
    ~Logger();
    std::string getCurrentTime();
    void logToFile(std::string &text);
    void logToConsole(std:: string &text);

    template<typename T>
    void constructMessage(std::ostringstream &os, T arg1) {
        os<<arg1;
    }

    template<typename T, typename... Args>
    void constructMessage(std::ostringstream &os, T arg1, Args... args) {
        os<<arg1<<" : ";
        constructMessage(os,args...);
    }

    //Logging Type Can only be changed from confg1 file
    void initLoggingType();

    //Logging Level Can only be changed from confg1 file
    void initLoggingLevel();
    void setLoggerLevel(LoggerLevel level);
    void disableLogging();


    void logError(std::string &text);
    void logWarn(std::string &text);
    void logInfo(std::string &text);
    void logTrace(std::string &text);
    void logDebug(std::string &text);

public:

    static Logger& getInstance();

    template<typename... Args>
    void error(Args... args) {
        std::ostringstream os;
        constructMessage(os, args...);
        std::string text = os.str();
        logError(text);
    }

    template<typename... Args>
    void warn(Args... args) {
        std::ostringstream os;
        constructMessage(os, args...);
        std::string text = os.str();
        logWarn(text);
    }

    template<typename... Args>
    void info(Args... args) {
        std::ostringstream os;
        constructMessage(os, args...);
        std::string text = os.str();
        logInfo(text);
    }

    template<typename... Args>
    void trace(Args... args) {
        std::ostringstream os;
        constructMessage(os, args...);
        std::string text = os.str();
        logTrace(text);
    }

    template<typename... Args>
    void debug(Args... args) {
        std::ostringstream os;
        constructMessage(os, args...);
        std::string text = os.str();
        logDebug(text);
    }

    int getLoggerLevel();
    bool isConsoleLoggingEnabled();
    bool isFileLoggingEnabled();
};
#endif
