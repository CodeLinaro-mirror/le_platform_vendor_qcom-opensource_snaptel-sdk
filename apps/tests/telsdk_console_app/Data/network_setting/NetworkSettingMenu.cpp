/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

extern "C" {
#include "unistd.h"
}

#include <algorithm>
#include <iostream>

#include <telux/data/DataFactory.hpp>
#include "../../../../common/utils/Utils.hpp"

#include "NetworkSettingMenu.hpp"
#include "../DataUtils.hpp"

using namespace std;

NetworkSettingMenu::NetworkSettingMenu(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
    networkSettingManager_ = nullptr;
    menuOptionsAdded_ = false;
    subSystemStatusUpdated_ = false;
}

NetworkSettingMenu::~NetworkSettingMenu() {
}

bool NetworkSettingMenu::init() {
    telux::common::ServiceStatus subSystemStatus = telux::common::ServiceStatus::SERVICE_FAILED;
    subSystemStatusUpdated_ = false;
    if (networkSettingManager_ == nullptr) {
        auto initCb = std::bind(&NetworkSettingMenu::onInitComplete, this, std::placeholders::_1);
        auto &dataFactory = telux::data::DataFactory::getInstance();
        /* Try both local and remote operation type. If operation type is not supported,
           nullptr is returned. networkSettingManager_ pointer will be associated with
           valid return pointer. */
        auto localNetworkSettingMgr = dataFactory.getNetworkSettingManager(
            telux::data::OperationType::DATA_LOCAL, initCb);
        if (localNetworkSettingMgr) {
            networkSettingManager_ = localNetworkSettingMgr;
        }
        auto remoteNetworkSettingMgr = dataFactory.getNetworkSettingManager(
            telux::data::OperationType::DATA_REMOTE, initCb);

        if (remoteNetworkSettingMgr) {
            networkSettingManager_ = remoteNetworkSettingMgr;
        }
        if(networkSettingManager_ == nullptr ) {
            //Return immediately
            std::cout << "\nError encountered in initializing NetworkSetting Manager" << std::endl;
            return false;
        }
        networkSettingManager_->registerListener(shared_from_this());
    }
    {
        std::unique_lock<std::mutex> lck(mtx_);
        /* NetworkSetting Manager is guaranteed to be valid pointer at this point. If manager
           initialization fails and factory invalidated it's own pointer to NetworkSetting manager
           before reaching this point, reference count of NetworkSetting manager should still
           be 1. */
        telux::common::ServiceStatus subSystemStatus = networkSettingManager_->getServiceStatus();
        if (subSystemStatus == telux::common::ServiceStatus::SERVICE_UNAVAILABLE) {
            std::cout << "\nInitializing Network Setting Manager, Please wait ..." << std::endl;
            cv_.wait(lck, [this]{return this->subSystemStatusUpdated_;});
            subSystemStatus = networkSettingManager_->getServiceStatus();
        }
        //At this point, initialization should be either AVAILABLE or FAIL
        if (subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "\nNetwork Setting Manager is ready" << std::endl;
        }
        else {
            std::cout << "\nNetwork Setting Manager initialization failed" << std::endl;
            networkSettingManager_ = nullptr;
            return false;
        }
    }

    if (menuOptionsAdded_ == false) {
        menuOptionsAdded_ = true;
        std::shared_ptr<ConsoleAppCommand> addPortTriggerEntry =
            std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("1", "add_port_trigger_entry",
            {}, std::bind(&NetworkSettingMenu::addPortTriggerEntry, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> requestPortTriggerEntry =
            std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("2",
                "request_port_trigger_entry",
            {}, std::bind(&NetworkSettingMenu::requestPortTriggerEntry, this,
            std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> deletePortTriggerEntry =
            std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("3",
                "delete_port_trigger_entry",
            {}, std::bind(&NetworkSettingMenu::deletePortTriggerEntry, this,
            std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> updateAlg =
            std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("4", "update_alg",
            {}, std::bind(&NetworkSettingMenu::updateAlg, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> setDataPathOptStatus
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("5",
                "set_Data_Path_Opt_Status",
            {}, std::bind(&NetworkSettingMenu::setDataPathOptStatus, this,
            std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> requestDataPathOptStatus
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("6",
                "request_Data_Path_Opt_Status",
            {}, std::bind(&NetworkSettingMenu::requestDataPathOptStatus, this,
                std::placeholders::_1)));

        std::vector<std::shared_ptr<ConsoleAppCommand>> commandsList = {
            addPortTriggerEntry, requestPortTriggerEntry, deletePortTriggerEntry,
            updateAlg, setDataPathOptStatus, requestDataPathOptStatus };
        addCommands(commandsList);
    }
    ConsoleApp::displayMenu();
    return true;
}

void NetworkSettingMenu::onInitComplete(telux::common::ServiceStatus status) {
    std::lock_guard<std::mutex> lock(mtx_);
    subSystemStatusUpdated_ = true;
    cv_.notify_all();
}

void NetworkSettingMenu::addPortTriggerEntry(std::vector<std::string> inputCommand) {
    telux::common::Status retStat;
    telux::data::net::PortTriggerConfig portTriggerCfg {};
    std::cout << "Add Port Trigger Entry \n";

    std::cout<<"\nPlease enter the Trigger Port Range:";
    int triggerStartPort;
    std::cout<<"  \nEnter the start port: ";
    std::cin >> triggerStartPort;
    Utils::validateInput(triggerStartPort);

    int triggerEndPort;
    std::cout<<"  \nEnter the end port: ";
    std::cin >> triggerEndPort;
    Utils::validateInput(triggerEndPort);

    std::cout<<"\nPlease enter the Forward Port Range:";
    int forwardStartPort;
    std::cout<<"  \nEnter the start port: ";
    std::cin >> forwardStartPort;
    Utils::validateInput(forwardStartPort);

    int forwardEndPort;
    std::cout<<"  \nEnter the end port: ";
    std::cin >> forwardEndPort;
    Utils::validateInput(forwardEndPort);

    int triggerProtocol;
    std::cout<<"     \nPlease input port_trigger_protocol number (6-TCP, 17-UDP) : ";
    std::cin >> triggerProtocol;
    Utils::validateInput(triggerProtocol);

    int forwardProtocol;
    std::cout<<"     \nPlease input port_forward_protocol number (6-TCP, 17-UDP): ";
    std::cin >> forwardProtocol;
    Utils::validateInput(forwardProtocol);

    int timer;
    std::cout<<"     \nPlease enter the timer value in secs for Port Triggering : ";
    std::cin >> timer;
    Utils::validateInput(timer);

    portTriggerCfg.triggerStartPort  =  static_cast<uint16_t>(triggerStartPort);
    portTriggerCfg.triggerEndPort    =  static_cast<uint16_t>(triggerEndPort);
    portTriggerCfg.forwardStartPort  =  static_cast<uint16_t>(forwardStartPort);
    portTriggerCfg.forwardEndPort    =  static_cast<uint16_t>(forwardEndPort);
    portTriggerCfg.triggerProtocol   =  static_cast<uint16_t>(triggerProtocol);
    portTriggerCfg.forwardProtocol   =  static_cast<uint16_t>(forwardProtocol);
    portTriggerCfg.timer             =  (timer);

    //Callback
    auto respCb = [](telux::data::net::PortConfigId &portConfigId,
        telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "addPortTrigger Response"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
        if (error == telux::common::ErrorCode::SUCCESS) {
            std::cout << "Successfully added the port trigger entry with handle "
                      << portConfigId << "\n";
        } else {
            std::cout << "Failed to add the port trigger entry\n";
        }
    };

    retStat = networkSettingManager_->addPortTriggerEntry(portTriggerCfg, respCb);
    Utils::printStatus(retStat);
}

void NetworkSettingMenu::requestPortTriggerEntry(std::vector<std::string> inputCommand) {
    telux::common::Status retStat;

    std::cout << "\nGet Port Trigger Entry\n";

    int inputHandleflag ;
    std::cout<<"\nDo you want to see the full list of entries or "
             <<"just a single entry? (1-Single/0-All):";
    std::cin >> inputHandleflag;
    Utils::validateInput(inputHandleflag,{0,1});

    int handleVal = 0;

    if(inputHandleflag) {
        std::cout<<"Enter the handle: ";
        std::cin >> handleVal;
        Utils::validateInput(handleVal);
    }

    const telux::data::net::PortConfigId portConfigId = handleVal;

    auto respCb = [](const std::vector<telux::data::net::PortTriggerConfig> &portTriggerEntries,
        telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "requestPortTriggerEntry Response"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;

        if (error == telux::common::ErrorCode::SUCCESS) {
            int numEntries = portTriggerEntries.size();
            std::cout << " Number of port trigger entries :" << numEntries << std::endl;
            for (int i = 0; i < numEntries; i++) {
                std::cout<<"\nPort Trigger Handle: "<< portTriggerEntries[i].portConfigId ;
                std::cout<<"\nTrigger Port Start port:End Port is "
                         <<portTriggerEntries[i].triggerStartPort<<":"
                         <<portTriggerEntries[i].triggerEndPort;
                std::cout<<"\nForward Port Start port:End Port is "
                         <<portTriggerEntries[i].forwardStartPort<<":"
                         <<portTriggerEntries[i].forwardEndPort;
                std::cout<<"\nTrigger protcol is "
                    << DataUtils::protocolToString(
                        static_cast<uint8_t>(portTriggerEntries[i].triggerProtocol));
                std::cout<<"\nForward protcol is "
                    << DataUtils::protocolToString(
                        static_cast<uint8_t>(portTriggerEntries[i].forwardProtocol));
                std::cout<<"\nTimer value is "<< portTriggerEntries[i].timer << std::endl;
             }
        }
        else {
           std::cout << "\nPort Trigger Handle list get failed \n";
        }
    };

    retStat = networkSettingManager_->requestPortTriggerEntry(portConfigId, respCb);
    Utils::printStatus(retStat);
}

void NetworkSettingMenu::deletePortTriggerEntry(std::vector<std::string> inputCommand) {
    telux::common::Status retStat;

    std::cout << "\nDelete Port Trigger Entry\n";

    int handle;
    std::cout<<"Enter the port trigger handle to be deleted:";
    std::cin >> handle;
    Utils::validateInput(handle);

    // Callback
    auto respCb = [](telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "deletePortTriggerEntry Response"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful"
                     : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
        if (error == telux::common::ErrorCode::SUCCESS) {
            std::cout<< "\n Successfully deleted the port trigger entry \n";
        }
        else {
            std::cout<< "\n Failed to delete the port trigger entry \n";
        }
    };

    retStat = networkSettingManager_->deletePortTriggerEntry(handle, respCb);
    Utils::printStatus(retStat);
}

void NetworkSettingMenu::updateAlg(std::vector<std::string> inputCommand) {
    telux::common::Status retStat = telux::common::Status::SUCCESS;
    bool algEnable = false;
    telux::data::net::AlgType algType;

    std::cout << "Update Alg \n";

    int algTypeVal = 0, algTypeMask= 0;
    std::cout << "Select the Type of ALG : \n"
              << static_cast<int>(telux::data::net::AlgType::RTSP)<<":RTSP\n"
              << static_cast<int>(telux::data::net::AlgType::SIP)<<":SIP \n"
              << "0: EXIT "<<std::endl;
    std::cin >> algTypeVal;
    Utils::validateInput(algTypeVal,
        {0, static_cast<int>(telux::data::net::AlgType::RTSP),
        static_cast<int>(telux::data::net::AlgType::SIP)});

    while(algTypeVal != 0) {
        if (algTypeVal < 1 ||
          algTypeVal > 2){
          std::cout<< "\nInvalid alg Type" << algTypeVal;
          break;
        }

        if (algTypeVal == 1){
         algTypeMask = algTypeMask | 1;
        } else if (algTypeVal == 2) {
         algTypeMask = algTypeMask | 2;
        }

        std::cout << "Select the Type of ALG : \n"
                  << static_cast<int>(telux::data::net::AlgType::RTSP) <<":RTSP\n"
                  << static_cast<int>(telux::data::net::AlgType::SIP)  <<":SIP \n"
                  << "0: EXIT"<<std::endl;
        std::cin >> algTypeVal;
        Utils::validateInput(algTypeVal,
            {0, static_cast<int>(telux::data::net::AlgType::RTSP),
            static_cast<int>(telux::data::net::AlgType::SIP)});
     }

    algType = static_cast<telux::data::net::AlgType>(algTypeMask);

    int enableAlgFlag;
    std::cout << "Please input ALG State (1-Enable/0-Disable) : ";
    std::cin >> enableAlgFlag;
    Utils::validateInput(enableAlgFlag, {0, 1});
    if (enableAlgFlag) {
        algEnable = true;
    }

    // Callback
    auto respCb = [enableAlgFlag](telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "updateAlg "
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
        if (error == telux::common::ErrorCode::SUCCESS) {
            if(enableAlgFlag){
                std::cout<< "ALG Enable succeeds"<< std::endl;
            }
            else {
                std::cout<< "ALG Disable succeeds"<< std::endl;
            }
        } else {
            if(enableAlgFlag){
                std::cout<< "ALG Enable Fails"<< std::endl;
            }
            else {
                std::cout<< "ALG Disable Fails"<< std::endl;
            }
        }
    };

    retStat = networkSettingManager_->updateAlg(algType, algEnable, respCb);
    Utils::printStatus(retStat);
}

void NetworkSettingMenu::setDataPathOptStatus(std::vector<std::string> inputCommand) {
    std::cout << "\nSet Data Path Opt Status" << std::endl;
    telux::common::Status retStat = telux::common::Status::SUCCESS;

    int dataPathOptStatusFlag;
    bool dataPathOptStatus = false; //DEFUALT
    std::cout << "\n Please input 1-[enable]/0-[disable]:";
    std::cin >> dataPathOptStatusFlag;
    Utils::validateInput(dataPathOptStatusFlag, {0, 1});
    if (dataPathOptStatusFlag) {
        dataPathOptStatus = true;
    }

    // Callback
    auto respCb = [](telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "setDataPathOptStatus Response"
                  << ((error == telux::common::ErrorCode::SUCCESS) ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
        if (error == telux::common::ErrorCode::SUCCESS) {
              std::cout<<"\nData path opt status set successfully "<<std::endl;
        } else {
              std::cout <<"Set data path opt  status fails"<<std::endl;
        }
    };

    retStat = networkSettingManager_->setDataPathOptStatus(dataPathOptStatus, respCb);
    Utils::printStatus(retStat);
}

void NetworkSettingMenu::requestDataPathOptStatus(std::vector<std::string> inputCommand) {
    std::cout << "\nGet Data PathOpt Status" << std::endl;
    telux::common::Status retStat = telux::common::Status::SUCCESS;

    // Callback
    auto respCb = [](bool &dataPathOptStatus, telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                    << "requestDataPathOptStatus Response"
                    << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed");
        if (error == telux::common::ErrorCode::SUCCESS) {
            if(dataPathOptStatus)
              std::cout<<"\nData optimization handler enabled\n";
            else
              std::cout<<"\nData optimization handler not enabled\n";
        } else {
              std::cout << "ErrorCode: " << static_cast<int>(error)
                        << ", description: "
                        << Utils::getErrorCodeAsString(error) << std::endl;
        }
    };

    retStat = networkSettingManager_->requestDataPathOptStatus(respCb);
    Utils::printStatus(retStat);
}

