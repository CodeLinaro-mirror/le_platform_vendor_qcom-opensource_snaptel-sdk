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
 * @file       DiagUtils.cpp
 *
 * @brief      This class class performs common functions in Diag.
 */

#include <regex>
#include "DiagUtils.hpp"

bool DiagUtils::isString(std::string input) {
    if (!input.empty()) {
        // std::regex pattern("([a-zA-Z_0-9]+/?)+");
        for ( uint8_t i = 0; i < input.length(); i++){
            if (isspace(input[i])){
                std::cout << "ERROR: Can not use whitespaces." << std::endl;
                return false;
            }
        }
        return true;
    }
    return false;
}

std::string DiagUtils::validateString(std::string input) {
    bool valid = false;
    do {
        //If user input is within the list, just exist
        if ((std::cin.good()) && (isString(input)))
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
            valid = isString(input);
        }
    } while (!valid);
    return input;
}

bool DiagUtils::isNumber(std::string input) {
    std::regex pattern("-?[0-9]+");
    if (regex_match(input,pattern)){
        return true;
    }
    return false;
}

int DiagUtils::validateNumber(std::string input) {
    bool valid = false;
    do {
        //If user input is within the list, just exist
        if ((std::cin.good()) && (isNumber(input)))
        {
            valid = true;
        }
        else {
            //User input does not match any of the possible entries
            std::cin.clear();
            // Extracts characters from the previous input sequence and discards them,
            // until entire stream have been extracted, or one compares equal to newline.
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << std::endl << "ERROR: Invalid input, please re-enter: ";
            std::cin >> input;
            valid = isNumber(input);
        }
    } while (!valid);
    return stoi(input);
}

telux::platform::diag::DiagLogMode DiagUtils::setMode(int mode) {
    switch (mode) {
        case 0:
            return telux::platform::diag::DiagLogMode::STREAMING;
        case 1:
            return telux::platform::diag::DiagLogMode::THRESHOLD;
        case 2:
            return telux::platform::diag::DiagLogMode::CIRCULAR_BUFFER;
        default:
            return telux::platform::diag::DiagLogMode::STREAMING;
    }
}

std::string DiagUtils::toString(telux::platform::diag::DiagConfig config) {
    std::string configStruct =
    "Source Type :" + std::to_string(static_cast<int>(config.srcType));
    if(config.srcType == telux::platform::diag::SourceType::DEVICE) {
        configStruct += "\nSource Info :" + std::to_string(static_cast<int>(config.srcInfo.device));
    } else {
        configStruct +=
        "\nSource :" + std::to_string(static_cast<int>(config.srcInfo.peripheral));
    }
    configStruct += "\nmdmLogMaskPath : " + config.mdmLogMaskFile
    + "\neapLogMaskPath : " + config.eapLogMaskFile
    + "\nmodeType : " + std::to_string(static_cast<int>(config.modeType))
    + "\nLogMethod : " + std::to_string(static_cast<int>(config.method))
    + "\nmaxSize : " + std::to_string(config.methodConfig.fileConfig.maxSize)
    + "\nmaxNumber : " + std::to_string(config.methodConfig.fileConfig.maxNumber) + "\n";
    return configStruct;
}

