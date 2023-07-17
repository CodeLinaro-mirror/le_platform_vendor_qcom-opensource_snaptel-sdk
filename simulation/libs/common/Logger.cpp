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

#include <thread>

#include "Logger.hpp"
#include "ConfigParser.hpp"

extern "C" {
#include <limits.h>
#include <unistd.h>
}

#define DEFAULT_LOG_FILENAME "tel.log"

using namespace std;

Logger::Logger() {
    config_ = std::make_shared<ConfigParser>(DEFAULT_STUB_CONFIG_FILE_NAME,
        DEFAULT_STUB_CONFIG_FILE_PATH);

    initLoggingType();
    initLoggingLevel();
    initProcessName();
    initProcessId();
}

void Logger::initProcessId() {
   processID_ = getpid();
}

void Logger::initProcessName() {
    char path[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", path, PATH_MAX);
    std::string fullPath = std::string(path, (count > 0) ? count : 0);
    auto const pos = fullPath.find_last_of('/');
    processName_ = fullPath.substr(pos + 1);
}

void Logger::initLoggingType() {
    std::string loggerType = config_->getValue("LOGGER_TYPE");

    if (loggerType == "CONSOLE_LOG") {
        loggerType_ = LoggerType::CONSOLE_LOG;
    } else if (loggerType == "FILE_LOG") {
        loggerType_ = LoggerType::FILE_LOG;
    } else if (loggerType == "CONSOLE_FILE_LOG") {
        loggerType_ = LoggerType::CONSOLE_FILE_LOG;
    } else {
        loggerType_ = LoggerType::FILE_LOG;
    }

    if(isFileLoggingEnabled()) {
        string fullPath = "";
        std::string filePath = config_->getValue("LOG_FILE_PATH");
        std::string fileName = config_->getValue("LOG_FILE_NAME");
        if(!filePath.empty())
        {
            fullPath += filePath;
        }

        if(!fileName.empty()) {
            fullPath += fileName;
        } else {
            fullPath += DEFAULT_LOG_FILENAME;
        }

        logFile_.open(fullPath, ios::out|ios::app);
        if(logFile_.rdstate() != std::ios_base::goodbit) {
            std::cout << __FUNCTION__ << " open " << fullPath << "failed" << "\n";
            loggerType_ = LoggerType::CONSOLE_LOG;
        }
    }
}

void Logger::initLoggingLevel() {
    std::string loggerLevel = config_->getValue("LOGGER_LEVEL");

    if (loggerLevel == "DISABLE_ALL") {
        loggerLevel_ = LogLevel::LEVEL_NONE;
    } else if (loggerLevel == "ERROR") {
        loggerLevel_ = ERROR;
    } else if (loggerLevel == "WARNING") {
        loggerLevel_ = WARNING;
    } else if (loggerLevel == "INFO") {
        loggerLevel_ = INFO;
    } else if (loggerLevel == "DEBUG") {
        loggerLevel_ = DEBUG;
    } else {
        loggerLevel_ = DEBUG;
    }
}

LogLevel Logger::getLoggerLevel() {
    return loggerLevel_;
}

Logger::~Logger() {
    if (config_ ) {
        config_ = nullptr;
    }
    lock_guard<mutex> guard(fileMutex_);
    if (logFile_.is_open()) {
        logFile_.close();
    }
}

Logger& Logger::getInstance() {
    static Logger logger;
    return logger;
}

string Logger::getCurrentTime() {
    string currTime;
    time_t now = time(0);
    currTime.assign(ctime(&now));
    // removing Last charactor of currentTime which is "\n"
    currTime = currTime.substr(0, currTime.size()-1);
    return currTime;
}

void Logger::writeLogMessage(std::ostringstream &os, LogLevel logLevel,
    const std::string &fileName, const int &component,
    const std::string &lineNo) {
    std::string timeStamp = getCurrentTime();
    std::string fileNameAndLineNo = "";
    std::string processIdAndName = "";
    std::ostringstream outputStream;

    processIdAndName = std::to_string(processID_) + "/" + processName_;

    auto const pos = fileName.find_last_of('/');
    // get the filename from full path
    fileNameAndLineNo = " " + fileName.substr(pos + 1) + "(" + lineNo + ") ";

    switch(logLevel) {
      case ERROR:
         outputStream << "[E]" << timeStamp << " " << processIdAndName << fileNameAndLineNo;
         break;
      case WARNING:
         outputStream << "[W]" << timeStamp << " " << processIdAndName << fileNameAndLineNo;
         break;
      case INFO:
         outputStream << "[I]" << timeStamp << " " << processIdAndName << fileNameAndLineNo;
         break;
      case DEBUG:
         outputStream << "[D]" << timeStamp << " " << processIdAndName << fileNameAndLineNo;
         break;
      default:
         break;
}

    outputStream << std::this_thread::get_id() << ": ";

    //Check if the ostringstream containing the input argument is empty
    if(!os.str().empty()) {
      outputStream << os.str();

    if(isFileLoggingEnabled() && getLoggerLevel() >= ERROR)
        logToFile(outputStream);

    if(isConsoleLoggingEnabled() && getLoggerLevel() >= ERROR)
        logToConsole(outputStream);
    }
}

void Logger::logToFile(std::ostringstream & os) {
    lock_guard<mutex> guard(fileMutex_);
    logFile_<< os.str() << endl;
}

void Logger::logToConsole(std::ostringstream & os) {
    std::cout << os.str() << "\n";
}

bool Logger::isFileLoggingEnabled() {
    if(loggerType_ == LoggerType::FILE_LOG || loggerType_ == LoggerType::CONSOLE_FILE_LOG)
        return true;
    return false;
}

bool Logger::isConsoleLoggingEnabled() {
    if(loggerType_ == LoggerType::CONSOLE_LOG || loggerType_ == LoggerType::CONSOLE_FILE_LOG)
        return true;
    return false;
}
