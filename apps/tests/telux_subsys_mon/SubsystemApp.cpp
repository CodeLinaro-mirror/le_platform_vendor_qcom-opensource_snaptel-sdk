/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <future>
#include <iostream>

#include <telux/common/Version.hpp>

#include "common/utils/Utils.hpp"

#include "SubsystemApp.hpp"

SubsystemApp::SubsystemApp(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
}

SubsystemApp::~SubsystemApp() {
}

/*
 *  Listener to receive state change updates.
 */
void StateChangeListener::onStateChange(telux::common::SubsystemInfo subsystemInfo,
    telux::common::OperationalStatus newOperationalStatus) {

    std::cout << "\nLocation   : " << static_cast<int>(subsystemInfo.location) << std::endl;
    std::cout << "Subsystem  : " << static_cast<int>(subsystemInfo.subsystems) << std::endl;
    std::cout << "New status : " << static_cast<int>(newOperationalStatus) << std::endl;
}

std::string SubsystemApp::edlStateToString(telux::platform::EdlState edlState) {

    switch (edlState) {
        case telux::platform::EdlState::IDLE:
            return "IDLE";
        case telux::platform::EdlState::MOVING_TO_EDL:
            return "MOVING TO EDL";
        case telux::platform::EdlState::MOVED_TO_EDL:
            return "MOVED TO EDL";
        case telux::platform::EdlState::FLASHING:
            return "FLASHING";
        case telux::platform::EdlState::FLASHED:
            return "FLASHED";
        default:
            return "UNKNOWN";
    }
}

void EdlListener::onEdlConfigUpdate(const telux::platform::EdlConfigs &edlConfigs) {

    std::cout << "\n[NOTIFICATION] EDL configuration updated:" << std::endl;
    std::cout << "  Image path        : " << edlConfigs.imagePath << std::endl;
    std::cout << "  Rawprogram XML    : " << edlConfigs.imageRawProgramXml << std::endl;
    std::cout << "  Patch XML         : " << edlConfigs.imagePatchXml << std::endl;
}

void EdlListener::onEdlStateChanged(telux::platform::EdlState edlState) {

    std::cout << "\n[NOTIFICATION] EDL state changed: " << SubsystemApp::edlStateToString(edlState)
              << std::endl;
}

void EdlListener::onEdlOperationResult(bool success) {

    std::cout << "\n[NOTIFICATION] EDL operation result: " << (success ? "SUCCESS" : "FAILED")
              << std::endl;
}

/*
 * Set EDL configuration parameters.
 */
void SubsystemApp::setEdlConfigurations() {

    telux::platform::EdlConfigs configs;

    configs.imagePath          = userUtils_.getStringFromUser("Enter image path: ");
    configs.imageRawProgramXml = userUtils_.getStringFromUser("Enter rawprogram XML filename: ");
    configs.imagePatchXml      = userUtils_.getStringFromUser("Enter patch XML filename: ");

    telux::common::ErrorCode ec = subsystemMgr_->setEdlConfigurations(configs);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "Failed to set EDL configurations, err " << static_cast<int>(ec) << std::endl;
        return;
    }

    std::cout << "EDL configurations set successfully" << std::endl;
}

/*
 * Get and display the current EDL configuration parameters.
 */
void SubsystemApp::getEdlConfigurations() {

    telux::platform::EdlConfigs configs;

    telux::common::ErrorCode ec = subsystemMgr_->getEdlConfigurations(configs);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "Failed to get EDL configurations, err " << static_cast<int>(ec) << std::endl;
        return;
    }

    std::cout << "Current EDL configurations:" << std::endl;
    std::cout << "  Image path        : " << configs.imagePath << std::endl;
    std::cout << "  Rawprogram XML    : " << configs.imageRawProgramXml << std::endl;
    std::cout << "  Patch XML         : " << configs.imagePatchXml << std::endl;
}

/*
 * Trigger EDL mode. Progress will be reported via onEdlStateChanged and
 * onEdlOperationResult notifications.
 */
void SubsystemApp::triggerEdl() {

    telux::common::ErrorCode ec = subsystemMgr_->triggerEdl();
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "Failed to trigger EDL, err " << static_cast<int>(ec) << std::endl;
        return;
    }

    std::cout << "EDL triggered successfully, progress will be reported via notifications"
              << std::endl;
}

