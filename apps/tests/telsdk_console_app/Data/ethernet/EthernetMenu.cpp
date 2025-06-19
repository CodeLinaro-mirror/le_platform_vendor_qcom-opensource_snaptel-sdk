/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <algorithm>

#include <telux/data/DataFactory.hpp>
#include "../../../../common/utils/Utils.hpp"

#include "EthernetMenu.hpp"

using namespace std;

EthernetMenu::EthernetMenu(std::string appName, std::string cursor)
   :ConsoleApp(appName, cursor) {
    ethernetManager_ = nullptr;
    menuOptionsAdded_ = false;
    subSystemStatusUpdated_ = false;
    ethConfigValid_= false;
    ethConfigStatusUpdated_ = false;
}

EthernetMenu::~EthernetMenu() {
    ethernetManager_->deregisterListener(shared_from_this());
}

bool EthernetMenu::init() {

    telux::common::ServiceStatus subSystemStatus = telux::common::ServiceStatus::SERVICE_FAILED;
    subSystemStatusUpdated_ = false;

    if (ethernetManager_ == nullptr) {
        auto initCb = std::bind(&EthernetMenu::onInitComplete, this, std::placeholders::_1);
        auto &dataFactory = telux::data::DataFactory::getInstance();

        auto localEthernetMgr = dataFactory.getEthernetManager(
            telux::data::OperationType::DATA_LOCAL, initCb);
        if (localEthernetMgr ) {
            ethernetManager_ = localEthernetMgr ;
        }

        auto remoteEthernetMgr = dataFactory.getEthernetManager(
            telux::data::OperationType::DATA_REMOTE, initCb);
        if (remoteEthernetMgr) {
            ethernetManager_ = remoteEthernetMgr;
        }

        if(ethernetManager_ == nullptr ) {
            //Return immediately
            std::cout << "\nError encountered in initializing Ethernet Manager" << std::endl;
            return false;
        }
    }

    {
        std::unique_lock<std::mutex> lck(mtx_);

        telux::common::ServiceStatus subSystemStatus = ethernetManager_->getServiceStatus();
        if (subSystemStatus == telux::common::ServiceStatus::SERVICE_UNAVAILABLE) {
            std::cout << "\nInitializing Ethernet Manager, Please wait ..." << std::endl;
            cv_.wait(lck, [this]{return this->subSystemStatusUpdated_;});
            subSystemStatus = ethernetManager_->getServiceStatus();
        }
        //At this point, initialization should be either AVAILABLE or FAIL
        if (subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "\nEthernet Manager is ready" << std::endl;
        }
        else {
            std::cout << "\nEthernet Manager initialization failed" << std::endl;
            ethernetManager_ = nullptr;
            return false;
        }
        ethernetManager_->registerListener(shared_from_this());
    }

    if (menuOptionsAdded_ == false) {
        menuOptionsAdded_ = true;
        std::shared_ptr<ConsoleAppCommand> setEthernetConfig
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("1", "set_ethernet_config", {},
                std::bind(&EthernetMenu::setEthernetConfig, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> requestEthernetConfig
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("2", "request_ethernet_config",
            {}, std::bind(&EthernetMenu::requestEthernetConfig, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> enableMacsec
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("3", "enable_Macsec", {},
                std::bind(&EthernetMenu::enableMacsec, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> disableMacsec
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("4", "disable_Macsec", {},
                std::bind(&EthernetMenu::disableMacsec, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> requestMacsecConfig
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("5", "request_macsec_Config",
            {}, std::bind(&EthernetMenu::requestMacsecConfig, this, std::placeholders::_1)));

        std::vector<std::shared_ptr<ConsoleAppCommand>> commandsList = {setEthernetConfig,
            requestEthernetConfig, enableMacsec, disableMacsec, requestMacsecConfig};
        addCommands(commandsList);
    }
    ConsoleApp::displayMenu();
    return true;
}

void EthernetMenu::onInitComplete(telux::common::ServiceStatus status) {
    std::lock_guard<std::mutex> lock(mtx_);
    subSystemStatusUpdated_ = true;
    cv_.notify_all();
}

void EthernetMenu::setEthernetConfig(std::vector<std::string> inputCommand) {
    telux::common::Status retStat = telux::common::Status::FAILED;
    telux::data::net::EthConfig ethConfig {};
    char delimiter = '\n';
    int wanNicCount = 0;
    int newNicConfig = 0;
    std::cout << "Set Ethernet Config entry\n";

     //If configuring ETH NIC for the first time or User doesn't want to use the existing conf
    std::cout << "Do you want add new ETH NIC config (1-Yes / 0-No): ";
    std::cin >> newNicConfig;
    Utils::validateInput(newNicConfig,{0,1});

    //Set ETH NIC config for the request
    if (newNicConfig) {
        int noOfNics;
        std::cout<<"\nHow many ETH NIC's you want to configure (Max NIC supported - "
                 << MAX_ETH_NICS << ") : ";
        std::cin >> noOfNics;
        Utils::validateInput(noOfNics,{1,2});

        if (noOfNics > 0 && noOfNics <= MAX_ETH_NICS) {
            std::cout<<"Enter Eth NIC Iface and network type for all " << noOfNics <<" "
                     <<"Eth Interfaces\n";
            ethConfig.configList.resize(noOfNics);

            for (uint8_t i = 0; i < noOfNics ; i++) {
                cout<<"\n----- NICNo.: "<< i+1 <<" -----\n";
                setEthNicIfaceName(ethConfig.configList[i].ifName);

                std::cout<< "\n   Enter NIC ("<< i+1 <<"-"<< ethConfig.configList[i].ifName
                         <<") "<< "connectivity type (0-LAN/1-WAN):";

                if(setEthNicType(ethConfig.configList[i].type) == 1) {
                    wanNicCount++;
                }

                if(wanNicCount > 1) {
                    cout<< " Multiple NICs("<< wanNicCount <<") configured for WAN connectivity"
                        <<" -- Not supported \n";
                    return;
                }
            }
        }
        else {
            std::cout << "   Entered number of Ethernet Interfaces is not in range 1-"
                      << MAX_ETH_NICS <<"\n";
            return;
        }
    }

    // Get ETH NIC configuration
    if (!newNicConfig) {
        this->ethConfigStatusUpdated_ = false;
        auto respCb = [this, &ethConfig](const telux::data::net::EthConfig& ethConfigRsp,
                      telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "existing EthConfig Retrieval Response"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
            if (error == telux::common::ErrorCode::SUCCESS) {
                this->ethConfigValid_= true;
                ethConfig = ethConfigRsp;
                this->displayEthernetNicConfig(ethConfigRsp);
            }
            {
                std::lock_guard<std::mutex> lock(mtx_);
                this->ethConfigStatusUpdated_ = true;
                cv_.notify_one();
            }
        };

        retStat = ethernetManager_->requestEthernetConfig(respCb);
        {
          std::unique_lock<std::mutex> lck(mtx_);
          cv_.wait(lck, [this]{return this->ethConfigStatusUpdated_;});
        }

        if(this->ethConfigValid_) {
            std::cout<< "Do want to change NIC connectivity type (1-Yes/0-No):";
            int newNicConnectivity;
            std::cin >> newNicConnectivity;
            Utils::validateInput(newNicConnectivity,{0,1});

            wanNicCount = 0 ;

            if(newNicConnectivity) {
                for (int i = 0; i < ethConfig.configList.size() ; i++) {
                    std::cout<< "\n   Enter NIC ("<< i+1 <<"-"<<ethConfig.configList[i].ifName
                             <<") "<< "connectivity type (0-LAN/1-WAN): ";
                    if(setEthNicType(ethConfig.configList[i].type) == 1) {
                        wanNicCount++;
                    }
                }

                if(wanNicCount > 1) {
                    cout<< " Multiple NICs("<< wanNicCount <<") configured for WAN connectivity"
                        << "-- Not supported \n";
                    return;
                }
            }
        }
        else {
            std::cout<<"Error in fetching existing Eth Config \n";
            Utils::printStatus(retStat);
            return;
        }
    }

    //Input Mode
    int ethernetMode;
    std::cout << "Enter Eth mode (0-LAN, 1-WAN, 2-WAN_LAN) : ";
    std::cin >> ethernetMode;
    Utils::validateInput(ethernetMode, {static_cast<int>(telux::data::net::EthMode ::WAN),
        static_cast<int>(telux::data::net::EthMode::LAN),
        static_cast<int>(telux::data::net::EthMode::WAN_LAN)});
    ethConfig.mode = static_cast<telux::data::net::EthMode>(ethernetMode);

    //Invalid Config Check
    if (ethConfig.mode == telux::data::net::EthMode::WAN ||
        ethConfig.mode == telux::data::net::EthMode::WAN_LAN) {
        if ( wanNicCount > 1) {
            std::cout<<"\nMultiple NICs configured for WAN connectivity -- Not supported"
                   << wanNicCount ;
            return ;
        }
        if ( wanNicCount == 0) {
            std::cout<<"\nNo NIC configured for WAN connectivity in WAN_LAN mode "
                     <<"-- Invalid Config\n";
            return ;
        }
    }
    else if (ethConfig.mode == telux::data::net::EthMode::LAN) {
        if ( wanNicCount > 0) {
            std::cout<<"\nNIC configured as WAN connectivity type in LAN+LAN mode "
                     <<" -- Invalid config"<<std::endl;
            return ;
        }
    }

    // Callback
    auto respCb = [](telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "setEthernetConfig Response"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
        if (error == telux::common::ErrorCode::SUCCESS) {
             std::cout<<"ETH NIC('s) Config/Mobile AP Ethernet has been set"<<std::endl;
        } else {
             std::cout<<"Failed to Set Ethernet Mode"<<std::endl;
        }

    };

    std::cout<<"\n\nBelow configuration is entered by the user: ";
    this->displayEthernetNicConfig(ethConfig);
    retStat = ethernetManager_->setEthernetConfig(ethConfig, respCb);
    Utils::printStatus(retStat);
}

void EthernetMenu::requestEthernetConfig(std::vector<std::string> inputCommand) {
    telux::common::Status retStat;
    std::cout << "Request EthernetNic Conig \n";

    // Callback
    auto respCb = [this](const telux::data::net::EthConfig& ethConfig,
                         telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "requestEthernetConfig Response"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
        if (error == telux::common::ErrorCode::SUCCESS) {
             this->displayEthernetNicConfig(ethConfig);
        }
    };

    retStat = ethernetManager_->requestEthernetConfig(respCb);
    Utils::printStatus(retStat);
}

void EthernetMenu::enableMacsec(std::vector<std::string> inputCommand) {
    telux::common::Status retStat = telux::common::Status::FAILED;
    telux::data::net::MacsecConfig macsecConfig  {};
    std::cout << "Enable Macsec \n";

    //Setting MACSec configuration from user
    std::cout<<"\n Do you want set new MACsec config (1-Yes / 0-No): ";
    int newMacsecConfig;
    std::cin >> newMacsecConfig;
    Utils::validateInput(newMacsecConfig,{0,1});
    if (newMacsecConfig) {
       setMacsecConfig(macsecConfig);
    }

    // Callback
    auto respCb = [](telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "enableMacsec Response"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
    };

    std::cout<<"\n\nBelow configuration is entered by the user: \n";
    this->displayMacSecConfig(macsecConfig);
    retStat = ethernetManager_->enableMacsec(macsecConfig, respCb);
    Utils::printStatus(retStat);
}

void EthernetMenu::disableMacsec(std::vector<std::string> inputCommand) {
    telux::common::Status retStat = telux::common::Status::FAILED;
    std::cout << "Disable Macsec\n";

    //Enter the Eth IFace to disable the Macsec config for
    std::string ethNicIfName;
    setEthNicIfaceName(ethNicIfName);

    // Callback
    auto respCb = [](telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "disableMacsec Response"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
    };

    std::cout<<"\n\nBelow configuration is entered by the user: \n";
    retStat = ethernetManager_->disableMacsec(ethNicIfName, respCb);
    Utils::printStatus(retStat);
}

void EthernetMenu::requestMacsecConfig(std::vector<std::string> inputCommand) {
    telux::common::Status retStat;
    std::cout << "Request MacSec Config \n";

    // Callback
    auto respCb = [this](const telux::data::net::MacsecConfig& macsecConfig,
                         telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "requestMacsecConfig Response"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
        if (error == telux::common::ErrorCode::SUCCESS) {
             this->displayMacSecConfig(macsecConfig);
        }
    };

    retStat = ethernetManager_->requestMacsecConfig(respCb);
    Utils::printStatus(retStat);
}

void EthernetMenu::displayEthernetNicConfig(const telux::data::net::EthConfig &ethConfig){
    //Print Eth Mode
    std::cout << "\n\n------ Display Eth NIC config ------\n";

    std::cout << "\   Eth Mode : ";
    if (ethConfig.mode == telux::data::net::EthMode::LAN) {
         std::cout<< "LAN Router\n";
     } else if (ethConfig.mode == telux::data::net::EthMode::WAN) {
         std::cout<< "WAN Router\n";
     } else if (ethConfig.mode == telux::data::net::EthMode::WAN_LAN) {
         std::cout<< "WAN LAN Router\n";
     } else {
         std::cout<<"Invalid Eth Mode"<<static_cast<int>(ethConfig.mode)<<"\n";
    }

    //Print Eth Nic Config
    int noOfNics = ethConfig.configList.size();
    if(noOfNics > 0 && noOfNics <= MAX_ETH_NICS) {
        std::cout<<"\n   NICNo     NICType     NICIFace  \n";
        for (uint32_t i = 0; i < noOfNics; i++) {
            std::cout<<"   "<< i <<"         ";
            if (ethConfig.configList[i].type == telux::data::net::EthNetworkType::LAN) {
                std::cout<< "LAN         ";
            } else if (ethConfig.configList[i].type == telux::data::net::EthNetworkType::WAN) {
                std::cout<< "WAN         ";
            } else {
                std::cout<<static_cast<int>(ethConfig.mode)<<"        ";
            }
            std::cout<<ethConfig.configList[i].ifName << "     \n";
        }
    }
    else if (noOfNics == 0) {
        std::cout<<"No Eth Config present\n";
    }
    std::cout<< std::endl << std::endl;
}

void EthernetMenu::displayMacSecConfig(const telux::data::net::MacsecConfig &macsecConfig)
{
    //Print Eth Mode
    std::cout << "\n------ Display MacSec config ------\n";

    //Print Mac Config
    int macConfigLen = macsecConfig.configList.size();
    if(macConfigLen > 0 && macConfigLen <= MAX_ETH_NICS) {
    std::cout<<"  NICNo     NICIFace     MACsecState      MACsecIface     MACsecMode"
               <<"      MTU(byte)of EthIFace \n";
      for (uint32_t i = 0; i < macConfigLen ; i++) {
          std::cout<<"   "<< i << "         ";
          std::cout<< macsecConfig.configList[i].ethNicIfName << "         ";
          if (macsecConfig.configList[i].opr == telux::data::net::MacsecOp::DISABLE) {
             std::cout<< "Disable          ";
          } else if (macsecConfig.configList[i].opr == telux::data::net::MacsecOp::ENABLE) {
             std::cout<< "Enable           ";
          } else if (macsecConfig.configList[i].opr == telux::data::net::MacsecOp::RESTART) {
             std::cout<< "Restart          ";
          } else {
             std::cout<<"Invalid MACsecState"<<static_cast<int>(macsecConfig.configList[i].opr)
                      <<"     ";
          }
          std::cout<< macsecConfig.configList[i].macsecIfName << "           ";
          if (macsecConfig.configList[i].mode == telux::data::net::MacsecMode::SUPPLICANT) {
             std::cout<< "SUPPLICANT     ";
          } else if (macsecConfig.configList[i].mode ==
                        telux::data::net::MacsecMode::AUTHENTICATOR) {
             std::cout<< "AUTHENTICATOR     ";
          } else {
             std::cout<<" Invalid Macsecmode     "<<static_cast<int>(macsecConfig.configList[i].mode) 
                      <<"     ";
          }
         std::cout<< macsecConfig.configList[i].mtuSize << " \n";
     }
    }
    else if (macConfigLen == 0) {
        std::cout<<"\nNo Macsec NIC Config Present "<< std::endl;
    }
    else {
        std::cout<<"Invlaid number of Mac Config Received "<< std::endl;
    }
    std::cout<<std::endl<<std::endl;
}

bool EthernetMenu::setEthNicIfaceName(std::string &ethIface){
    std::string ifName;
    char delimiter = '\n';
    std::cout<<"   Enter NIC interface name (example eth0,eth1 etc, please be careful of case)  "
             <<"(Max Length - " << MAX_IFACE_NAME_SIZE << " ) : ";

    std::getline(std::cin, ifName, delimiter);

    if(ifName.length() == 0 || ifName.length() >= MAX_IFACE_NAME_SIZE){
        std::cout<<"ERROR: Overflow..Supports only 16 char interface size\n";
        return false;
    } else {
        //Special char check
        if(Utils::validateCharString(ifName)){
            ethIface = ifName;
        } else {
            std::cout<<"\nERROR: special char found in interface name\n";
            return false;
        }
    }
    return true;
}

int EthernetMenu::setEthNicType(telux::data::net::EthNetworkType &type) {
    int ethNicTypeVal;
    std::cin >> ethNicTypeVal;
    Utils::validateInput(ethNicTypeVal,
                        {static_cast<int>(telux::data::net::EthNetworkType::LAN),
                         static_cast<int>(telux::data::net::EthNetworkType::WAN)}
                        );
    type = static_cast<telux::data::net::EthNetworkType>(ethNicTypeVal);
    return ethNicTypeVal;
}

void EthernetMenu::setMacsecConfig(telux::data::net::MacsecConfig        &macsecConfig) {
    //Input MacSec Config
    int noOfNics;
    std::cout<< "\n   How many macsec NICs you want to configure (eg: 1,2  Max:2) :";
    std::cin >> noOfNics;
    Utils::validateInput(noOfNics,{1,2});
    int macsecConfigLen = noOfNics;
    macsecConfig.configList.resize(macsecConfigLen);
    // Input MacSec Config
    if (macsecConfigLen > 0 && macsecConfigLen <= MAX_ETH_NICS) {
        for (uint32_t i = 0; i < macsecConfigLen; i++) {
            std::cout<<"\n----- NICNo.: "<< i <<" ----- Macsec config ------\n";
            std::cout<< "\n     On Which NIC interface you want to configure macsec "
                        "(eg: eth0 (NIC-1), eth1 (NIC-2) ) \n";
            setEthNicIfaceName(macsecConfig.configList[i].ethNicIfName);
            setMacsecOpr(macsecConfig.configList[i].opr);
            //In case of enable/restart, input macsecMode
            if(static_cast<int>(macsecConfig.configList[i].opr)) {
                setMacsecMode(macsecConfig.configList[i].mode);
            }
        }
    }
    else if (macsecConfigLen > MAX_ETH_NICS) {
        std::cout << "Entered number of Ethernet Interfaces is not in range 1 -"
                  << MAX_ETH_NICS <<"\n";
        return;
    }
}

void EthernetMenu::setMacsecOpr(telux::data::net::MacsecOp &macsecOp) {
    int macsecOpVal;
    std::cout << "   Please input MACsec config action (1-Enable,2-Restart): ";
    std::cin >> macsecOpVal;
    Utils::validateInput(macsecOpVal, {static_cast<int>(telux::data::net::MacsecOp ::ENABLE),
    static_cast<int>(telux::data::net::MacsecOp ::RESTART)});
    macsecOp = static_cast<telux::data::net::MacsecOp >(macsecOpVal);
}

void EthernetMenu::setMacsecMode(telux::data::net::MacsecMode       &mode) {
    int macsecMode;
    std::cout << "   Enter MacSec Mode (1-SUPPLICANT, 2-AUTHENTICATOR) : ";
    std::cin >> macsecMode;
    Utils::validateInput(macsecMode,\
                        {static_cast<int>(telux::data::net::MacsecMode::SUPPLICANT),
                         static_cast<int>(telux::data::net::MacsecMode::AUTHENTICATOR)});
    mode = static_cast<telux::data::net::MacsecMode>(macsecMode);
}

