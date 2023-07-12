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


#include "JsonParser.hpp"
#include "Logger.hpp"

#include <fstream>

std::mutex JsonParser::fileMutex_;

telux::common::ErrorCode JsonParser::readFromJsonFile(Json::Value &rootNode,
        std::string fileName) {
    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    std::ifstream ifs;
    std::lock_guard<std::mutex> lk(JsonParser::fileMutex_);
    ifs.open(fileName);
    try {
        ifs >> rootNode;
    } catch (std::exception &e) {
        LOG(ERROR, "Parsing the json file failed with ", e.what());
        error = telux::common::ErrorCode::INTERNAL_ERR;
    }
    ifs.close();
    return error;
}

telux::common::ErrorCode JsonParser::writeToJsonFile(Json::Value rootNode,
        std::string fileName) {
    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    std::ofstream ofs;
    std::lock_guard<std::mutex> lk(JsonParser::fileMutex_);
    ofs.open(fileName);
    ofs << rootNode;
    ofs.close();
    return error;
}