/*
 * Get and display the current EDL operation phase.
 */
void SubsystemApp::getEdlState() {

    telux::platform::EdlState edlState;

    telux::common::ErrorCode ec = subsystemMgr_->getEdlState(edlState);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "Failed to get EDL state, err " << static_cast<int>(ec) << std::endl;
        return;
    }

    std::cout << "Current EDL state: " << edlStateToString(edlState) << std::endl;
}

/*
 *  Register a listener to start monitoring subsystem state changes.
 */
void SubsystemApp::registerListener() {

    telux::common::ErrorCode ec;
    std::vector<telux::common::SubsystemInfo> listOfSubsystems;

    if (stateChangeListener_) {
        std::cout << "Listener exist" << std::endl;
        return;
    }

    try {
        stateChangeListener_ = std::make_shared<StateChangeListener>();
    } catch (const std::exception &e) {
        std::cout << "Can't allocate StateChangeListener" << std::endl;
        stateChangeListener_ = nullptr;
        return;
    }

    getSubsystemsToMonitor(listOfSubsystems);

    if (listOfSubsystems.empty()) {
        std::cout << "Not monitoring as no subsystem specified" << std::endl;
        stateChangeListener_ = nullptr;
        return;
    }

    ec = subsystemMgr_->registerListener(stateChangeListener_, listOfSubsystems);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "Can't register listener, err " << static_cast<int>(ec) << std::endl;
        stateChangeListener_ = nullptr;
        return;
    }

    std::cout << "Listener registered" << std::endl;
}

/*
 * Ask user to specify what subsystems to monitor and populate listOfSubsystems accordingly.
 */
void SubsystemApp::getSubsystemsToMonitor(
    std::vector<telux::common::SubsystemInfo> &listOfSubsystems) {

    bool yes, local;
    telux::common::SubsystemInfo subsysInfo{};

    yes = userUtils_.getYesNoFromUser("Monitor MPSS");
    if (yes) {
        subsysInfo.subsystems = telux::common::Subsystem::MPSS;
        local                 = userUtils_.getLocalRemoteFromUser();
        if (local) {
            subsysInfo.location = telux::common::ProcType::LOCAL_PROC;
        } else {
            subsysInfo.location = telux::common::ProcType::REMOTE_PROC;
        }
        listOfSubsystems.push_back(subsysInfo);
    }

    yes = userUtils_.getYesNoFromUser("Monitor APSS");
    if (yes) {
        subsysInfo.subsystems = telux::common::Subsystem::APSS;
        local                 = userUtils_.getLocalRemoteFromUser();
        if (local) {
            subsysInfo.location = telux::common::ProcType::LOCAL_PROC;
        } else {
            subsysInfo.location = telux::common::ProcType::REMOTE_PROC;
        }
        listOfSubsystems.push_back(subsysInfo);
    }
}

/*
 *  Deregister listener to stop monitoring subsystems.
 */
void SubsystemApp::deRegisterListener() {

    telux::common::ErrorCode ec;

    if (!stateChangeListener_) {
        std::cout << "Listener doesn't exist" << std::endl;
        return;
    }

    ec = subsystemMgr_->deRegisterListener(stateChangeListener_);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "Can't deregister listener, err " << static_cast<int>(ec) << std::endl;
        return;
    }

    stateChangeListener_ = nullptr;
    std::cout << "Listener deregistered" << std::endl;
}

void SubsystemApp::triggerMpssRestart() {
    if (subsystemMgr_) {
        telux::common::Status status = subsystemMgr_->triggerMpssRestart(
            [](telux::common::ErrorCode error) {
                if (error == telux::common::ErrorCode::SUCCESS) {
                    std::cout << "Modem DSP Restart succeeded" << std::endl;
                } else {
                    std::cout << "Modem DSP Restart failed with error: " << static_cast<int>(error)
                              << std::endl;
                }
            });
        if (status == telux::common::Status::SUCCESS) {
            std::cout << "Triggered Modem DSP restart successfully" << std::endl;
        } else {
            std::cout << "Failed to trigger Modem DSP restart" << std::endl;
        }
    } else {
        std::cout << "Subsystem Mgr not present" << std::endl;
    }
}

