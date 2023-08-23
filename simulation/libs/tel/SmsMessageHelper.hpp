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


/**
 * @file       SmsMessageHelper.hpp
 *
 * @brief
 *
 */

#ifndef SMS_MESSAGEHELPER_HPP
#define SMS_MESSAGEHELPER_HPP

#include "../common/Logger.hpp"
#include "../common/CsvHandler.hpp"
#include <telux/tel/SmsManager.hpp>

 enum TelEventType {
    UNKNOWN,
    SMS_MEMORY_FULL,
    SMS_INCOMING
};
class SmsHelper  {
public:
    static telux::tel::SmsTagType getTagType(std::string tagType ) {
        if(tagType == "MT_READ") {
            return telux::tel::SmsTagType::MT_READ;
        } else if (tagType == "MT_NOT_READ") {
            return telux::tel::SmsTagType::MT_NOT_READ;
        } else {
            return telux::tel::SmsTagType::UNKNOWN;
        }
    }

    static telux::tel::SmsEncoding getencodingMethod(std::string encoding ) {
        if(encoding == "GSM7") {
            return telux::tel::SmsEncoding::GSM7;
        } else if (encoding == "GSM8") {
            return telux::tel::SmsEncoding::GSM8;
        } else if (encoding == "UCS2") {
            return telux::tel::SmsEncoding::UCS2;
        } else {
            return telux::tel::SmsEncoding::UNKNOWN;
        }
    }

    static std::string convertVectorToString(std::vector<std::uint8_t> bytes) {
        std::stringstream ss;
        for (std::size_t i = 0; i < bytes.size(); i++)
        {
            ss <<  static_cast<int>(bytes[i]);
        }
        return ss.str();
    }

    static std::vector<int> convertStringToVector(std::string input) {
        std::stringstream iss( input );
        int parsednum;
        std::vector<int> myNumbers;
        while ( iss >> parsednum ) {
            myNumbers.push_back( parsednum );
        }
        return myNumbers;
    }
};


#endif // SMS_MESSAGEHELPER_HPP