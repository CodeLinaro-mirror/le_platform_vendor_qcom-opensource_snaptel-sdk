/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <cstdint>
extern "C" {
#include "unistd.h"
}

#include <algorithm>
#include <iostream>

#include <telux/data/DataFactory.hpp>
#include "../../../../common/utils/Utils.hpp"

#include "IpsecMenu.hpp"
#include "../DataUtils.hpp"

IpsecMenu::IpsecMenu(std::string appName, std::string cursor)
    : ConsoleApp(appName, cursor) {
    ipsecManager_ = nullptr;
    menuOptionsAdded_ = false;
    subSystemStatusUpdated_ = false;
}

IpsecMenu::~IpsecMenu() {
}

bool IpsecMenu::init() {
    telux::common::ServiceStatus subSystemStatus = telux::common::ServiceStatus::SERVICE_FAILED;
    subSystemStatusUpdated_ = false;
    if (ipsecManager_ == nullptr) {
        auto initCb = std::bind(&IpsecMenu::onInitComplete, this, std::placeholders::_1);
        auto &dataFactory = telux::data::DataFactory::getInstance();
        //Try both local and remote operation type. If operation type is not supported,
        // nullptr is returned. ipsecManager_ pointer will be associated with valid return pointer
        auto localIpsecMgr = dataFactory.getIpsecManager(
            telux::data::OperationType::DATA_LOCAL, initCb);
        if (localIpsecMgr) {
            ipsecManager_ = localIpsecMgr;
        }
        auto remoteIpsecMgr = dataFactory.getIpsecManager(
            telux::data::OperationType::DATA_REMOTE, initCb);
        if (remoteIpsecMgr) {
            ipsecManager_ = remoteIpsecMgr;
        }
        if(ipsecManager_ == nullptr ) {
            //Return immediately
            std::cout << "\nError encountered in initializing IPsec Manager" << std::endl;
            return false;
        }
        ipsecManager_->registerListener(shared_from_this());
    }
    {
        std::unique_lock<std::mutex> lck(mtx_);
        //IPsec Manager is guaranteed to be valid pointer at this point. If manager initialization
        //fails and factory invalidated it's own pointer to snat manager before reaching this point,
        //reference count of IPsec manager should still be 1
        telux::common::ServiceStatus subSystemStatus = ipsecManager_->getServiceStatus();
        if (subSystemStatus == telux::common::ServiceStatus::SERVICE_UNAVAILABLE) {
            std::cout << "\nInitializing IPsec Manager, Please wait ..." << std::endl;
            cv_.wait(lck, [this]{return this->subSystemStatusUpdated_;});
            subSystemStatus = ipsecManager_->getServiceStatus();
        }
        //At this point, initialization should be either AVAILABLE or FAIL
        if (subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "\nIPsec Manager is ready" << std::endl;
        }
        else {
            std::cout << "\nIPsec Manager initialization failed" << std::endl;
            ipsecManager_ = nullptr;
            return false;
        }
    }

    if (menuOptionsAdded_ == false) {
        menuOptionsAdded_ = true;
        std::shared_ptr<ConsoleAppCommand> enableIpsec
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("1", "Enable_IPsec_Feature", {},
            std::bind(&IpsecMenu::enableIpsec, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> disableIpsec
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("2", "Disable_IPsec", {},
            std::bind(&IpsecMenu::disableIpsec, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> getIpsecEnabled
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("3", "Get_IPsec_Feature", {},
            std::bind(&IpsecMenu::getIpsecEnabled, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> setTunnelConfig
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("4", "Set_IPsec_Tunnel", {},
            std::bind(&IpsecMenu::setTunnelConfig, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> setTunnelState
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("5", "Set_IPsec_Tunnel_State", {},
            std::bind(&IpsecMenu::setTunnelState, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> deleteTunnel
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("6", "Delete_IPsec_Tunnel", {},
            std::bind(&IpsecMenu::deleteTunnel, this, std::placeholders::_1)));

        std::shared_ptr<ConsoleAppCommand> getTunnelConfig
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("7", "Get_IPsec_Tunnel_Config", {},
            std::bind(&IpsecMenu::getTunnelConfig, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> getTunnelState
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("8", "Get_IPsec_Tunnel_State", {},
            std::bind(&IpsecMenu::getTunnelState, this, std::placeholders::_1)));

        std::vector<std::shared_ptr<ConsoleAppCommand>> commandsList = {
            enableIpsec, disableIpsec,
            getIpsecEnabled, setTunnelConfig,
            setTunnelState, deleteTunnel,
            getTunnelConfig, getTunnelState};
        addCommands(commandsList);
    }
    ConsoleApp::displayMenu();
    return true;
}

void IpsecMenu::onInitComplete(telux::common::ServiceStatus status) {
    std::lock_guard<std::mutex> lock(mtx_);
    subSystemStatusUpdated_ = true;
    cv_.notify_all();
}

void IpsecMenu::enableIpsec(std::vector<std::string> inputCommand) {
    telux::common::ErrorCode error;

    error = ipsecManager_->enableIpsec();
    std::cout << std::endl << std::endl;
    std::cout << "Enable IPsec Feature Response"
              << (telux::common::ErrorCode::SUCCESS == error ? " is successful" : " failed")
              << ". ErrorCode: " << static_cast<int>(error)
              << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
}
void IpsecMenu::disableIpsec(std::vector<std::string> inputCommand) {
    telux::common::ErrorCode error;

    error = ipsecManager_->disableIpsec();
    std::cout << std::endl << std::endl;
    std::cout << "Disable IPsec Feature Response"
              << (telux::common::ErrorCode::SUCCESS == error ? " is successful" : " failed")
              << ". ErrorCode: " << static_cast<int>(error)
              << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
}
void IpsecMenu::getIpsecEnabled(std::vector<std::string> inputCommand) {
    telux::common::ErrorCode error;

    bool enable = false;
    error = ipsecManager_->getIpsecEnabled(enable);
    std::cout << std::endl << std::endl;
    std::cout << "Disable IPsec Feature Response"
              << (telux::common::ErrorCode::SUCCESS == error ? " is successful" : " failed")
              << ". ErrorCode: " << static_cast<int>(error)
              << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;

    if (telux::common::ErrorCode::SUCCESS != error) {
        return;
    }

    std::cout << "IPsec Feature is:"
              << (enable ? " enabled" : " disabled")
              << std::endl;
}

void IpsecMenu::setTunnelConfig(std::vector<std::string> inputCommand) {
    telux::common::ErrorCode error;
    struct IpsecConfig ipsecConfig;
    std::cout << "List of IPsec topology supported:\n"
              << "1. IPsec Host-to-Host\n"
              << "2. IPsec Site-to-Site without NAT\n"
              << "3. IPsec Host-to-Host and IPsec Site-to-Site without NAT\n";

    int topology = 0;
    std::cout << "Please enter the choice: ";
    std::cin >> topology;
    Utils::validateInput(topology, {1, 2, 3});

    switch(topology) {
        /* IPsec Host-to-Host */
        case 1: {
            ipsecConfig.topology = IpsecTopology::HOST_TO_HOST;
            setIKEConfig(ipsecConfig);
            setHostToHostConfig(ipsecConfig);
            break;
        }
        /* IPsec Site_to_Site without NAT */
        case 2: {
            ipsecConfig.topology = IpsecTopology::SITE_TO_SITE_WITHOUT_NAT;
            setIKEConfig(ipsecConfig);
            setSiteToSiteWithoutNATConfig(ipsecConfig);
            break;
        }
        /* IPsec Host-to-Host and IPsec Site_to_Site without NAT */
        case 3: {
            ipsecConfig.topology = IpsecTopology::HOST_TO_HOST_AND_SITE_TO_SITE_WITHOUT_NAT;
            setIKEConfig(ipsecConfig);
            setSiteToSiteWithoutNATConfig(ipsecConfig);
            setHostToHostConfig(ipsecConfig);
            break;
        }
        default: {
            ipsecConfig.topology = IpsecTopology::INVALID;
            break;
        }
    }

    error = ipsecManager_->setTunnelConfig(ipsecConfig);
    std::cout << std::endl << std::endl;
    std::cout << "set IPsec Tunnel Config Response"
              << (telux::common::ErrorCode::SUCCESS == error ? " is successful" : " failed")
              << ". ErrorCode: " << static_cast<int>(error)
              << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
}

void IpsecMenu::setTunnelState(std::vector<std::string> inputCommand) {
    telux::common::ErrorCode error;
    struct IpsecIdentifier identifier;
    IpsecSetTunnelState state;

    /* IPsec Tunnel State */
    int stateInput;
    std::cout << "Please input IPsec Tunnel State:(0.Activate 1.Deactivate) ";
    std::cin >> stateInput;
    Utils::validateInput(stateInput, {0, 1});
    state = static_cast<IpsecSetTunnelState>(stateInput);

    std::string stateString;
    switch(state) {
        case IpsecSetTunnelState::SET_TUNNEL_STATE_ACTIVATE: {
            stateString = "activate";
            break;
        }
        case IpsecSetTunnelState::SET_TUNNEL_STATE_DEACTIVATE: {
            stateString = "deactivate";
            break;
        }
        default: {
            std::cout << "Invalid State Input\n";
            break;
        }
    }

    /* IPsec Tunnel IKE Identifier */
    std::string ikeIdentifier;
    std::cout << "Please input IKE Identifier:\n";
    std::getline(std::cin, ikeIdentifier, delimiter);
    identifier.ikeIdentifier = ikeIdentifier;

    /* IPsec Tunnel Child Identifier Choice */
    int childChoice;
    std::cout << "Do you want to "
              << stateString
              << " specific child? (1.YES, 2.NO)\n";
    std::cin >> childChoice;
    Utils::validateInput(childChoice, {1, 2});
    if (1 == childChoice) {
        std::string childIdentifier;
        std::cout << "Please input Child Identifier:\n";
        std::getline(std::cin, childIdentifier, delimiter);
        identifier.childIdentifier = childIdentifier;
    }
    else if (2 == childChoice) {
        identifier.childIdentifier.clear();
    }

    error = ipsecManager_->setTunnelState(identifier, state);
    std::cout << std::endl << std::endl;
    std::cout << "set IPsec Tunnel State Response"
              << (telux::common::ErrorCode::SUCCESS == error ? " is successful" : " failed")
              << ". ErrorCode: " << static_cast<int>(error)
              << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
}

void IpsecMenu::deleteTunnel(std::vector<std::string> inputCommand) {
    telux::common::ErrorCode error;
    struct IpsecIdentifier identifier;

    /* IPsec Tunnel IKE Identifier */
    std::string ikeIdentifier;
    std::cout << "Please input IKE Identifier:\n";
    std::getline(std::cin, ikeIdentifier, delimiter);
    identifier.ikeIdentifier = ikeIdentifier;

    /* IPsec Tunnel Child Identifier Choice */
    int childChoice;
    std::cout << "Do you want to delete specific child? (1.YES, 2.NO)\n";
    std::cin >> childChoice;
    Utils::validateInput(childChoice, {1, 2});
    if (1 == childChoice) {
        std::string childIdentifier;
        std::cout << "Please input Child Identifier:\n";
        std::getline(std::cin, childIdentifier, delimiter);
        identifier.childIdentifier = childIdentifier;
    }
    else if (2 == childChoice) {
        identifier.childIdentifier.clear();
    }

    error = ipsecManager_->deleteTunnel(identifier);
    std::cout << std::endl << std::endl;
    std::cout << "Delete IPsec Tunnel Response"
              << (telux::common::ErrorCode::SUCCESS == error ? " is successful" : " failed")
              << ". ErrorCode: " << static_cast<int>(error)
              << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
}

void IpsecMenu::getTunnelConfig(std::vector<std::string> inputCommand) {
    telux::common::ErrorCode error;
    struct IpsecIdentifier identifier;
    IpsecConfig ipsecConfig;

    /* IPsec Tunnel IKE Identifier */
    std::string ikeIdentifier;
    std::cout << "Please input IKE Identifier:\n";
    std::getline(std::cin, ikeIdentifier, delimiter);
    identifier.ikeIdentifier = ikeIdentifier;

    /* IPsec Tunnel Child Identifier Choice */
    int childChoice;
    std::cout << "Do you want to get specific child? (1.YES, 2.NO)\n";
    std::cin >> childChoice;
    Utils::validateInput(childChoice, {1, 2});
    if (1 == childChoice) {
        std::string childIdentifier;
        std::cout << "Please input Child Identifier:\n";
        std::getline(std::cin, childIdentifier, delimiter);
        identifier.childIdentifier = childIdentifier;
    }
    else if (2 == childChoice) {
        identifier.childIdentifier.clear();
    }

    error = ipsecManager_->getTunnelConfig(identifier, ipsecConfig);
    std::cout << std::endl << std::endl;
    std::cout << "Get IPsec Tunnel Config Response"
              << (telux::common::ErrorCode::SUCCESS == error ? " is successful" : " failed")
              << ". ErrorCode: " << static_cast<int>(error)
              << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;

    if (telux::common::ErrorCode::SUCCESS != error) {
        return;
    }

    /* Show IPsec Config */
    std::cout << "\n IPsec Configuration:\n";
    std::cout << "\nProfile id: "
              << ipsecConfig.profileId
              << std::endl;

    /* Show IPsec Topology */
    std::cout << "Topology: ";
    switch (ipsecConfig.topology) {
        case IpsecTopology::HOST_TO_HOST: {
            std::cout << "Host-To-Host\n";
            break;
        }
        case IpsecTopology::SITE_TO_SITE_WITHOUT_NAT: {
            std::cout << "Site-To-Site Without NAT\n";
            break;
        }
        case IpsecTopology::HOST_TO_HOST_AND_SITE_TO_SITE_WITHOUT_NAT: {
            std::cout << "Host-To-Host and Site-To-Site Without NAT\n";
            break;
        }
        default: {
            std::cout << "Invalid\n";
            break;
        }
    }

    /* Show IPsec IKE Configuration */
    std::cout << "\n--------IKE--------\n";
    std::cout << "IKE identifier: "
              << ipsecConfig.ikeConfig.ikeIdentifier
              << std::endl;

    /* Show IKE Backhaul Type */
    std::cout << "Backhaul Type: ";
    switch (ipsecConfig.ikeConfig.bhType) {
        case IpsecBackhaulType::WWAN: {
            std::cout << "WWAN CELLULAR Backhaul\n";
            break;
        }
        case IpsecBackhaulType::ETH: {
            std::cout << "Ethernet Backhaul\n";
            break;
        }
        default: {
            std::cout << "Invalid Backhaul\n";
            break;
        }
    }

    /* Show IKE Tunnel Type */
    std::cout << "Tunnel Type: ";
    switch (ipsecConfig.ikeConfig.tunnelType) {
        case IpsecTunnelType::V4_ESP_TUNNEL_MODE: {
            std::cout << "IPv4 ESP Tunnel Mode\n";
            break;
        }
        case IpsecTunnelType::V6_ESP_TUNNEL_MODE: {
            std::cout << "IPv6 ESP Tunnel Mode\n";
            break;
        }
        default: {
            std::cout << "Invalid\n";
            break;
        }
    }

    /* Show IKE Remote Endpoint Address */
    std::cout << "Remote EP address: "
              << ipsecConfig.ikeConfig.remoteEpAddr
              << std::endl;

    /* Show IKE Authentication Type */
    std::cout << "Authentication Type: "
              << (IpsecAuthenticationType::PSK == ipsecConfig.ikeConfig.authType ?
                  "PSK" : "X509")
              << std::endl;

    /* Show IKE X509 Identifier if necessary */
    if (IpsecAuthenticationType::X509 == ipsecConfig.ikeConfig.authType) {
        /* X509 Local Identifier */
        std::cout << "X509 Local Identifier: "
                  << ipsecConfig.ikeConfig.localIdentifier
                  << std::endl;

        /* X509 Remote Identifier */
        std::cout << "X509 Remote Identifier: "
                  << ipsecConfig.ikeConfig.remoteIdentifier
                  << std::endl;
    }

    /* Show IKE Rekey Time */
    std::cout << "IKE rekey time(mins): "
              << ipsecConfig.ikeConfig.rekeyIntervalMins
              << std::endl;

    /* Show Child SAs number */
    std::cout << "Child entries: "
              << ipsecConfig.childEntries
              << std::endl;

    /* Show Child SAs Configuration */
    auto printChild = [](const ChildConfig &child) {
        /* Show Child SA Identifier */
        std::cout << "Child Identifier: "
                  << child.childIdentifier
                  << std::endl;

        /* Show SA Hardware Offload Option */
        std::cout << "Child HW Offload option: "
                  << (child.hwOffload ? "True" : "False")
                  << std::endl;

        /* Show SA Start Action Option */
        std::cout << "Child Trap Action option: "
                  << (child.trapAction ? "Trap" : "Start")
                  << std::endl;

        /* Show SA Rekey Time in minutes */
        std::cout << "Child rekey time(mins): "
                  << child.rekeyIntervalMins
                  << std::endl;

        /* Show traffic selector local address */
        if (!child.localAddr.empty()) {
            std::cout << "Child local subnet: "
                      << child.localAddr
                      << std::endl;
        }

        /* Show traffic selector remote address */
        if (!child.remoteAddr.empty()) {
            std::cout << "Child remote subnet: "
                      << child.remoteAddr
                      << std::endl;
        }
    };


    for (int i = 0; i < ipsecConfig.childCfg.size(); ++i) {
        auto child = ipsecConfig.childCfg[i];

        /* Show Specific child if necessary */
        if (!identifier.childIdentifier.empty()) {
            if (identifier.childIdentifier == child.childIdentifier) {
                std::cout << "\n--------Child "
                        << child.childIdentifier
                        << " ----------\n";
                printChild(child);
                break;
            }
            continue;
        }
        std::cout << "\n--------Child "
                << i + 1
                << " ----------\n";
        printChild(child);
    }
}

void IpsecMenu::getTunnelState(std::vector<std::string> inputCommand) {
    telux::common::ErrorCode error;
    IpsecIdentifier identifier;
    IpsecTunnelStateMap ipsecTunnelState;

    /* IPsec Tunnel IKE Identifier */
    std::string ikeIdentifier;
    std::cout << "Please input IKE Identifier:\n";
    std::getline(std::cin, ikeIdentifier, delimiter);
    identifier.ikeIdentifier = ikeIdentifier;

    /* IPsec Tunnel Child Identifier Choice */
    int childChoice;
    std::cout << "Do you want to get specific child? (1.YES, 2.NO)\n";
    std::cin >> childChoice;
    Utils::validateInput(childChoice, {1, 2});
    if (1 == childChoice) {
        std::string childIdentifier;
        std::cout << "Please input Child Identifier:\n";
        std::getline(std::cin, childIdentifier, delimiter);
        identifier.childIdentifier = childIdentifier;
    }
    else if (2 == childChoice) {
        identifier.childIdentifier.clear();
    }

    error = ipsecManager_->getTunnelState(identifier, ipsecTunnelState);
    std::cout << std::endl << std::endl;
    std::cout << "Get IPsec Tunnel State Response"
              << (telux::common::ErrorCode::SUCCESS == error ? " is successful" : " failed")
              << ". ErrorCode: " << static_cast<int>(error)
              << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;

    if (telux::common::ErrorCode::SUCCESS != error) {
        return;
    }

    /* Show IPsec Tunnel State */
    std::cout << "--------------IPSEC Tunnel Status----------\n";
    auto printState = [](const std::string &childIdentifier,
                         const IpsecTunnelState &state) {
        /* Show Child SA Identifier */
        std::cout << "Child Identifier: "
                  << childIdentifier
                  << std::endl;

        /* Show Enable Option */
        std::cout << "Enabled: "
                  << (state.enableStatus ? "True" : "False")
                  << std::endl;

        /* Show Tunnel Current State */
        std::cout << "IPsec tunnel state: ";
        switch (state.state) {
            case IpsecGetTunnelState::TUNNEL_DISCONNECTED: {
                std::cout << "DISCONNECTED\n";
                break;
            }
            case IpsecGetTunnelState::TUNNEL_INPROGRESS: {
                std::cout << "CONNECTING\n";
                break;
            }
            case IpsecGetTunnelState::TUNNEL_CONNECTED: {
                std::cout << "CONNECTED\n";
                break;
            }
            default: {
                std::cout << "INVALID\n";
                break;
            }
        }
    };

    int count = 0;
    for (auto &[childIdentifier, state] : ipsecTunnelState) {
        if (!identifier.childIdentifier.empty()) {
            if (childIdentifier == identifier.childIdentifier) {
                std::cout << "\n--------Child "
                          << childIdentifier
                          << " ----------\n";
                printState(childIdentifier, state);
                break;
            }
            continue;
        }
        std::cout << "\n--------Child "
                  << ++count
                  << " ----------\n";
        printState(childIdentifier, state);
    }
}

void IpsecMenu::setIKEConfig(telux::data::net::IpsecConfig &ipsecConfig) {
    /* Profile id */
    int profileId;
    std::cout << "   Please input the profile id:";
    std::cin >> profileId;
    Utils::validateInput(profileId);
    ipsecConfig.profileId = profileId;

    /* IKE Tunnel Info */
    /* IPsec Backhaul Type */
    int bhType;
    std::cout << "   Please enter BH type choices(1. WWAN BH, 2. ETH BH): ";
    std::cin >> bhType;
    Utils::validateInput(bhType, {1, 2});
    ipsecConfig.ikeConfig.bhType = static_cast<IpsecBackhaulType>(bhType);

    /* IPsec IKE Identifier */
    std::string ikeIdentifier;
    std::cout << "   Please input the IKE_identifier of IPsec tunnel:";
    std::getline(std::cin, ikeIdentifier, delimiter);
    ipsecConfig.ikeConfig.ikeIdentifier = ikeIdentifier;

    /* IPsec IKE Tunnel Type */
    int tunnelType;
    std::cout << "   Please input the tunnel type(1 for IPv4, 2 for IPv6):";
    std::cin >> tunnelType;
    Utils::validateInput(tunnelType, {1, 2});
    ipsecConfig.ikeConfig.tunnelType = static_cast<IpsecTunnelType>(tunnelType);

    /* IPsec IKE Auth Type */
    int authType;
    std::cout << "   Please input the authentication type(1 for PSK, 2 for X509):";
    std::cin >> authType;
    Utils::validateInput(authType, {1, 2});
    ipsecConfig.ikeConfig.authType = static_cast<IpsecAuthenticationType>(authType);

    /* IPsec IKE Remote EP Addr */
    std::string remoteEpAddr;
    switch (ipsecConfig.ikeConfig.tunnelType) {
        case IpsecTunnelType::V4_ESP_TUNNEL_MODE: {
            std::cout << "   Please input Remote Gateway IPv4 address : ";
            break;
        }
        case IpsecTunnelType::V6_ESP_TUNNEL_MODE: {
            std::cout << "   Please input Remote Gateway IPv6 address : ";
            break;
        }
        default: {
            ipsecConfig.ikeConfig.tunnelType = IpsecTunnelType::INVALID;
            break;
        }
    }
    std::getline(std::cin, remoteEpAddr, delimiter);
    ipsecConfig.ikeConfig.remoteEpAddr = remoteEpAddr;

    /* IPsec IKE X509 Identifier */
    if (ipsecConfig.ikeConfig.authType == IpsecAuthenticationType::X509) {
        /* IPsec IKE X509 Local Identifier */
        std::string localIdentifier;
        std::cout << "   Please input the local identifier:";
        std::getline(std::cin, localIdentifier, delimiter);
        ipsecConfig.ikeConfig.localIdentifier = localIdentifier;
        /* IPsec IKE X509 Remote Identifier */
        std::string remoteIdentifier;
        std::cout << "   Please input the remote identifier:";
        std::getline(std::cin, remoteIdentifier, delimiter);
        ipsecConfig.ikeConfig.remoteIdentifier = remoteIdentifier;
    }

    /* IPsec IKE Rekey Time */
    uint16_t rekeyIntervalMins;
    std::cout << "   Please input rekey time(mins) for IKE tunnel:";
    std::cin >> rekeyIntervalMins;
    Utils::validateInput(rekeyIntervalMins);
    ipsecConfig.ikeConfig.rekeyIntervalMins = rekeyIntervalMins;
}

void IpsecMenu::setHostToHostConfig(telux::data::net::IpsecConfig &ipsecConfig) {
    /* Child Info for Host-to-Host */
    ipsecConfig.childEntries += 1;
    struct ChildConfig child;

    /* IPsec Child SA Identifier */
    std::string childIdentifier;
    std::cout << "\n";
    std::cout << "   Please input the CHILD_identifier of IPsec Host-to-Host tunnel:";
    std::getline(std::cin, childIdentifier, delimiter);
    child.childIdentifier = childIdentifier;

    /* IPsec Child Hardware Offload Option */
    int hwOffload;
    std::cout << "   Do you want to enable hw_offload:(1.YES 0.NO) ";
    std::cin >> hwOffload;
    Utils::validateInput(hwOffload, {0, 1});
    child.hwOffload = static_cast<bool>(hwOffload);

    /* IPsec Child Start action Option */
    int trapAction;
    std::cout << "   Do you want to set start action as trap:(1.YES 0.NO) ";
    std::cin >> trapAction;
    Utils::validateInput(trapAction, {0, 1});
    child.trapAction = static_cast<bool>(trapAction);

    /* IPsec Child Rekey Time */
    uint16_t rekeyIntervalMins;
    std::cout << "   Please input rekey time(mins) for Child tunnel:";
    std::cin >> rekeyIntervalMins;
    Utils::validateInput(rekeyIntervalMins);
    child.rekeyIntervalMins = rekeyIntervalMins;

    ipsecConfig.childCfg.emplace_back(child);
}

void IpsecMenu::setSiteToSiteWithoutNATConfig(telux::data::net::IpsecConfig &ipsecConfig) {
    /* IPsec Child SA Config */
    /* IPsec Child entires */
    uint32_t childEntries;
    std::cout << "   Please input number of Child SAs : ";
    std::cin >> childEntries;
    Utils::validateInput(childEntries);
    ipsecConfig.childEntries = childEntries;

    for (int i = 0; i < childEntries; ++i) {
        struct ChildConfig child;
        std::cout << "\n-----------------Child "
                  << i + 1
                  << "------------------\n";

        /* IPsec Child Identifier */
        std::string childIdentifier;
        std::cout << "   Please input the CHILD_identifier of IPsec tunnel:";
        std::getline(std::cin, childIdentifier, delimiter);
        child.childIdentifier = childIdentifier;

        /* IPsec Child Local subnets */
        std::string localAddr;
        std::cout << "   Please input Local Address : ";
        std::getline(std::cin, localAddr, delimiter);
        child.localAddr = localAddr;

        /* IPsec Child Remote subnets */
        std::string remoteAddr;
        std::cout << "   Please input Remote Address : ";
        std::getline(std::cin, remoteAddr, delimiter);
        child.remoteAddr = remoteAddr;

        /* IPsec Child Protocol Type */
        int protocolType;
        std::cout << "   Please input protocol type for local_ts:(1.TCP 2.UDP (0 for no protocol)) ";
        std::cin >> protocolType;
        Utils::validateInput(protocolType, {0, 1, 2});
        child.protocolType = static_cast<IpsecProtocolType>(protocolType);

        /* IPsec Port Range(Only work for TCP/UDP) */
        if (child.protocolType != IpsecProtocolType::NO_PROTOCOL){
            /* IPsec Local Port Range */
            struct IpsecPortRange localPortRange = { 0 , 0 };
            std::cout << "   Please input local start port id(0 for no local port range) : ";
            std::cin >> localPortRange.startPortId;
            Utils::validateInput(localPortRange.startPortId);
            if (localPortRange.startPortId != 0) {
                std::cout << "   Please input local end port id(0 for no local end port id): ";
                std::cin >> localPortRange.endPortId;
                Utils::validateInput(localPortRange.endPortId);
            }
            child.localPortRange = localPortRange;

            /* IPsec Remote Port Range */
            struct IpsecPortRange remotePortRange = { 0 , 0 };
            std::cout << "   Please input remote start port id(0 for no remote port range) : ";
            std::cin >> remotePortRange.startPortId;
            Utils::validateInput(remotePortRange.startPortId);
            if (remotePortRange.startPortId != 0) {
                std::cout << "   Please input remote end port id(0 for no remote end port id): ";
                std::cin >> remotePortRange.endPortId;
                Utils::validateInput(remotePortRange.endPortId);
            }
            child.remotePortRange = remotePortRange;
        }

        /* IPsec Child Hardware Offload Option */
        int hwOffload;
        std::cout << "   Do you want to enable hw_offload:(1.YES 0.NO) ";
        std::cin >> hwOffload;
        Utils::validateInput(hwOffload, {0, 1});
        child.hwOffload = static_cast<bool>(hwOffload);

        /* IPsec Child Start action Option */
        int trapAction;
        std::cout << "   Do you want to set start action as trap:(1.YES 0.NO) ";
        std::cin >> trapAction;
        Utils::validateInput(trapAction, {0, 1});
        child.trapAction = static_cast<bool>(trapAction);

        /* IPsec Child Rekey Time */
        uint16_t rekeyIntervalMins;
        std::cout << "   Please input rekey time(mins) for Child tunnel:";
        std::cin >> rekeyIntervalMins;
        Utils::validateInput(rekeyIntervalMins);
        child.rekeyIntervalMins = rekeyIntervalMins;

        ipsecConfig.childCfg.emplace_back(child);
    }
}