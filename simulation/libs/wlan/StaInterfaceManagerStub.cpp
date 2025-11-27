/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "StaInterfaceManagerStub.hpp"
#include "common/CommonUtils.hpp"
#include "common/Logger.hpp"
#include <thread>

namespace telux {
namespace wlan {

StaInterfaceManagerStub::StaInterfaceManagerStub() {
    LOG(DEBUG, __FUNCTION__);
}

StaInterfaceManagerStub::~StaInterfaceManagerStub() {
    LOG(DEBUG, __FUNCTION__);
}

telux::common::Status StaInterfaceManagerStub::init() {
    LOG(DEBUG, __FUNCTION__);
    return telux::common::Status::SUCCESS;
}

telux::common::ErrorCode StaInterfaceManagerStub::setIpConfig(
    Id staId, StaIpConfig ipConfig, StaStaticIpConfig staticIpConfig) {
    LOG(DEBUG, __FUNCTION__);
    return telux::common::ErrorCode::REQUEST_NOT_SUPPORTED;
}

telux::common::ErrorCode StaInterfaceManagerStub::setBridgeMode(
    Id staId, StaBridgeMode bridgeMode) {
    LOG(DEBUG, __FUNCTION__);
    return telux::common::ErrorCode::REQUEST_NOT_SUPPORTED;
}

telux::common::ErrorCode StaInterfaceManagerStub::enableHotspot2(Id staId, bool enable) {
    LOG(DEBUG, __FUNCTION__);
    return telux::common::ErrorCode::REQUEST_NOT_SUPPORTED;
}

telux::common::ErrorCode StaInterfaceManagerStub::getConfig(std::vector<StaConfig> &staConfigs) {
    LOG(DEBUG, __FUNCTION__);
    return telux::common::ErrorCode::REQUEST_NOT_SUPPORTED;
}

telux::common::ErrorCode StaInterfaceManagerStub::getStatus(std::vector<StaStatus> &staStatuses) {
    LOG(DEBUG, __FUNCTION__);
    return telux::common::ErrorCode::REQUEST_NOT_SUPPORTED;
}

telux::common::ErrorCode StaInterfaceManagerStub::startScan(Id staId) {
    LOG(DEBUG, __FUNCTION__);
    return telux::common::ErrorCode::REQUEST_NOT_SUPPORTED;
}

telux::common::ErrorCode StaInterfaceManagerStub::addNetworkConfig(
    Id staId, const StaNetworkConfigEntry &network) {
    LOG(DEBUG, __FUNCTION__);
    return telux::common::ErrorCode::REQUEST_NOT_SUPPORTED;
}

telux::common::ErrorCode StaInterfaceManagerStub::removeNetworkConfig(
    Id staId, NetworkId networkId) {
    LOG(DEBUG, __FUNCTION__);
    return telux::common::ErrorCode::REQUEST_NOT_SUPPORTED;
}

telux::common::ErrorCode StaInterfaceManagerStub::getNetworkConfigs(
    Id staId, std::vector<StaNetworkConfigInfo> &networkConfigs) {
    LOG(DEBUG, __FUNCTION__);
    return telux::common::ErrorCode::REQUEST_NOT_SUPPORTED;
}

telux::common::ErrorCode StaInterfaceManagerStub::connect(Id staId, NetworkId networkId) {
    LOG(DEBUG, __FUNCTION__);
    return telux::common::ErrorCode::REQUEST_NOT_SUPPORTED;
}

telux::common::ErrorCode StaInterfaceManagerStub::disconnect(Id staId) {
    LOG(DEBUG, __FUNCTION__);
    return telux::common::ErrorCode::REQUEST_NOT_SUPPORTED;
}

telux::common::ErrorCode StaInterfaceManagerStub::manageStaService(Id staId, ServiceOperation opr) {
    LOG(DEBUG, __FUNCTION__);
    return telux::common::ErrorCode::REQUEST_NOT_SUPPORTED;
}

void StaInterfaceManagerStub::onStationStatusChanged(std::vector<StaStatus> staStatuses) {
    LOG(DEBUG, __FUNCTION__);
    (void)staStatuses;
}

void StaInterfaceManagerStub::onScanResultUpdated(const StaScanResult &staScanResult) {
    LOG(DEBUG, __FUNCTION__);
    (void)staScanResult;
}

void StaInterfaceManagerStub::onStationBandChanged(BandType band) {
    LOG(DEBUG, __FUNCTION__);
    (void)band;
}

telux::common::ErrorCode StaInterfaceManagerStub::registerListener(
    std::weak_ptr<IStaListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    return telux::common::ErrorCode::REQUEST_NOT_SUPPORTED;
}

telux::common::ErrorCode StaInterfaceManagerStub::deregisterListener(
    std::weak_ptr<IStaListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    return telux::common::ErrorCode::REQUEST_NOT_SUPPORTED;
}

}  // namespace wlan
}  // namespace telux