/*
 *  Prepare the menu and display it on the console.
 */
void SubsystemApp::init() {

    telux::common::ServiceStatus serviceStatus;
    std::promise<telux::common::ServiceStatus> p{};

    auto &subsystemFact = telux::platform::SubsystemFactory::getInstance();

    subsystemMgr_ = subsystemFact.getSubsystemManager(
        [&p](telux::common::ServiceStatus srvStatus) { p.set_value(srvStatus); });

    if (!subsystemMgr_) {
        std::cout << "Can't get ISubsystemManager, waiting..." << std::endl;
        return;
    }

    serviceStatus = p.get_future().get();
    if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << "Subsystem manager unavailable" << std::endl;
        return;
    }

    /* Auto-register EDL listener at startup so notifications arrive without
     * requiring the user to press "1". APSS+REMOTE_PROC is the only valid
     * combination for EDL on this platform. */
    edlListener_ = std::make_shared<EdlListener>();
    {
        telux::common::SubsystemInfo edlSubsysInfo{};
        edlSubsysInfo.subsystems = telux::common::Subsystem::APSS;
        edlSubsysInfo.location   = telux::common::ProcType::REMOTE_PROC;

        telux::common::ErrorCode ec
            = subsystemMgr_->registerListener(edlListener_, {edlSubsysInfo});
        if (ec != telux::common::ErrorCode::SUCCESS) {
            std::cout << "Warning: failed to register EDL listener, err " << static_cast<int>(ec)
                      << std::endl;
            edlListener_ = nullptr;
        }
    }

    std::shared_ptr<ConsoleAppCommand> regListener
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("1", "Start monitoring subsystems",
            {}, std::bind(&SubsystemApp::registerListener, this)));

    std::shared_ptr<ConsoleAppCommand> deregListener
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("2", "Stop monitoring subsystems",
            {}, std::bind(&SubsystemApp::deRegisterListener, this)));

    std::shared_ptr<ConsoleAppCommand> triggerMpssRestartCmd
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
            "3", "Trigger Mpss restart", {}, std::bind(&SubsystemApp::triggerMpssRestart, this)));

    std::shared_ptr<ConsoleAppCommand> setEdlCfg
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("4", "Set EDL configurations", {},
            std::bind(&SubsystemApp::setEdlConfigurations, this)));

    std::shared_ptr<ConsoleAppCommand> getEdlCfg
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("5", "Get EDL configurations", {},
            std::bind(&SubsystemApp::getEdlConfigurations, this)));

    std::shared_ptr<ConsoleAppCommand> trigEdl = std::make_shared<ConsoleAppCommand>(
        ConsoleAppCommand("6", "Trigger EDL", {}, std::bind(&SubsystemApp::triggerEdl, this)));

    std::shared_ptr<ConsoleAppCommand> getEdlSt = std::make_shared<ConsoleAppCommand>(
        ConsoleAppCommand("7", "Get EDL state", {}, std::bind(&SubsystemApp::getEdlState, this)));

    std::vector<std::shared_ptr<ConsoleAppCommand>> mainCmds = {
        regListener, deregListener, triggerMpssRestartCmd, setEdlCfg, getEdlCfg, trigEdl, getEdlSt};

    ConsoleApp::addCommands(mainCmds);
    ConsoleApp::displayMenu();
}

int main(int argc, char **argv) {

    auto sdkVersion = telux::common::Version::getSdkVersion();

    std::string sdkReleaseName = telux::common::Version::getReleaseName();

    std::string appName = "Subsystem monitor console app - SDK v" + std::to_string(sdkVersion.major)
                          + "." + std::to_string(sdkVersion.minor) + "."
                          + std::to_string(sdkVersion.patch) + "\n"
                          + "Release name: " + sdkReleaseName;

    auto sysApp = std::make_shared<SubsystemApp>(appName, "subsys> ");

    std::vector<std::string> supplementaryGrps{"system", "diag", "logd", "dlt"};

    int rc = Utils::setSupplementaryGroups(supplementaryGrps);
    if (rc < 0) {
        std::cout << "Adding supplementary groups failed!" << std::endl;
    }

    sysApp->init();

    return sysApp->mainLoop();
}