void DiagUtils::printConfig(telux::platform::diag::DiagConfig config) {
    if(config.srcType == telux::platform::diag::SourceType::DEVICE) {
        std::cout << "Source Type: DEVICE" << std::endl;
        if(config.srcInfo.device) {
            if(config.srcInfo.device & telux::platform::diag::DeviceType::DIAG_DEVICE_EXTERNAL_AP) {
                std::cout << "Selected Device: External AP" << std::endl;
            }
            if(config.srcInfo.device & telux::platform::diag::DeviceType::DIAG_DEVICE_MDM) {
                std::cout << "Selected Device: MDM" << std::endl;
            }
        } else {
            std::cout << "Device: NONE" << std::endl;
        }
    } else if(config.srcType == telux::platform::diag::SourceType::PERIPHERAL) {
        std::cout << "Source Type: PERIPHERAL" << std::endl;
        if(config.srcInfo.peripheral) {
            if(config.srcInfo.peripheral &
               telux::platform::diag::PeripheralType::DIAG_PERIPHERAL_INTEGRATED_AP) {
                std::cout << "Selected Peripheral: Application Processor" << std::endl;
            }
            if(config.srcInfo.peripheral &
               telux::platform::diag::PeripheralType::DIAG_PERIPHERAL_MODEM_DSP) {
                std::cout << "Selected Peripheral: Modem DSP" << std::endl;
            }
            if(config.srcInfo.peripheral &
               telux::platform::diag::PeripheralType::DIAG_PERIPHERAL_SVM) {
                std::cout << "Selected Peripheral: SVMs" << std::endl;
            }
            if(config.srcInfo.peripheral &
               telux::platform::diag::PeripheralType::DIAG_PERIPHERAL_LPASS) {
                std::cout << "Selected Peripheral: LPAS" << std::endl;
            }
            if(config.srcInfo.peripheral &
               telux::platform::diag::PeripheralType::DIAG_PERIPHERAL_CDSP) {
                std::cout << "Selected Peripheral: CDSP" << std::endl;
            }
        }
    }else {
        std::cout << "Source Type: NONE" << std::endl;
        std::cout << "Device: NONE" << std::endl;
        std::cout << "Peripheral: NONE" << std::endl;
    }
    std::cout << "EAP Log Mask Path: " <<  config.eapLogMaskFile << std::endl;
    std::cout << "MDM Log Mask Path: " <<  config.mdmLogMaskFile << std::endl;

    if(config.modeType == telux::platform::diag::DiagLogMode::STREAMING) {
        std::cout << "Mode Type: Streaming" << std::endl;
    } else if(config.modeType == telux::platform::diag::DiagLogMode::THRESHOLD) {
        std::cout << "Mode Type: Threshold" << std::endl;
    } else if(config.modeType == telux::platform::diag::DiagLogMode::CIRCULAR_BUFFER) {
        std::cout << "Mode Type: Circular Buffer" << std::endl;
    } else {
        std::cout << "Mode Type: Unknown" << std::endl;
    }
    if(config.method == telux::platform::diag::LogMethod::FILE) {
        std::cout << "Log Method: File" << std::endl;
    } else if(config.method == telux::platform::diag::LogMethod::CALLBACK) {
        std::cout << "Log Method: Callback" << std::endl;
    } else {
        std::cout << "Log Method: None" << std::endl;
    }
    std::cout << "Max File Size: "
              <<  std::to_string(config.methodConfig.fileConfig.maxSize) << std::endl;
    std::cout << "Max Number of Files: "
              << std::to_string(config.methodConfig.fileConfig.maxNumber) << std::endl;
}

void DiagUtils::convertIntToPeripheral(
    int peripheralInt, telux::platform::diag::Peripherals& peripherals) {
        uint32_t tmpMask = 0;
    switch(peripheralInt) {
        case 7:
            tmpMask = (telux::platform::diag::PeripheralType::DIAG_PERIPHERAL_SVM |
                       telux::platform::diag::PeripheralType::DIAG_PERIPHERAL_MODEM_DSP |
                       telux::platform::diag::PeripheralType::DIAG_PERIPHERAL_INTEGRATED_AP);
            break;
        case 6:
            tmpMask = (telux::platform::diag::PeripheralType::DIAG_PERIPHERAL_SVM |
                       telux::platform::diag::PeripheralType::DIAG_PERIPHERAL_MODEM_DSP);
            break;
        case 5:
            tmpMask = (telux::platform::diag::PeripheralType::DIAG_PERIPHERAL_SVM |
                       telux::platform::diag::PeripheralType::DIAG_PERIPHERAL_INTEGRATED_AP);
            break;
        case 4:
            tmpMask = telux::platform::diag::PeripheralType::DIAG_PERIPHERAL_SVM;
            break;
        case 3:
            tmpMask = (telux::platform::diag::PeripheralType::DIAG_PERIPHERAL_MODEM_DSP |
                       telux::platform::diag::PeripheralType::DIAG_PERIPHERAL_INTEGRATED_AP);
            break;
        case 2:
            tmpMask = telux::platform::diag::PeripheralType::DIAG_PERIPHERAL_MODEM_DSP;
            break;
        case 1:
            tmpMask = telux::platform::diag::PeripheralType::DIAG_PERIPHERAL_INTEGRATED_AP;
            break;
        default:
            break;
    }
    peripherals = static_cast<telux::platform::diag::Peripherals>(tmpMask);
}

