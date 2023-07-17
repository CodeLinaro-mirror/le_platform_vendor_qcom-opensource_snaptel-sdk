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

#include <iostream>
#include "Logger.hpp"
#include "../StubHelper.hpp"

#define DEFAULT_LOG_FILENAME "log_messages"
#define FILE_EXTENSION ".log"
#define MAX_DATE 20

using namespace std;

Logger::Logger() {
    initLoggingType();
    initLoggingLevel();
}

void Logger::initLoggingType() {
    auto &stub =  StubHelper::getInstance();
    string typeString = stub.configValue("LOGGER_TYPE");
    if(typeString.compare(0, 11, "CONSOLE_LOG", 11) ==0)
        loggerType_ = LoggerType::CONSOLE_LOG;
    else if(typeString.compare(0, 8, "FILE_LOG", 8) ==0)
        loggerType_ = LoggerType::FILE_LOG;
    else if(typeString.compare(0, 16, "CONSOLE_FILE_LOG", 16) ==0)
        loggerType_ = LoggerType::CONSOLE_FILE_LOG;
    else
        loggerType_ = LoggerType::CONSOLE_LOG;

    if(isFileLoggingEnabled()) {
        time_t now = time(0);
        char the_date[MAX_DATE];
        //localtime is used just once as it is not thread safe
        strftime(the_date, MAX_DATE, "%Y%m%d%H%M%S", localtime(&now));
        string filename = DEFAULT_LOG_FILENAME + string(the_date)+ FILE_EXTENSION;
        logFile_.open(filename, ios::out|ios::app);
    }
}

void Logger::initLoggingLevel() {
    auto &stub =  StubHelper::getInstance();
    string levelString = stub.configValue("LOGGER_LEVEL");
    if(levelString.compare(0,11,"DISABLE_ALL",11) ==0)
        disableLogging();
    else if(levelString.compare(0,5,"ERROR",5) ==0)
        setLoggerLevel(LoggerLevel::ERROR);
    else if(levelString.compare(0,4,"WARN",4) ==0)
        setLoggerLevel(LoggerLevel::WARN);
    else if(levelString.compare(0,4,"INFO",4) ==0)
        setLoggerLevel(LoggerLevel::INFO);
    else if(levelString.compare(0,5,"TRACE",5) ==0)
        setLoggerLevel(LoggerLevel::TRACE);
    else if(levelString.compare(0,5,"DEBUG",5) ==0)
        setLoggerLevel(LoggerLevel::DEBUG);
    else
        setLoggerLevel(LoggerLevel::INFO);
}

Logger::~Logger() {
    if(logFile_.is_open())
        logFile_.close();
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

void Logger::logToFile(string &text) {
    lock_guard<mutex> guard(fileMutex_);
    logFile_<< getCurrentTime() << " " << text << endl;
}

void Logger::logToConsole(string &text) {
    cout<< getCurrentTime() << " " << text << endl;
}

void Logger::setLoggerLevel(LoggerLevel level) {
    loggerLevel_ = level;
}

void Logger::disableLogging() {
    setLoggerLevel(LoggerLevel::DISABLE_ALL);
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

int Logger::getLoggerLevel() {
    return (static_cast<int>(loggerLevel_));
}

void Logger::logError(string &text) {
    string message;
    message.append(" [E] ");
    message.append(text);

    //logging to file
    if(isFileLoggingEnabled() && loggerLevel_ >= LoggerLevel::ERROR)
        logToFile(message);

    //logging to console
    if(isConsoleLoggingEnabled() && loggerLevel_ >= LoggerLevel::ERROR)
        logToConsole(message);
}

void Logger::logWarn(string &text) {
    string message;
    message.append(" [W] ");
    message.append(text);

    //logging to file
    if(isFileLoggingEnabled() && loggerLevel_ >= LoggerLevel::WARN)
        logToFile(message);

    //logging to console
    if(isConsoleLoggingEnabled() && loggerLevel_ >= LoggerLevel::WARN)
        logToConsole(message);
}

void Logger::logInfo(string &text) {
    string message;
    message.append(" [I] ");
    message.append(text);

    //logging to file
    if(isFileLoggingEnabled() && loggerLevel_ >= LoggerLevel::INFO)
        logToFile(message);

    //logging to console
    if(isConsoleLoggingEnabled() && loggerLevel_ >= LoggerLevel::INFO)
        logToConsole(message);
}

void Logger::logTrace(string &text) {
    string message;
    message.append(" [T] ");
    message.append(text);

    //logging to file
    if(isFileLoggingEnabled() && loggerLevel_ >= LoggerLevel::TRACE)
        logToFile(message);

    //logging to console
    if(isConsoleLoggingEnabled() && loggerLevel_ >= LoggerLevel::TRACE)
        logToConsole(message);
}

void Logger::logDebug(string &text) {
    string message;
    message.append(" [D] ");
    message.append(text);

    //logging to file
    if(isFileLoggingEnabled() && loggerLevel_ >= LoggerLevel::DEBUG)
        logToFile(message);

    //logging to console
    if(isConsoleLoggingEnabled() && loggerLevel_ >= LoggerLevel::DEBUG)
        logToConsole(message);
}
