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
#include <string>

#include "CellbroadcastListener.hpp"

#define PRINT_NOTIFICATION std::cout << "\033[1;35mNOTIFICATION: \033[0m"

void CellbroadcastListener::onIncomingMessage(
    const std::shared_ptr<telux::tel::CellBroadcastMessage> cbMessage) {

    if(cbMessage->getMessageType() == telux::tel::MessageType::ETWS) {
        std::shared_ptr<telux::tel::EtwsInfo> etwsInfo = cbMessage->getEtwsWarningInfo();
        if(etwsInfo) {
            std::string languageCode =
                (etwsInfo->getLanguageCode() == "") ? "UNAVAILABLE": etwsInfo->getLanguageCode();
            PRINT_NOTIFICATION << " Incoming Cellbroadcast Message:" <<
                " \nETWS Info: " << " \nGeographical Scope: " << geograhicalScopeToString(
                etwsInfo->getGeographicalScope()) << " \nMessage Identifier: " <<
                etwsInfo->getMessageId() << " \nSerial Number: " << etwsInfo->getSerialNumber() <<
                " \nLanguage code: " << languageCode <<
                " \nMessage code: " << etwsInfo->getMessageCode() <<
                " \nUpdate number: " << etwsInfo->getUpdateNumber() <<
                " \nMessage: " << etwsInfo->getMessageBody() <<" \nPriority: " <<
                priorityToString(etwsInfo->getPriority()) <<
                " \nisEmergencyUserAlert: " << etwsInfo->isEmergencyUserAlert() <<
                " \nisPopupAlert: " << etwsInfo->isPopupAlert() << " \nisPrimary: " <<
                etwsInfo->isPrimary() << " \nWarningType: " <<
                etwsWarningTypeToString(etwsInfo->getEtwsWarningType()) << std::endl;
        } else {
            PRINT_NOTIFICATION << " ETWS info is null " << std::endl;
        }
    } else if(cbMessage->getMessageType() == telux::tel::MessageType::CMAS) {
        std::shared_ptr<telux::tel::CmasInfo> cmasInfo = cbMessage->getCmasWarningInfo();
        if(cmasInfo) {
            std::string languageCode =
                (cmasInfo->getLanguageCode() == "") ? "UNAVAILABLE": cmasInfo->getLanguageCode();
            PRINT_NOTIFICATION << " Incoming Cellbroadcast Message:" <<
                " \nCMAS Info: " << " \nGeographical Scope: " << geograhicalScopeToString(
                cmasInfo->getGeographicalScope()) << " \nMessage Identifier: " <<
                cmasInfo->getMessageId() << " \nSerial Number: " << cmasInfo->getSerialNumber() <<
                " \nLanguage code: " << languageCode <<
                " \nMessage code: " << cmasInfo->getMessageCode() <<
                " \nUpdate number: " << cmasInfo->getUpdateNumber() <<
                " \nMessage: " << cmasInfo->getMessageBody() <<" \nPriority: " <<
                priorityToString(cmasInfo->getPriority()) << " \nCmasMessageClass: " <<
                cmasMessageClassToString(cmasInfo->getMessageClass()) << " \nCmasSeverity: " <<
                cmasSeverityToString(cmasInfo->getSeverity()) << " \nCmasUrgency: " <<
                cmasUrgencyToString(cmasInfo->getUrgency()) << " \nCmasCertainty: " <<
                cmasCertaintyToString(cmasInfo->getCertainty()) << std::endl;
        } else {
            PRINT_NOTIFICATION << " CMAS info is null " << std::endl;
        }
    }
}

std::string CellbroadcastListener::geograhicalScopeToString(telux::tel::GeographicalScope scope) {
    switch(scope) {
        case telux::tel::GeographicalScope::CELL_WIDE_IMMEDIATE:
            return "CELL_WIDE_IMMEDIATE";
        case telux::tel::GeographicalScope::PLMN_WIDE:
            return "PLMN_WIDE";
        case telux::tel::GeographicalScope::LA_WIDE:
            return "LA_WIDE";
        case telux::tel::GeographicalScope::CELL_WIDE:
            return "CELL_WIDE";
        default:
            return "UNKNOWN";
    }
}

