/*
 *  Copyright (c) 2019, The Linux Foundation. All rights reserved.
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

#include "Cv2xUtils.hpp"

int Cv2xUtils::DataCallEndReasonToInt(DataCallEndReason reason)
{
    switch (reason.type) {
        case EndReasonType::CE_UNKNOWN:
            return -1;
        case EndReasonType::CE_MOBILE_IP:
            return static_cast<int>(reason.IpCode);
        case EndReasonType::CE_INTERNAL:
            return static_cast<int>(reason.internalCode);
        case EndReasonType::CE_CALL_MANAGER_DEFINED:
            return static_cast<int>(reason.cmCode);
        case EndReasonType::CE_3GPP_SPEC_DEFINED:
            return static_cast<int>(reason.specCode);
        case EndReasonType::CE_PPP:
            return static_cast<int>(reason.pppCode);
        case EndReasonType::CE_EHRPD:
            return static_cast<int>(reason.ehrpdCode);
        case EndReasonType::CE_IPV6:
            return static_cast<int>(reason.ipv6Code);
        default:
            return -1;
    }
}

std::string Cv2xUtils::IpFamilyTypeToStr(IpFamilyType type)
{
    switch (type) {
        case IpFamilyType::UNKNOWN:
            return "UNKNOWN";
        case IpFamilyType::IPV4:
            return "IPV4";
        case IpFamilyType::IPV6:
            return "IPV6";
        case IpFamilyType::IPV4V6:
            return "IPV4V6";
        default:
            return "UNKNOWN";
    }
}

std::string Cv2xUtils::DataCallStatusToStr(DataCallStatus status)
{
    switch (status) {
        case DataCallStatus::INVALID:
            return "NET_INVALID";
        case DataCallStatus::NET_CONNECTED:
            return "NET_CONNECTED";
        case DataCallStatus::NET_NO_NET:
            return "NET_NO_NET";
        case DataCallStatus::NET_IDLE:
            return "NET_IDLE";
        case DataCallStatus::NET_CONNECTING:
            return "NET_CONNECTING";
        case DataCallStatus::NET_DISCONNECTING:
            return "NET_DISCONNECTING";
        case DataCallStatus::NET_RECONFIGURED:
            return "NET_RECONFIGURED";
        case DataCallStatus::NET_NEWADDR:
            return "NET_NEWADDR";
        case DataCallStatus::NET_DELADDR:
            return "NET_DELADDR";
        default:
            return "NET_INVALID";
    }
}



