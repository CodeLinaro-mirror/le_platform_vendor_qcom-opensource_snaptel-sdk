/*
 *  Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <telux/data/DataDefines.hpp>
#include "common/Logger.hpp"
#include "protos/proto-src/data_simulation.grpc.pb.h"

extern "C" {
#include <arpa/inet.h>
}

class DataUtilsStub {
public:
    static ::dataStub::TechPreference::TechPref convertTechPrefStringToEnum(
        std::string techPref) {
        LOG(DEBUG, __FUNCTION__);

        ::dataStub::TechPreference::TechPref enumTechPref;
        if (techPref == "TP_3GPP") {
            enumTechPref = ::dataStub::TechPreference::TP_3GPP;
        } else if (techPref == "TP_3GPP2") {
            enumTechPref = ::dataStub::TechPreference::TP_3GPP2;
        } else if (techPref == "TP_ANY") {
            enumTechPref = ::dataStub::TechPreference::TP_ANY;
        } else {
            enumTechPref = ::dataStub::TechPreference::UNKNOWN;
        }
        return enumTechPref;
    }

    static ::dataStub::IpFamilyType::Type convertIpFamilyStringToEnum(
        std::string ipFamily) {
        LOG(DEBUG, __FUNCTION__);

        ::dataStub::IpFamilyType::Type ipFamilyType;
        if (ipFamily == "IPV4") {
            ipFamilyType = ::dataStub::IpFamilyType::IPV4;
        } else if (ipFamily == "IPV6") {
            ipFamilyType = ::dataStub::IpFamilyType::IPV6;
        } else if (ipFamily == "IPV4V6") {
            ipFamilyType = ::dataStub::IpFamilyType::IPV4V6;
        } else {
            ipFamilyType = ::dataStub::IpFamilyType::UNKNOWN;
        }
        return ipFamilyType;
    }

    static ::dataStub::AuthProtocolType::AuthProto convertAuthProtocolStringToEnum(
        std::string authProtocol) {
        LOG(DEBUG, __FUNCTION__);

        ::dataStub::AuthProtocolType::AuthProto authProtocolType;
        if (authProtocol == "AUTH_PAP") {
            authProtocolType = ::dataStub::AuthProtocolType::AUTH_PAP;
        } else if (authProtocol == "AUTH_CHAP") {
            authProtocolType = ::dataStub::AuthProtocolType::AUTH_CHAP;
        } else if (authProtocol == "AUTH_PAP_CHAP") {
            authProtocolType = ::dataStub::AuthProtocolType::AUTH_PAP_CHAP;
        } else {
            authProtocolType = ::dataStub::AuthProtocolType::AUTH_NONE;
        }
        return authProtocolType;
    }

    static std::string convertTechPrefEnumToString(
        ::dataStub::TechPreference::TechPref enumTechPref) {
        LOG(DEBUG, __FUNCTION__);

        std::string techPref;
        if (enumTechPref == ::dataStub::TechPreference::TP_3GPP) {
            techPref = "TP_3GPP";
        } else if (enumTechPref == ::dataStub::TechPreference::TP_3GPP2) {
            techPref = "TP_3GPP2";
        } else if (enumTechPref == ::dataStub::TechPreference::TP_ANY) {
            techPref = "TP_ANY";
        } else {
            techPref = "Unknown";
        }
        LOG(DEBUG, __FUNCTION__, " TechPreference is :",  techPref);
        return techPref;
    }

    static std::string convertIpFamilyEnumToString(
        ::dataStub::IpFamilyType::Type ipFamilyType) {
        LOG(DEBUG, __FUNCTION__);

        std::string ipFamily;
        if (ipFamilyType == ::dataStub::IpFamilyType::IPV4) {
            ipFamily = "IPV4";
        } else if (ipFamilyType == ::dataStub::IpFamilyType::IPV6) {
            ipFamily = "IPV6";
        } else if (ipFamilyType == ::dataStub::IpFamilyType::IPV4V6) {
            ipFamily = "IPV4V6";
        } else {
            ipFamily = "Unknown";
        }
        LOG(DEBUG, __FUNCTION__, " ipFamily is :",  ipFamily);
        return ipFamily;
    }

    static std::string convertAuthProtocolEnumToString(
            ::dataStub::AuthProtocolType::AuthProto authProtocolType) {
        LOG(DEBUG, __FUNCTION__);

        std::string authProtocol;
        if (authProtocolType == ::dataStub::AuthProtocolType::AUTH_PAP) {
            authProtocol = "AUTH_PAP";
        } else if (authProtocolType == ::dataStub::AuthProtocolType::AUTH_CHAP) {
            authProtocol = "AUTH_CHAP";
        } else if (authProtocolType == ::dataStub::AuthProtocolType::AUTH_PAP_CHAP) {
            authProtocol = "AUTH_PAP_CHAP";
        } else {
            authProtocol = "AUTH_NONE";
        }
        LOG(DEBUG, __FUNCTION__, " authProtocol is :",  authProtocol);
        return authProtocol;
    }

    static bool isValidIpv4Address(const std::string &addr) {
        struct sockaddr_in sa;
        int res = inet_pton(AF_INET, addr.c_str(), &(sa.sin_addr));
        return res != 0;
    }

    static bool isValidIpv6Address(const std::string &addr) {
        struct sockaddr_in6 sa;
        int res = inet_pton(AF_INET6, addr.c_str(), &(sa.sin6_addr));
        return res != 0;
    }
};