/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _LOGGER_HPP_
#define _LOGGER_HPP_

#include <iostream>
#include <string>
#include <sstream>
#include <mutex>
#include <fstream>
#include <memory>
#include <telux/common/Log.hpp>

using namespace telux::common;

class ConfigParser;

enum class LoggerType {
    CONSOLE_LOG = 1,
    FILE_LOG = 2,
    CONSOLE_FILE_LOG =3,
};

class Logger {
private:
    LoggerType loggerType_;
    LogLevel loggerLevel_;
    int processID_;
    std::mutex fileMutex_;
    std::ofstream logFile_;
    std::shared_ptr<ConfigParser> config_;
    std::string processName_;

    Logger();
    Logger(const Logger &) = delete;
    Logger &operator=(const Logger &) = delete;
    ~Logger();
    std::string getCurrentTime();
    void logToFile(std::ostringstream & os);
    void logToConsole(std::ostringstream & os);

    //Logging Type can only be changed from config file
    void initLoggingType();

    //Logging Level can only be changed from config file
    void initLoggingLevel();
    void initProcessId();
    void initProcessName();

public:
    static Logger& getInstance();
    LogLevel getLoggerLevel();

    /*
    * write log a message to console and a log file based on the settings.
    */
    void writeLogMessage(std::ostringstream &os, LogLevel logLevel,
        const std::string &fileName, const int &component, const std::string &lineNo);

    bool isConsoleLoggingEnabled();
    bool isFileLoggingEnabled();
};

template <typename... MessageArgs>
void Log::logMessage(LogLevel logLevel, const std::string &fileName, const std::string &lineNo,
                     const int &component, MessageArgs... params) {
    Logger &logger = Logger::getInstance();

    std::ostringstream outputStream;
    if (logger.getLoggerLevel() >= ERROR) {
        constructMessage(outputStream, params...);
        logger.writeLogMessage(outputStream, logLevel, fileName, component, lineNo);
    }
}

template <typename K, typename T>
void Log::constructMessage(K &os, T param) {
   os << param;
}

template <typename K, typename T, typename... MessageArgs>
void Log::constructMessage(K &os, T param, MessageArgs... params) {
   os << param;
   return constructMessage(os, params...);
}

#endif
