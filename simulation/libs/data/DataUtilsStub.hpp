/*
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <telux/data/DataDefines.hpp>
#include "../../protos/proto-src/data.grpc.pb.h"

class DataUtilsStub {
public:
    static ::dataStub::TechPreference::TechPref convertTechPrefStringToEnum(
        std::string techPref);

    static ::dataStub::IpFamilyType::Type convertIpFamilyStringToEnum(
        std::string ipFamily);

    static ::dataStub::AuthProtocolType::AuthProto
        convertAuthProtocolStringToEnum(std::string authProtocol);

    static std::string convertTechPrefEnumToString(
        ::dataStub::TechPreference::TechPref enumTechPref);

    static std::string convertIpFamilyEnumToString(
        ::dataStub::IpFamilyType::Type ipFamilyType);

    static std::string convertAuthProtocolEnumToString(
        ::dataStub::AuthProtocolType::AuthProto authProtocolType);
};