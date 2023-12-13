/*
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "DataUtilsStub.hpp"
#include "../common/Logger.hpp"

::dataStub::TechPreference::TechPref DataUtilsStub::convertTechPrefStringToEnum(std::string techPref) {
    ::dataStub::TechPreference::TechPref enumTechPref;
    LOG(DEBUG, __FUNCTION__);
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

::dataStub::IpFamilyType::Type DataUtilsStub::convertIpFamilyStringToEnum(std::string ipFamily) {
    ::dataStub::IpFamilyType::Type ipFamilyType;
    LOG(DEBUG, __FUNCTION__);
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

::dataStub::AuthProtocolType::AuthProto DataUtilsStub::convertAuthProtocolStringToEnum(std::string authProtocol) {
    ::dataStub::AuthProtocolType::AuthProto authProtocolType;
    LOG(DEBUG, __FUNCTION__);
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

std::string DataUtilsStub::convertTechPrefEnumToString(::dataStub::TechPreference::TechPref enumTechPref) {
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

std::string DataUtilsStub::convertIpFamilyEnumToString(::dataStub::IpFamilyType::Type ipFamilyType) {
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

std::string DataUtilsStub::convertAuthProtocolEnumToString(
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