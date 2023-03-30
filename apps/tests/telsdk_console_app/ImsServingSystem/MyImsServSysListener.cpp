/*
 *  Copyright (c) 2021, The Linux Foundation. All rights reserved.
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
 *
 *  Copyright (c) 2021,2023 Qualcomm Innovation Center, Inc. All rights reserved.
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


#include <iostream>
#include <string>

#include "MyImsServSysListener.hpp"
#include "../Phone/MyPhoneListener.hpp"
#include "Utils.hpp"

#define PRINT_NOTIFICATION std::cout << "\033[1;35mNOTIFICATION: \033[0m"
#define PRINT_CB std::cout << "\033[1;35mCallback: \033[0m"

// Implementation of IMS Registration State callback
void MyImsServSysCallback::imsRegStateResponse(SlotId slotId,
    telux::tel::ImsRegistrationInfo status, telux::common::ErrorCode error) {
    std::cout << " Request IMS Registration status response received on slotId "
            << static_cast<int>(slotId) << std::endl;
    if(error == telux::common::ErrorCode::SUCCESS) {
        PRINT_CB << "IMS Registration Status: "
            << MyImsServSysListener::convertRegStatustoString(status.imsRegStatus)
                << ", radio technology: "
                << MyPhoneHelper::radioTechToString(status.rat)
                << ", error code: " << status.errorCode << ", error description: "
                << status.errorString << std::endl;
    } else {
        PRINT_CB << "requestRegistrationInfo failed, errorCode: " << static_cast<int>(error)
                << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
    }
}

// Implementation of IMS Service Status callback
void MyImsServSysCallback::imsServiceStatusResponse(SlotId slotId,
    telux::tel::ImsServiceInfo service, telux::common::ErrorCode error) {
    std::cout << "IMS service status response received on slotId "
            << static_cast<int>(slotId) << std::endl;
    if(error == telux::common::ErrorCode::SUCCESS) {
        PRINT_CB << "SMS Service Status over IMS: "
            << MyImsServSysListener::convertServiceStatustoString(service.sms)
            << ", Voice Service Status over IMS: "
            << MyImsServSysListener::convertServiceStatustoString(service.voice)
            << std::endl;
    } else {
        PRINT_CB << "requestServiceInfo failed, errorCode: " << static_cast<int>(error)
                << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
    }
}

MyImsServSysListener::MyImsServSysListener(SlotId slotId)
   : slotId_(slotId) {
}

void MyImsServSysListener::onImsRegStatusChange(telux::tel::ImsRegistrationInfo status) {
    PRINT_NOTIFICATION << "onImsRegStatusChange, SlotId: " << static_cast<int>(slotId_)
        << std::endl;
    PRINT_NOTIFICATION << "IMS Registration status changed to: "
                << convertRegStatustoString(status.imsRegStatus)
                << ", radio technology: " << MyPhoneHelper::radioTechToString(status.rat)
                << ", error code: " << status.errorCode
                << ", error description: " << status.errorString << std::endl;
}

void MyImsServSysListener::onImsServiceInfoChange(telux::tel::ImsServiceInfo service) {
    PRINT_NOTIFICATION << "onImsServiceInfoChange, SlotId: " << static_cast<int>(slotId_)
        << std::endl;
    PRINT_NOTIFICATION << "SMS Service Status over IMS: "
                << convertServiceStatustoString(service.sms)
                << ", Voice Service Status over IMS: "
                << convertServiceStatustoString(service.voice)
                << std::endl;
}

std::string MyImsServSysListener::convertRegStatustoString(telux::tel::RegistrationStatus state) {
    std::string stateString;

    switch(state) {
        case telux::tel::RegistrationStatus::NOT_REGISTERED:
            stateString = "NOT_REGISTERED";
            break;
        case telux::tel::RegistrationStatus::REGISTERING:
            stateString = "REGISTERING";
            break;
        case telux::tel::RegistrationStatus::REGISTERED:
            stateString = "REGISTERED";
            break;
        case telux::tel::RegistrationStatus::LIMITED_REGISTERED:
            stateString = "LIMITED_REGISTERED";
            break;
        default:
            stateString = "Unknown registration status";
            break;
    }

    return stateString;
}

std::string MyImsServSysListener::convertServiceStatustoString(
    telux::tel::CellularServiceStatus status) {
    std::string statusString;

    switch(status) {
        case telux::tel::CellularServiceStatus::NO_SERVICE:
            statusString = "NO_SERVICE";
            break;
        case telux::tel::CellularServiceStatus::LIMITED_SERVICE:
            statusString = "LIMITED_SERVICE";
            break;
        case telux::tel::CellularServiceStatus::FULL_SERVICE:
            statusString = "FULL_SERVICE";
            break;
        default:
            statusString = "Unknown service status";
            break;
    }

    return statusString;
}

void MyImsServSysListener::onServiceStatusChange(telux::common::ServiceStatus status) {
    std::string stat;

    switch(status) {
        case telux::common::ServiceStatus::SERVICE_AVAILABLE:
            stat = " SERVICE_AVAILABLE";
            break;
        case telux::common::ServiceStatus::SERVICE_UNAVAILABLE:
            stat =  " SERVICE_UNAVAILABLE";
            break;
        default:
            stat = " Unknown service status";
            break;
    }

    PRINT_NOTIFICATION << " Ims Settings onServiceStatusChange" << stat << "\n";
}
