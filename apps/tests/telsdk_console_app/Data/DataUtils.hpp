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
 * Changes from Qualcomm Technologies, Inc. are provided under the following license:
 *
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef DATAUTILS_HPP
#define DATAUTILS_HPP

#include <telux/data/DataFactory.hpp>
#include <telux/data/DataConnectionManager.hpp>
#include <telux/data/ServingSystemManager.hpp>
#include <telux/data/DataHealthManager.hpp>
#include "../../../common/utils/Utils.hpp"

#define PROTO_TCP 6
#define PROTO_UDP 17
#define PROTO_TCP_UDP 253
class DataUtils {
public:

    template <typename T>
    static void validateInput(T &input, std::initializer_list<T> list) {
        return Utils::validateInput(input, list);
    }

   static std::string callEndReasonTypeToString(telux::data::EndReasonType type);
   static int callEndReasonCode(telux::data::DataCallEndReason ceReason);
   static std::string techPreferenceToString(telux::data::TechPreference techPref);
   static std::string ipFamilyTypeToString(telux::data::IpFamilyType ipType);
   static std::string dataCallStatusToString(telux::data::DataCallStatus dcStatus);
   static std::string bearerTechToString(telux::data::DataBearerTechnology bearerTech);
   static std::string operationTypeToString(telux::data::OperationType oprType);
   static std::string protocolToString(telux::data::IpProtocol proto);
   static telux::data::IpProtocol getProtcol(std::string protoStr);
   static std::string drbStatusToString(telux::data::DrbStatus stat);
   static std::string serviceRatToString(telux::data::NetworkRat rat);
   static std::string backhaulToString(telux::data::BackhaulType backhaul);
   //Retuns true if multiple backhauls are supported
   static bool populateBackhaulInfo(telux::data::BackhaulInfo& backhaulInfo);
   static std::string vlanInterfaceToString(telux::data::InterfaceType interface);
   static std::string trafficClassToString(telux::data::IpTrafficClassType tc);
   static std::string flowStateEventToString(telux::data::QosFlowStateChangeEvent state);

   static std::string networkModuleToString(telux::data::DataStallNetworkModule module);
   static std::string stallReasonToString(telux::data::DataStallReason reason);
   static std::string recoveryActionToString(telux::data::DataStallRecoveryAction action);
   static std::string recoveryResultToString(telux::data::DataStallRecoveryResult result);
   static std::string disablementReasonToString(telux::data::DataStallDisablementReason reason);
   static std::string restartTimerStatusToString(telux::data::DataStallRestartTimerStatus status);
   static void printDataStallConfig(const telux::data::DataStallConfig &config);

   static void logQosDetails(std::shared_ptr<telux::data::TrafficFlowTemplate> &tft);
   static void printFilterDetails(std::shared_ptr<telux::data::IIpFilter> filter);
};

#endif  // DATAUTILS_HPP
