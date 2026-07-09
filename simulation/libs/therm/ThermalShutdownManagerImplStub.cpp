/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "common/Logger.hpp"
#include "ThermalShutdownManagerImplStub.hpp"

namespace telux {
namespace therm {

using namespace telux::common;

ThermalShutdownManagerImplStub::ThermalShutdownManagerImplStub() {
}

ThermalShutdownManagerImplStub::~ThermalShutdownManagerImplStub() {
}

telux::common::ServiceStatus ThermalShutdownManagerImplStub::getServiceStatus() {
    LOG(DEBUG, __FUNCTION__);
    return telux::common::ServiceStatus::SERVICE_AVAILABLE;
}

Status ThermalShutdownManagerImplStub::registerListener(
    std::weak_ptr<IThermalShutdownListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    (void)listener;
    return telux::common::Status::NOTSUPPORTED;
}

Status ThermalShutdownManagerImplStub::deregisterListener(
    std::weak_ptr<IThermalShutdownListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    (void)listener;
    return telux::common::Status::NOTSUPPORTED;
}

Status ThermalShutdownManagerImplStub::setAutoShutdownMode(
    AutoShutdownMode mode, telux::common::ResponseCallback callback, uint32_t timeout) {
    LOG(INFO, " Mode: ", static_cast<int>(mode));
    (void)callback;
    (void)timeout;
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status ThermalShutdownManagerImplStub::getAutoShutdownMode(
    GetAutoShutdownModeResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);

    return telux::common::Status::NOTSUPPORTED;
}

}  // end of namespace therm
}  // end of namespace telux
