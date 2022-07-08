/*
 *  Copyright (c) 2019-2021, The Linux Foundation. All rights reserved.
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

/*
 *  Changes from Qualcomm Innovation Center are provided under the following license:

 *  Copyright (c) 2021-2022 Qualcomm Innovation Center, Inc. All rights reserved.
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

#ifndef DATAUTILS_HPP
#define DATAUTILS_HPP

#include <telux/data/DataFactory.hpp>
#include <telux/data/DataConnectionManager.hpp>
#include <telux/data/ServingSystemManager.hpp>

class DataUtils {
public:
    // Validate the input and in case of invalid input request
    // for proper input from user.
    template <typename T>
    static bool isInputValid(T input, std::initializer_list<T> list) {
        for (auto elem = list.begin(); elem != list.end(); ++elem)
        {
            if (*elem == input)
            {
                return true;
            }
        }
        return false;
    }

    template <typename T>
    static void validateInput(T &input, std::initializer_list<T> list) {
        bool valid = false;
        do {
            //If user input is within the list, just exist
            if ((std::cin.good()) && (isInputValid(input, list)))
            {
                valid = true;
            }
            else {
                //User input does not match any of the possible entries
                std::cin.clear();
                // Extracts characters from the previous input sequence and discards them,
                // until entire stream have been extracted, or one compares equal to newline.
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                std::cout << "ERROR: Invalid input, please re-enter." << std::endl;
                std::cin >> input;
                valid = isInputValid(input, list);
            }
        } while (!valid);
    }

   static std::string callEndReasonTypeToString(telux::data::EndReasonType type);
   static int callEndReasonCode(telux::data::DataCallEndReason ceReason);
   static std::string techPreferenceToString(telux::data::TechPreference techPref);
   static std::string ipFamilyTypeToString(telux::data::IpFamilyType ipType);
   static std::string dataCallStatusToString(telux::data::DataCallStatus dcStatus);
   static std::string usageResetReasonToString(telux::data::UsageResetReason usageResetReason);
   static std::string bearerTechToString(telux::data::DataBearerTechnology bearerTech);
   static std::string operationTypeToString(telux::data::OperationType oprType);
   static std::string protocolToString(telux::data::IpProtocol proto);
   static telux::data::IpProtocol getProtcol(std::string protoStr);
   static std::string drbStatusToString(telux::data::DrbStatus stat);
   static std::string serviceRatToString(telux::data::NetworkRat rat);
};

#endif  // DATAUTILS_HPP
