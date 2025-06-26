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

        std::shared_ptr<ConsoleAppCommand> addSWIpChannelConfig
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("7",
                "addSWIpChannelConfig",
            {}, std::bind(&NetworkSettingMenu::addSWIpChannelConfig, this,
                std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> removeSWIpChannelConfig
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("8",
                "removeSWIpChannelConfig",
            {}, std::bind(&NetworkSettingMenu::removeSWIpChannelConfig, this,
                std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> requestSWIpChannelConfig
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("9",
                "requestSWIpChannelConfig",
            {}, std::bind(&NetworkSettingMenu::requestSWIpChannelConfig, this,
                std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> allowIpFamily
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("10",
                "allowIpFamily",
            {}, std::bind(&NetworkSettingMenu::allowIpFamily, this,
                std::placeholders::_1)));


        std::shared_ptr<ConsoleAppCommand> addDHCPReservationRecord
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("11",
                "addDHCPReservationRecord",
            {}, std::bind(&NetworkSettingMenu::addDHCPReservationRecord, this,
                std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> editDHCPReservationRecord
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("12",
                "editDHCPReservationRecord",
            {}, std::bind(&NetworkSettingMenu::editDHCPReservationRecord, this,
                std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> requestDHCPReservationRecords
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("13",
                "requestDHCPReservationRecords",
            {}, std::bind(&NetworkSettingMenu::requestDHCPReservationRecords, this,
                std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> deleteDHCPReservationRecord
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("14",
                "deleteDHCPReservationRecord",
            {}, std::bind(&NetworkSettingMenu::deleteDHCPReservationRecord, this,
                std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> activateLAN
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("15",
                "activateLAN",
            {}, std::bind(&NetworkSettingMenu::activateLAN, this,
                std::placeholders::_1)));


        std::vector<std::shared_ptr<ConsoleAppCommand>> commandsList = {
            addPortTriggerEntry, requestPortTriggerEntry, deletePortTriggerEntry,
            updateAlg, setDataPathOptStatus, requestDataPathOptStatus,
            addSWIpChannelConfig, removeSWIpChannelConfig, requestSWIpChannelConfig,
            allowIpFamily, addDHCPReservationRecord, editDHCPReservationRecord,
            requestDHCPReservationRecords, deleteDHCPReservationRecord, activateLAN
        };
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

void NetworkSettingMenu::addSWIpChannelConfig(std::vector<std::string> inputCommand) {
    std::cout << "\n Add SWIP Channel Config " << std::endl;
    telux::common::Status retStat = telux::common::Status::SUCCESS;
    telux::data::net::SWIpChannelConfig swIpChCfg {} ;

    std::cout<<"   Please input logical IF(eg: mhi_swip0), Max Length - "
             << MAX_IFACE_NAME_SIZE-1 <<": ";
    setIfaceName(swIpChCfg.ifName);

    char delimiter = '\n';
    std::string staticIpAddr;
    std::cout << "   Please input ip address (xxx.xxx.xxx.xxx) for interface."
              "   (EX: 169.250.25.26): ";
    std::getline(std::cin, staticIpAddr, delimiter);
    swIpChCfg.staticIpAddr= staticIpAddr ;
    std::cout<<"   Default netmask of 255.255.255.252 will be applied!"<<std::endl;

    // Callback
    auto respCb = [](telux::common::ErrorCode error) {
     std::cout << std::endl << std::endl;
     std::cout << "CALLBACK: "
                 << "addSWIpChannelConfig Response"
                 << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                 << ". ErrorCode: " << static_cast<int>(error)
                 << ", description: " << Utils::getErrorCodeAsString(error) << std::endl ;
                 if (error == telux::common::ErrorCode::SUCCESS) {
                     std::cout << "\n SW IP channel is set up successfully \n";
                 } else {
                     std::cout << "\n SW IP channel is set up failed \n";
                 }
    };

    retStat = networkSettingManager_->addSWIpChannelConfig(swIpChCfg, respCb);
    Utils::printStatus(retStat);
}

void NetworkSettingMenu::removeSWIpChannelConfig(std::vector<std::string> inputCommand) {
    std::cout << "\n Remove SWIP Channel Config " << std::endl;
    telux::common::Status retStat = telux::common::Status::SUCCESS;
    string ifName {} ;

    std::cout<<"   Please input logical IF(eg: mhi_swip0), Max Length - "
             << MAX_IFACE_NAME_SIZE-1 <<": ";
    setIfaceName(ifName);

    // Callback
    auto respCb = [](telux::common::ErrorCode error) {
    std::cout << std::endl << std::endl;
    std::cout << "CALLBACK: "
              << "removeSWIpChannelConfig Response"
              << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
              << ". ErrorCode: " << static_cast<int>(error)
              << ", description: " << Utils::getErrorCodeAsString(error) << std::endl ;
              if (error == telux::common::ErrorCode::SUCCESS) {
                  std::cout << "\n Successfully deleted configuration parameters \n";
              } else {
                  std::cout << "\n Delete operation failed \n";
              }
    };

    retStat = networkSettingManager_->removeSWIpChannelConfig(ifName, respCb);
    Utils::printStatus(retStat);
}

void NetworkSettingMenu::requestSWIpChannelConfig(std::vector<std::string> inputCommand) {
    std::cout << "\nRequest SWIp Channel Config" << std::endl;
    telux::common::Status retStat = telux::common::Status::SUCCESS;

    auto respCb = [](const telux::data::net::SWIpChannelConfig& swIpChCfg,
                     telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                    << "requestSWIpChannelConfig Response"
                    << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                    << ". ErrorCode: " << static_cast<int>(error)
                    << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
        if (error == telux::common::ErrorCode::SUCCESS) {
            if (swIpChCfg.ifName.size() != 0 && swIpChCfg.staticIpAddr.size() != 0 ) {
               std::cout<<"\n SW IP Channel Interface name : "<< swIpChCfg.ifName <<"\n";
               std::cout<<"\n SW IP Channel Interface IP address:  : "<< swIpChCfg.staticIpAddr <<"\n";
               std::cout<<"\n SW IP Channel Neighbor link local address : "<< swIpChCfg.neighLinkLocalAddr <<"\n";
               std::cout<<"\n SW IP Channel Neighbor IP address:  : "<< swIpChCfg.neighIpAddr <<"\n";
            } 
            else {
                std::cout<<" SW IP config parameters are not present "<<std::endl;
            }
        }   else {
                std::cout<<"\n ERROR: Did not receive SW IP Channel config "<< std::endl;
        }
    };

    retStat = networkSettingManager_->requestSWIpChannelConfig(respCb);
    Utils::printStatus(retStat);
}

void NetworkSettingMenu::allowIpFamily(std::vector<std::string> inputCommand) {
    std::cout << "\n Allow IP Family " << std::endl;
    telux::common::Status retStat = telux::common::Status::SUCCESS;
    bool allow = false;

    int allowIP;
    std::cout << "Enter (1-Allow IP/0-To Dsiallow IP) : \n";
    std::cin >> allowIP;
    Utils::validateInput(allowIP,{0,1});
    if(allowIP == 1) {
        allow = true;
    }

    telux::data::IpFamilyType ipFamilyType = static_cast<telux::data::IpFamilyType>((int)getIpFamilyTypeV4V6());

    auto respCb = [](telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                    << "allowIpFamily Response"
                    << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                    << ". ErrorCode: " << static_cast<int>(error)
                    << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
    };

    retStat = networkSettingManager_->allowIpFamily(allow, ipFamilyType, respCb);
    Utils::printStatus(retStat);
}

void NetworkSettingMenu::addDHCPReservationRecord(std::vector<std::string> inputCommand) {
    std::cout << "\n Add Prefix Delegation Config " << std::endl;
    telux::common::Status retStat = telux::common::Status::SUCCESS;
    telux::data::net::DHCPReservationInfo dhcpReserveInfo {};

    int device_type = 0;
    std::cout<<"Enter the device type(0-USB/1-Other LAN clients):   ";
    std::cin >> device_type;
    Utils::validateInput(device_type);

    std::string clientDeviceName;
    std::string clientMacAddr;
    std::string clientReservedIp;
    char delimiter = '\n';

    if(device_type == 1) {
        std::cout<<"Please input the MAC address :";
        std::getline(std::cin, clientMacAddr, delimiter);
    }

    std::cout<<"\nPlease input the client reserved IP(xxx.xxx.xxx.xxx) : ";
    std::getline(std::cin, clientReservedIp, delimiter);

    if(device_type == 0){
        std::cout<<" \nEnter device name USB client : "<< std::endl;
        std::getline(std::cin, clientDeviceName, delimiter);
    }

    int enableReservationFlag;
    bool enableReservation = false;
    std::cout << "Enable/disable reservation for this client(1-Enable/0-Disable): ";
    std::cin >> enableReservationFlag;
    Utils::validateInput(enableReservationFlag, {0, 1});
    if (enableReservationFlag) {
        enableReservation = true;
    }

    dhcpReserveInfo.clientDeviceName = clientDeviceName;
    dhcpReserveInfo.clientMacAddr = clientMacAddr;
    dhcpReserveInfo.clientReservedIp = clientReservedIp;
    dhcpReserveInfo.enable = enableReservation;

    auto respCb = [](telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                << "addDHCPReservRecord Response"
                << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                << ". ErrorCode: " << static_cast<int>(error)
                << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
        if (error == telux::common::ErrorCode::SUCCESS) {
            std::cout << "\n DHCP Reservation Record added successfully \n";
        } else {
            std::cout << "\n Failed to add DHCP Reservation record \n";
        }
    };

    retStat = networkSettingManager_->addDHCPReservationRecord(dhcpReserveInfo, respCb);
    Utils::printStatus(retStat);
}

void NetworkSettingMenu::editDHCPReservationRecord(std::vector<std::string> inputCommand) {
    std::cout << "\n Edit DHCP Reservation Record " << std::endl;
    telux::common::Status retStat = telux::common::Status::SUCCESS;
    telux::data::net::DHCPReservationInfo dhcpReserveInfo {};

    char delimiter = '\n';
    std::string addrToEdit;
    std::cout<< "Please input the client reserved IP(xxx.xxx.xxx.xxx) :";
    std::getline(std::cin, addrToEdit, delimiter);

    while(true){
       int options;
       std::cout<<"\nPlease enter the field to edit:  ";
       std::cout<<"\n\t1. MAC Address\n\t2. IP Addr\n\t3. Device Name\n\t4. Enable/Disable\n\t:";
       std::cin >> options;
       Utils::validateInput(options, {1, 2, 3, 4});
       std::string clientMacAddr;
       std::string clientReservedIp;
       std::string clientDeviceName;
       int enableReservationFlag;
       switch (options) {
         case 1:
             std::cout<<"Please input the MAC address :";
             std::getline(std::cin, clientMacAddr, delimiter);
             dhcpReserveInfo.clientMacAddr = clientMacAddr;
             break;
         case 2:
             std::cout<<"\nPlease input the client reserved IP(xxx.xxx.xxx.xxx) :";
             std::getline(std::cin, clientReservedIp, delimiter);
             break;
         case 3:
             while(true) {
                 std::cout<<"Please input the device name :";
                 std::getline(std::cin, clientDeviceName, delimiter);
                 dhcpReserveInfo.clientDeviceName = clientDeviceName;
                 if(clientDeviceName.size() > 0){ //check if enter is blocked or not
                     break;
                 }
                 else{
                    std::cout<<"\nInvalid Device name entered"<<std::endl;
                 }
             }
             break;
         case 4:
             std::cout<<"Enable/disable reservation for this client(1-Enable/0-Disable) :";
             dhcpReserveInfo.enable = false;
             std::cin >> enableReservationFlag;
             Utils::validateInput(enableReservationFlag, {0, 1});
             if (enableReservationFlag) {
                 dhcpReserveInfo.enable = true;
             }
             break;
         default:
             std::cout<<"Invalid response \n" << options << std::endl;
       }

       int continueFlag = 0;
       std::cout<<"Do you wish to Edit more fields(Enter-(0 to skipped/1-to continue):";
       std::cin >> continueFlag;
       Utils::validateInput(continueFlag, {0, 1});
       if ( continueFlag == 0){
          break;
       }
       else {
          continue;
       }
    }

    auto respCb = [](telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                << "editDHCPReservRecord Response"
                << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                << ". ErrorCode: " << static_cast<int>(error)
                << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
        if (error == telux::common::ErrorCode::SUCCESS) {
            std::cout << std::endl;
            std::cout << "DHCP  Reservation Record edited successfully"<< std::endl;
        } else {
            std::cout << "Failed to edit DHCP Reservation record." << std::endl;
        }
    };

    retStat = networkSettingManager_->editDHCPReservationRecord(addrToEdit, dhcpReserveInfo, respCb);
    Utils::printStatus(retStat);
}

void NetworkSettingMenu::requestDHCPReservationRecords(std::vector<std::string> inputCommand) {
    std::cout << "\n Request DHCP Reservation Records " << std::endl;
    telux::common::Status retStat = telux::common::Status::SUCCESS;

    auto respCb = [](const std::vector<telux::data::net::DHCPReservationInfo>& records,
                     telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                << "requestDHCPReservationRecords Response"
                << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                << ". ErrorCode: " << static_cast<int>(error)
                << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;

        if (error == telux::common::ErrorCode::SUCCESS) {
            int numEntries = records.size();
            if ( numEntries == 0 )  {
              std::cout <<" No DHCP Reservation Records" << std::endl;
            } else {
                for (int i = 0;i < numEntries; i++){
                 std::cout <<"\n Entry number : "<< i << std::endl;
                 std::cout <<"MAC address of the client: "<< records[i].clientMacAddr << std::endl;
                 std::cout <<"IP address of the client: "<< records[i].clientReservedIp << std::endl;
                 std::cout <<"Device Name of the client: "<< records[i].clientDeviceName << std::endl;
                 std::cout <<"DHCP Reservation enabled: " << records[i].enable <<std::endl;
               }
             }
          }
    };

    retStat = networkSettingManager_->requestDHCPReservationRecords(respCb);
    Utils::printStatus(retStat);
}

void NetworkSettingMenu::deleteDHCPReservationRecord(std::vector<std::string> inputCommand) {
    std::cout << "\n Add Prefix Delegation Config " << std::endl;
    telux::common::Status retStat = telux::common::Status::SUCCESS;

    char delimiter = '\n';
    std::string addrToEdit;
    std::cout<< "Please input the client reserved IP(xxx.xxx.xxx.xxx)  :";
    std::getline(std::cin, addrToEdit, delimiter);

    auto respCb = [](telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                << "deleteDHCPReservationRecord Response"
                << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                << ". ErrorCode: " << static_cast<int>(error)
                << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
        if (error == telux::common::ErrorCode::SUCCESS) {
            std::cout << std::endl;
            std::cout << "DHCP  Reservation Record deleted successfully"<< std::endl;
        } else {
            std::cout << "Failed to delete DHCP Reservation record." << std::endl;
        }
    };

    retStat = networkSettingManager_->deleteDHCPReservationRecord(addrToEdit, respCb);
    Utils::printStatus(retStat);
}

void NetworkSettingMenu::activateLAN(std::vector<std::string> inputCommand) {
    telux::common::Status retStat = telux::common::Status::SUCCESS;
    std::cout << "ActivateLAN Triggered \n";

    // Callback
    auto respCb = [](telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "activateLAN Response"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
    };

    retStat = networkSettingManager_->activateLAN(respCb);
    Utils::printStatus(retStat);
}

int NetworkSettingMenu::getIpFamilyTypeV4V6() {
    int ipFamilyType;
    std::cout << "Enter Ip Family (4-IPv4, 6-IPv6): ";
    std::cin >> ipFamilyType;
    Utils::validateInput(ipFamilyType, {static_cast<int>(telux::data::IpFamilyType::IPV4),
        static_cast<int>(telux::data::IpFamilyType::IPV6)});
    return ipFamilyType;
}

bool NetworkSettingMenu::setIfaceName(std::string &interfaceName) {
    std::string ifaceName;
    char delimiter = '\n';

    std::getline(std::cin, ifaceName, delimiter);

    if(ifaceName.length() == 0 || ifaceName.length() >= MAX_IFACE_NAME_SIZE-1) {
        std::cout<<"ERROR: Invalid Size, Supports only 1-16 char interface size\n";
        return false;
    } else {
        //Special char check
        if(Utils::validateCharString(ifaceName)) {
            interfaceName = ifaceName;
        }  else {
            std::cout<<"\nERROR: special char found in interface name\n";
            return false;
        }
    }
    return true;
}


