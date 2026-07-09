/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * CellInfo  implementation
 */
#include "telux/tel/CellInfo.hpp"
#include "common/Logger.hpp"

#define INVALID_VALUE -1
namespace telux {
namespace tel {

bool CellInfo::isRegistered() {
    if (registered_ != 0) {
        LOG(DEBUG, " Current cell is registered: ", registered_);
        return true;
    } else {
        LOG(DEBUG, " Cell is not registered");
        return false;
    }
}

CellType CellInfo::getType() {
    return type_;
}

/**
 * GSM CellInfo  implementation
 */
GsmCellInfo::GsmCellInfo(int registered, GsmCellIdentity id, GsmSignalStrengthInfo ssInfo)
   : id_(id)
   , ssInfo_(ssInfo) {
    registered_ = registered;
    type_       = CellType::GSM;
}

GsmCellIdentity GsmCellInfo::getCellIdentity() {
    return id_;
}

GsmSignalStrengthInfo GsmCellInfo::getSignalStrengthInfo() {
    return ssInfo_;
}

GsmCellIdentity::GsmCellIdentity(
    std::string mcc, std::string mnc, int lac, int cid, int arfcn, int bsic)
   : mcc_(mcc)
   , mnc_(mnc)
   , lac_(lac)
   , cid_(cid)
   , arfcn_(arfcn)
   , bsic_(bsic) {
}

// GSM cell info
const std::string GsmCellIdentity::getMobileCountryCode() {
    return mcc_;
}

const std::string GsmCellIdentity::getMobileNetworkCode() {
    return mnc_;
}

const int GsmCellIdentity::getLac() {
    return lac_;
}

const int GsmCellIdentity::getIdentity() {
    return cid_;
}

const int GsmCellIdentity::getArfcn() {
    return arfcn_;
}

const int GsmCellIdentity::getBaseStationIdentityCode() {
    return bsic_;
}

/**
 * LTE CellInfo  implementation
 */
LteCellInfo::LteCellInfo(int registered, LteCellIdentity id, LteSignalStrengthInfo ssInfo)
   : id_(id)
   , ssInfo_(ssInfo) {
    registered_ = registered;
    type_       = CellType::LTE;
}

LteCellIdentity LteCellInfo::getCellIdentity() {
    return id_;
}

LteSignalStrengthInfo LteCellInfo::getSignalStrengthInfo() {
    return ssInfo_;
}

LteCellIdentity::LteCellIdentity(
    std::string mcc, std::string mnc, int ci, int pci, int tac, int earfcn)
   : mcc_(mcc)
   , mnc_(mnc)
   , ci_(ci)
   , pci_(pci)
   , tac_(tac)
   , earfcn_(earfcn) {
}
// LTE cell info
const std::string LteCellIdentity::getMobileCountryCode() {
    return mcc_;
}

const std::string LteCellIdentity::getMobileNetworkCode() {
    return mnc_;
}

const int LteCellIdentity::getIdentity() {
    return ci_;
}

const int LteCellIdentity::getPhysicalCellId() {
    return pci_;
}

const int LteCellIdentity::getTrackingAreaCode() {
    return tac_;
}

const int LteCellIdentity::getEarfcn() {
    return earfcn_;
}

/**
 * WCDMA CellInfo  implementation
 */

WcdmaCellInfo::WcdmaCellInfo(int registered, WcdmaCellIdentity id, WcdmaSignalStrengthInfo ssInfo)
   : id_(id)
   , ssInfo_(ssInfo) {
    registered_ = registered;
    type_       = CellType::WCDMA;
}

WcdmaCellIdentity WcdmaCellInfo::getCellIdentity() {
    return id_;
}

WcdmaSignalStrengthInfo WcdmaCellInfo::getSignalStrengthInfo() {
    return ssInfo_;
}

WcdmaCellIdentity::WcdmaCellIdentity(
    std::string mcc, std::string mnc, int lac, int cid, int psc, int uarfcn)
   : mcc_(mcc)
   , mnc_(mnc)
   , lac_(lac)
   , cid_(cid)
   , psc_(psc)
   , uarfcn_(uarfcn) {
}

// WCDMA cell info
const std::string WcdmaCellIdentity::getMobileCountryCode() {
    return mcc_;
}

const std::string WcdmaCellIdentity::getMobileNetworkCode() {
    return mnc_;
}

const int WcdmaCellIdentity::getLac() {
    return lac_;
}

const int WcdmaCellIdentity::getIdentity() {
    return cid_;
}

const int WcdmaCellIdentity::getPrimaryScramblingCode() {
    return psc_;
}

const int WcdmaCellIdentity::getUarfcn() {
    return uarfcn_;
}

/**
 * NR5G CellInfo  implementation
 */
Nr5gCellInfo::Nr5gCellInfo(int registered, Nr5gCellIdentity id, Nr5gSignalStrengthInfo ssInfo)
   : id_(id)
   , ssInfo_(ssInfo) {
    registered_ = registered;
    type_       = CellType::NR5G;
}

Nr5gCellIdentity Nr5gCellInfo::getCellIdentity() {
    return id_;
}

Nr5gSignalStrengthInfo Nr5gCellInfo::getSignalStrengthInfo() {
    return ssInfo_;
}

Nr5gCellIdentity::Nr5gCellIdentity(
    std::string mcc, std::string mnc, uint64_t ci, uint32_t pci, int32_t tac, int32_t arfcn)
   : mcc_(mcc)
   , mnc_(mnc)
   , ci_(ci)
   , pci_(pci)
   , tac_(tac)
   , arfcn_(arfcn) {
}
// NR5G cell info
const std::string Nr5gCellIdentity::getMobileCountryCode() {
    return mcc_;
}

const std::string Nr5gCellIdentity::getMobileNetworkCode() {
    return mnc_;
}

const uint64_t Nr5gCellIdentity::getIdentity() {
    return ci_;
}

const uint32_t Nr5gCellIdentity::getPhysicalCellId() {
    return pci_;
}

const int32_t Nr5gCellIdentity::getTrackingAreaCode() {
    return tac_;
}

const int32_t Nr5gCellIdentity::getArfcn() {
    return arfcn_;
}

/**
 * NB1 NTN CellInfo implementation
 */
Nb1NtnCellInfo::Nb1NtnCellInfo(
    int registered, Nb1NtnCellIdentity id, Nb1NtnSignalStrengthInfo ssInfo)
   : id_(id)
   , ssInfo_(ssInfo) {
    registered_ = registered;
    type_       = CellType::NB1_NTN;
}

Nb1NtnCellIdentity Nb1NtnCellInfo::getCellIdentity() {
    return id_;
}

Nb1NtnSignalStrengthInfo Nb1NtnCellInfo::getSignalStrengthInfo() {
    return ssInfo_;
}

Nb1NtnCellIdentity::Nb1NtnCellIdentity(
    std::string mcc, std::string mnc, int ci, int tac, int earfcn)
   : mcc_(mcc)
   , mnc_(mnc)
   , ci_(ci)
   , tac_(tac)
   , earfcn_(earfcn) {
}

// NB1 NTN cell info
const std::string Nb1NtnCellIdentity::getMobileCountryCode() {
    return mcc_;
}

const std::string Nb1NtnCellIdentity::getMobileNetworkCode() {
    return mnc_;
}

const int Nb1NtnCellIdentity::getIdentity() {
    return ci_;
}

const int Nb1NtnCellIdentity::getTrackingAreaCode() {
    return tac_;
}

const int Nb1NtnCellIdentity::getEarfcn() {
    return earfcn_;
}

}  // end of namespace tel

}  // end namespace telux
