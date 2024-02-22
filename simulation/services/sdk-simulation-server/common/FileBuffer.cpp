/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "FileBuffer.hpp"

#include <fstream>
#include <sstream>
#include <chrono>
#include <unistd.h>
#include <thread>

FileBuffer::~FileBuffer() {
    cleanup();
}

FileBuffer::FileBuffer(std::string filePath, int thresholdValue) {
    fileName_ = filePath;
    threshold_ = thresholdValue;
}

void FileBuffer::startBuffering() {
    LOG(DEBUG, __FUNCTION__);
    auto f = std::async(std::launch::async,
        [=]() {
            this->startBufferingSync();
        }).share();
    taskQ_.add(f);
}

void FileBuffer::startBufferingSync() {
    LOG(DEBUG, __FUNCTION__);
    std::ifstream ifs(fileName_);
    if(!ifs.is_open()) {
        LOG(ERROR, __FUNCTION__, "Could not open the file: ", fileName_);
        return;
    }
    if(ifs.good()) {
        LOG(DEBUG, " Begin Buffering ", fileName_);
    }
    while(true) {
        int lineCount = 0;
        std::string line = "";
        while(lineCount < threshold_ && ifs.peek() != EOF) {
            std::getline(ifs, line);
            readBuffer_.push_back(line);
            lineCount++;
        }
        if(ifs.peek() == EOF) {
            LOG(DEBUG, " Reached EOF ", fileName_);
            ifs.close();
            reachedEOF_ = true;
            /**
             * The last row of the csv sheet is garbled data since the recording utility terminates
             * abruptly while retrieving the reports.
             */
            readBuffer_.pop_back();
            streamCurrentBatch_ = true;
            break;
        }
        //Current batch is available for streaming.
        {
            std::unique_lock<std::mutex> lck(streamBufferMtx_);
            streamCurrentBatch_ = true;
            streamBufferCv_.notify_all();
        }
        //Wait until the signal for reading the next batch is received.
        std::unique_lock<std::mutex> lck(nextBatchBufferMtx_);
        readNextBatch_ = false;
        nextBatchBufferCv_.wait(lck, [this]{ return readNextBatch_; });
        streamCurrentBatch_ = false;
    }
}

//Will be invoked by the streaming thread.
bool FileBuffer::getNextBuffer(std::vector<std::string> &requestBuffer) {
    LOG(DEBUG, __FUNCTION__);
    if(requestBuffer.empty()) {
        //Wait for read buffer readiness.
        {
            std::unique_lock<std::mutex> lck(streamBufferMtx_);
            streamBufferCv_.wait(lck, [this]{ return streamCurrentBatch_; });
        }
        // Swap read buffer and signal buffering thread.
        std::unique_lock<std::mutex> lck(nextBatchBufferMtx_);
        requestBuffer.swap(readBuffer_);
        readNextBatch_ = true;
        nextBatchBufferCv_.notify_all();
    }
    if(reachedEOF_ && requestBuffer.empty()) {
        return false;
    }
    return true;
}

void FileBuffer::cleanup() {
    LOG(DEBUG, __FUNCTION__);
}