std::string CellbroadcastListener::priorityToString(telux::tel::MessagePriority priority) {
    switch(priority) {
        case telux::tel::MessagePriority::UNKNOWN:
            return "UNKNOWN";
        case telux::tel::MessagePriority::NORMAL:
            return "NORMAL";
        case telux::tel::MessagePriority::EMERGENCY:
            return "EMERGENCY";
        default:
            return  "UNKNOWN Priority";
    }
}

std::string CellbroadcastListener::msgTypeToString(telux::tel::MessageType type) {
    switch(type) {
        case telux::tel::MessageType::UNKNOWN:
            return "UNKNOWN";
        case telux::tel::MessageType::ETWS:
            return "ETWS";
        case telux::tel::MessageType::CMAS:
            return "CMAS";
        default:
            return  "UNKNOWN";
    }
}

std::string CellbroadcastListener::etwsWarningTypeToString(telux::tel::EtwsWarningType warningtype) {
    switch(warningtype) {
        case telux::tel::EtwsWarningType::UNKNOWN:
            return "UNKNOWN";
        case telux::tel::EtwsWarningType::EARTHQUAKE:
            return "EARTHQUAKE";
        case telux::tel::EtwsWarningType::TSUNAMI:
            return "TSUNAMI";
        case telux::tel::EtwsWarningType::EARTHQUAKE_AND_TSUNAMI:
            return "EARTHQUAKE_AND_TSUNAMI";
        case telux::tel::EtwsWarningType::TEST_MESSAGE:
            return "TEST_MESSAGE";
        case telux::tel::EtwsWarningType::OTHER_EMERGENCY:
            return "OTHER_EMERGENCY";
        default:
            return  "UNKNOWN";
    }
}

std::string CellbroadcastListener::cmasMessageClassToString(telux::tel::CmasMessageClass msgClass) {
    switch(msgClass) {
        case telux::tel::CmasMessageClass::UNKNOWN:
            return "UNKNOWN";
        case telux::tel::CmasMessageClass::PRESIDENTIAL_LEVEL_ALERT:
            return "PRESIDENTIAL_LEVEL_ALERT";
        case telux::tel::CmasMessageClass::EXTREME_THREAT:
            return "EXTREME_THREAT";
        case telux::tel::CmasMessageClass::SEVERE_THREAT:
            return "SEVERE_THREAT";
        case telux::tel::CmasMessageClass::CHILD_ABDUCTION_EMERGENCY:
            return "CHILD_ABDUCTION_EMERGENCY";
        case telux::tel::CmasMessageClass::REQUIRED_MONTHLY_TEST:
            return "REQUIRED_MONTHLY_TEST";
        case telux::tel::CmasMessageClass::CMAS_EXERCISE:
            return "CMAS_EXERCISE";
        case telux::tel::CmasMessageClass::OPERATOR_DEFINED_USE:
            return "OPERATOR_DEFINED_USE";
        default:
            return  "UNKNOWN";
    }
}

std::string CellbroadcastListener::cmasSeverityToString(telux::tel::CmasSeverity severity) {
    switch(severity) {
        case telux::tel::CmasSeverity::UNKNOWN:
            return "UNKNOWN";
        case telux::tel::CmasSeverity::EXTREME:
            return "EXTREME";
        case telux::tel::CmasSeverity::SEVERE:
            return "SEVERE";
        default:
            return  "UNKNOWN";
    }
}

std::string CellbroadcastListener::cmasUrgencyToString(telux::tel::CmasUrgency urgency) {
    switch(urgency) {
        case telux::tel::CmasUrgency::UNKNOWN:
            return "UNKNOWN";
        case telux::tel::CmasUrgency::IMMEDIATE:
            return "IMMEDIATE";
        case telux::tel::CmasUrgency::EXPECTED:
            return "EXPECTED";
        default:
            return  "UNKNOWN";
    }
}

std::string CellbroadcastListener::cmasCertaintyToString(telux::tel::CmasCertainty certainity) {
    switch(certainity) {
        case telux::tel::CmasCertainty::UNKNOWN:
            return "UNKNOWN";
        case telux::tel::CmasCertainty::OBSERVED:
            return "OBSERVED";
        case telux::tel::CmasCertainty::LIKELY:
            return "LIKELY";
        default:
            return  "UNKNOWN";
    }
}

