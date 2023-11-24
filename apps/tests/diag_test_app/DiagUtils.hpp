/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *    * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *
 *   * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
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
 * DiagUtility helper class
 * @brief DiagUtils class performs common functions in Wlan.
 */

#ifndef DIAGUTILS_HPP
#define DIAGUTILS_HPP

#include <iostream>
#include <string>
#include <telux/common/CommonDefines.hpp>
#include <telux/platform/diag/DiagLogManager.hpp>

class DiagUtils {
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
    static bool isString(std::string input);
    static std::string validateString(std::string input);
    static bool isNumber(std::string input);
    static int validateNumber(std::string input);
    static telux::platform::diag::DiagLogMode setMode(int mode);
    static std::string toString(telux::platform::diag::DiagConfig Config);
    static void convertIntToPeripheral(
      int peripheralInt, telux::platform::diag::Peripherals& peripherals);
    static void printConfig(telux::platform::diag::DiagConfig config);
};

#endif