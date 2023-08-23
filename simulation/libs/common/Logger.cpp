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
#include <syslog.h>

#include "Logger.hpp"
#include "SimulationConfigParser.hpp"

extern "C" {
#include <stdio.h>
#include <sys/stat.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
}

#define DEFAULT_LOG_FILENAME "tel.log"
#define DEFAULT_LOG_FILE_MAX_SIZE 5 * 1024 * 1024  // 5 MB
#define STAT_FAILURE -1

static constexpr uint8_t UMASK_BITS = 0002;

using namespace std;

Logger::Logger() {
    config_ = std::make_shared<SimulationConfigParser>();

    initLoggingType();
    initLoggingLevel();
    initProcessName();
    initProcessId();
    initLogFileMaxSize();
}

void Logger::initProcessId() {
   processID_ = getpid();
}

void Logger::initLogFileMaxSize() {
   std::string val = config_->getValue("MAX_LOG_FILE_SIZE");
   logFileMaxSize_ = DEFAULT_LOG_FILE_MAX_SIZE;

   if(!val.empty()) {
      logFileMaxSize_ = stoi(val);
   }
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
    mode_t mask = umask(UMASK_BITS);

    if (loggerType == "CONSOLE_LOG") {
        loggerType_ = LoggerType::CONSOLE_LOG;
    } else if (loggerType == "FILE_LOG") {
        loggerType_ = LoggerType::FILE_LOG;
    } else if (loggerType == "CONSOLE_FILE_LOG") {
        loggerType_ = LoggerType::CONSOLE_FILE_LOG;
    } else if (loggerType == "SYSLOG_LOG") {
        loggerType_ = LoggerType::SYSLOG_LOG;
    } else {
        loggerType_ = LoggerType::FILE_LOG;
    }

    if(isFileLoggingEnabled()) {
        std::string filePath = config_->getValue("LOG_FILE_PATH");
        std::string fileName = config_->getValue("LOG_FILE_NAME");
        if(!filePath.empty())
        {
            logFileFullName_ += filePath;
        }

        if(!fileName.empty()) {
            logFileFullName_ += fileName;
        } else {
            logFileFullName_ += DEFAULT_LOG_FILENAME;
        }

        logFile_.open(logFileFullName_, ios::out|ios::app);
        if(logFile_.rdstate() != std::ios_base::goodbit) {
            std::cout << __FUNCTION__ << " open " << logFileFullName_ << "failed" << "\n";
            loggerType_ = LoggerType::CONSOLE_LOG;
        }
        struct stat st;
        if (stat(logFileFullName_.c_str(), &st) != STAT_FAILURE) {
            inodeNumber_ = st.st_ino;
        }
        umask(mask);
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

    if(isSyslogLoggingEnabled() && getLoggerLevel() >= ERROR)
        logToSyslog(outputStream, logLevel);
    }
}

ino_t Logger::reopenLogFile() {
   struct stat st;
   if (logFile_.is_open()) {
      logFile_.close();
   }
   logFile_.clear();

   logFile_.open(logFileFullName_, std::ios::app);
   if (logFile_.rdstate() == std::ios_base::goodbit) {
       stat(logFileFullName_.c_str(), &st);
       return st.st_ino;
   }
   return 0;
}

int Logger::acquireLock(int &fileDescriptor) {
    struct flock lock;
    lock.l_type = F_WRLCK;
    lock.l_start = 0;
    lock.l_whence = SEEK_SET;
    lock.l_len = 0;

    fileDescriptor = open(logFileFullName_.c_str(), O_RDWR, 0664);
    if (fileDescriptor < 0) {
        syslog(LOG_ERR, "%s File open fail", __FUNCTION__);
        return -errno;
    }
    int ret = fcntl(fileDescriptor, F_SETLK, &lock);
    if (ret < 0) {
        close(fileDescriptor);
        if(errno == EACCES || errno == EAGAIN) {
            return -EAGAIN;
        }
        syslog(LOG_ERR, "%s Can't acquire lock", __FUNCTION__);
        return -errno;
    } else {
        struct stat st;
        if (stat(logFileFullName_.c_str(), &st) == STAT_FAILURE) {
            close(fileDescriptor);
            return -errno;
        }
        if (inodeNumber_ != st.st_ino) {
            close(fileDescriptor);
            syslog(LOG_DEBUG, "%s Likely new tel.log file has been created", __FUNCTION__);
            // log file has been backup and recreated by another process
            // which acquire the lock firstly
            return -EAGAIN;
        }
    }
    return 0;
}

bool Logger::backupLogFile() {
    int ret, fileDescriptor;
    ret = acquireLock(fileDescriptor);
    if(ret < 0) {
        if(ret == -EAGAIN) {
            syslog(LOG_ERR, "%s File locked by another process", __FUNCTION__);
        } else {
            syslog(LOG_ERR, "%s File Lock Acquire failed", __FUNCTION__);
        }
        return false;
    } else {
        std::string backupFileName = logFileFullName_ + ".backup" ;
        rename(logFileFullName_.c_str(), backupFileName.c_str());
        std::ofstream logStream;
        mode_t mask = umask(UMASK_BITS);
        logStream.open(logFileFullName_.c_str(), std::ofstream::out | std::ofstream::trunc);
        logStream.close();
        umask(mask);
        close(fileDescriptor);
        return true;
    }
}

void Logger::logToFile(std::ostringstream & os) {
    lock_guard<mutex> guard(fileMutex_);

    bool logFileChanged = false;
    struct stat st;
    if (stat(logFileFullName_.c_str(), &st) == STAT_FAILURE) {
        return;
    }

    if(!logFileChanged) {
        if (st.st_size > logFileMaxSize_) {
            logFileChanged = backupLogFile();
        }
        if (inodeNumber_ != st.st_ino) {
            logFileChanged = true;
        }
    }
    //Updating the iNode number after a successful backup.
    if(logFileChanged) {
        inodeNumber_ = reopenLogFile();
    }

    if(logFile_.rdstate() == std::ios_base::goodbit) {
        // Write the log message into the file
        logFile_ << os.str() << std::endl;
    } else {
        syslog(LOG_NOTICE, "%s", os.str().c_str());
    }
}

void Logger::logToConsole(std::ostringstream & os) {
    std::cout << os.str() << "\n";
}

void Logger::logToSyslog(std::ostringstream & os, LogLevel logLevel) {
   std::string logMessage = os.str();
   switch(logLevel) {
      /*
       * Mapping of log levels in syslog
       * Error logs are mapped to LOG_ERR.
       * Warning logs are mapped to LOG_WARNING.
       * Info logs are mapped to LOG_INFO.
       * Debug logs are mapped to LOG_DEBUG.
       */

      case LogLevel::LEVEL_ERROR:
         syslog(LOG_ERR, "%s", logMessage.c_str());
         break;
      case LogLevel::LEVEL_WARNING:
         syslog(LOG_WARNING, "%s", logMessage.c_str());
         break;
      case LogLevel::LEVEL_INFO:
         syslog(LOG_INFO, "%s", logMessage.c_str());
         break;
      case LogLevel::LEVEL_DEBUG:
         syslog(LOG_DEBUG, "%s", logMessage.c_str());
         break;
      default:
         break;
   }
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

bool Logger::isSyslogLoggingEnabled() {
    if(loggerType_ == LoggerType::SYSLOG_LOG)
        return true;
    return false;
}
