/*
 *  Copyright (c) 2020-2021 The Linux Foundation. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of The Linux Foundation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * RemoteSimProfileMenu provides menu options to invoke remote sim profile MultiSim functions
 * such as addProfile, deleteProfile, requestProfileList, updateNickName, getEid,
 * provideUserConsent and provideConfirmationCode.
 */

#include <iostream>

#include <telux/common/DeviceConfig.hpp>
#include <telux/tel/PhoneFactory.hpp>
#include "../../common/utils/Utils.hpp"
#include "RspMenu.hpp"
#include "MyRspHandler.hpp"

#define MIN_SIM_SLOT_COUNT 1
#define MAX_SIM_SLOT_COUNT 2
#define DEFAULT_PROFILE_ID 1
#define PRINT_CB std::cout << "\033[1;35mCALLBACK: \033[0m"
#define INVALID -1

RemoteSimProfileMenu::RemoteSimProfileMenu(std::string appName, std::string cursor)
    : ConsoleApp(appName, cursor) {
}

RemoteSimProfileMenu::~RemoteSimProfileMenu() {
    if (simProfileManager_ && rspListener_) {
        simProfileManager_->deregisterListener(rspListener_);
    }

    rspListener_ = nullptr;
    simProfileManager_ = nullptr;

}

void RemoteSimProfileMenu::init() {

    //  Get the PhoneFactory and SimProfileManager instances.
    auto &phoneFactory = telux::tel::PhoneFactory::getInstance();
    simProfileManager_ = phoneFactory.getSimProfileManager();

    //  Check if subsystem is ready
    if (simProfileManager_) {
        //  Check if SimProfile subsystem is ready
        bool subSystemStatus = simProfileManager_->isSubsystemReady();

        //  If subsystem is not ready, wait for it to be ready
        if(!subSystemStatus) {
            std::cout << "\n\nSimProfile subsystem is not ready, Please wait" << std::endl;
            std::future<bool> f = simProfileManager_->onSubsystemReady();
            // If we want to wait unconditionally for SimProfile subsystem to be ready
            subSystemStatus = f.get();
        }

        //  Exit the application, if SDK is unable to initialize SimProfile subsystem
        if(subSystemStatus) {
            rspListener_ = std::make_shared<RspListener>();
            telux::common::Status status = simProfileManager_->registerListener(rspListener_);
            if(status != telux::common::Status::SUCCESS) {
                std::cout << "ERROR - Failed to register listener" << std::endl;
                exit(0);
            }
        } else {
            std::cout << "ERROR - Unable to initialize subsystem" << std::endl;
            exit(0);
        }
    } else {
        std::cout << "ERROR - SimProfileManger is null" << std::endl;
        exit(0);
    }

    std::shared_ptr<ConsoleAppCommand> getEIDCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("1", "Get_EID", {},
        std::bind(&RemoteSimProfileMenu::requestEid, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> addProfileCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("2", "Add_Profile", {},
        std::bind(&RemoteSimProfileMenu::addProfile, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> deleteProfileCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("3", "Delete_Profile", {},
        std::bind(&RemoteSimProfileMenu::deleteProfile, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> requestProfileListCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("4", "Request_Profile_List", {},
        std::bind(&RemoteSimProfileMenu::requestProfileList, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> setProfileCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("5", "Enable/Disable_Profile", {},
        std::bind(&RemoteSimProfileMenu::setProfile, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> updateNickNameCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("6", "Update_Nickname", {},
        std::bind(&RemoteSimProfileMenu::updateNickName, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> setUserConsent
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("7", "Provide_User_Consent", {},
        std::bind(&RemoteSimProfileMenu::provideUserConsent,
        this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> getServerAddress
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("8", "Get_Server_Address", {},
        std::bind(&RemoteSimProfileMenu::requestServerAddress,
        this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> setServerAddress
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("9", "Set_Server_Address", {},
        std::bind(&RemoteSimProfileMenu::setServerAddress,
        this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> provideConfirmationCode
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("10", "Provide_Confirmation_code",
        {}, std::bind(&RemoteSimProfileMenu::provideConfirmationCode,
        this, std::placeholders::_1)));

    std::vector<std::shared_ptr<ConsoleAppCommand>> commandsListRemoteSimProfileMenu
        = { getEIDCommand, addProfileCommand, deleteProfileCommand, requestProfileListCommand,
            setProfileCommand, updateNickNameCommand, setUserConsent, getServerAddress,
            setServerAddress, provideConfirmationCode};

    addCommands(commandsListRemoteSimProfileMenu);
    ConsoleApp::displayMenu();
}

SlotId RemoteSimProfileMenu::getSlotIdInput() {

    std::string slotSelection;
    char delimiter = '\n';
    SlotId slotId = SlotId::DEFAULT_SLOT_ID;

    std::cout << "Enter the desired SIM slot: ";
    std::getline(std::cin, slotSelection, delimiter);

    if (!slotSelection.empty()) {
       try {
           slotId = static_cast<SlotId>(std::stoi(slotSelection));
           if (slotId < MIN_SIM_SLOT_COUNT || slotId > MAX_SIM_SLOT_COUNT ) {
              std::cout << "ERROR: Invalid slot entered" << std::endl;
              return SlotId::INVALID_SLOT_ID;
           }
       } catch (const std::exception &e) {
           std::cout << "ERROR::Invalid input, please enter a numerical value. INPUT: "
              << slotSelection << std::endl;
           return SlotId::INVALID_SLOT_ID;
       }
    } else {
       std::cout << "Empty input, enter the correct slot" << std::endl;
       return SlotId::INVALID_SLOT_ID;
    }
    return slotId;
}

void RemoteSimProfileMenu::requestEid(std::vector<std::string> userInput) {
    if(simProfileManager_) {
        SlotId slotId = SlotId::DEFAULT_SLOT_ID;
        if (telux::common::DeviceConfig::isMultiSimSupported()) {
           slotId =  getSlotIdInput();
           if (slotId == SlotId::INVALID_SLOT_ID)
              return;
        }

        telux::common::Status status =
            simProfileManager_->requestEid(slotId, MyRspCallback::onEidResponse);
        if (status == telux::common::Status::SUCCESS) {
            std::cout << "Request Eid sent successfully" << std::endl;
        } else {
            std::cout << "Request Eid failed, status:" << static_cast<int>(status) << std::endl;
        }
    } else {
        std::cout << "ERROR - SimProfileManger is null" << std::endl;
    }
}

void RemoteSimProfileMenu::addProfile(std::vector<std::string> userInput) {
    if(simProfileManager_) {
        SlotId slotId = SlotId::DEFAULT_SLOT_ID;
        if (telux::common::DeviceConfig::isMultiSimSupported()) {
           slotId =  getSlotIdInput();
           if (slotId == SlotId::INVALID_SLOT_ID)
              return;
        }

        char delimiter = '\n';
        std::string actCode = "";
        std::string confCode = "";
        std::string userConsentSelection = "";
        bool isUserConsentReq = false;

        std::cout << "Enter activation code: ";
        std::getline(std::cin, actCode, delimiter);
        if (actCode.empty()) {
           std::cout << "Activation code is empty" << std::endl;
           return;
        }
        std::cout << "Enter confirmation code: ";
        std::getline(std::cin, confCode, delimiter);
        std::cout << "User consent for profile download and install(1 - Yes/0 - No): ";
        std::getline(std::cin, userConsentSelection, delimiter);
        if (!userConsentSelection.empty()) {
           try {
              isUserConsentReq = std::stoi(userConsentSelection);
              if (isUserConsentReq != 0 && isUserConsentReq != 1 ) {
                  std::cout << "ERROR::Invalid input" <<std::endl;
                  return;
              }
           } catch(const std::exception &e) {
              std::cout << "ERROR::Invalid input, please enter a numerical value" <<std::endl;
              return;
           }
        }

        Status status = simProfileManager_->addProfile(slotId, actCode, confCode,
            isUserConsentReq, MyRspCallback::onResponseCallback);
        if (status == Status::SUCCESS) {
            std::cout << "Add profile request sent successfully" << std::endl;
        } else {
            std::cout << "ERROR - Failed to send add profile request, Status:"
                      << static_cast<int>(status) << std::endl;
            Utils::printStatus(status);
        }
    } else {
        std::cout << "ERROR - SimProfileManger is null" << std::endl;
    }
}

void RemoteSimProfileMenu::deleteProfile(std::vector<std::string> userInput) {
    if(simProfileManager_) {
        SlotId slotId = SlotId::DEFAULT_SLOT_ID;
        if (telux::common::DeviceConfig::isMultiSimSupported()) {
           slotId =  getSlotIdInput();
           if (slotId == SlotId::INVALID_SLOT_ID)
              return;
        }

        char delimiter = '\n';
        std::string profileIdSelection = "";
        int profileId = DEFAULT_PROFILE_ID;

        std::cout << "Enter the desired Profile Id: ";
        std::getline(std::cin, profileIdSelection, delimiter);
        if (profileIdSelection.empty()) {
            std::cout << "ProfileId is empty" << std::endl;
            return;
        }
        try {
            profileId = std::stoi(profileIdSelection);
        } catch(const std::exception &e) {
            std::cout << "ERROR::Invalid input, please enter a numerical value" <<std::endl;
            return;
        }
        Status status = simProfileManager_->deleteProfile(slotId, profileId,
                                                          MyRspCallback::onResponseCallback);
        if (status == Status::SUCCESS) {
            std::cout << "Delete profile request sent successfully" << std::endl;
        } else {
            std::cout << "ERROR - Failed to send delete profile request, Status:"
                      << static_cast<int>(status) << std::endl;
            Utils::printStatus(status);
        }
    } else {
        std::cout << "ERROR - SimProfileManger is null" << std::endl;
    }
}

void RemoteSimProfileMenu::requestProfileList(std::vector<std::string> userInput) {
    if(simProfileManager_) {
        SlotId slotId = SlotId::DEFAULT_SLOT_ID;
        if (telux::common::DeviceConfig::isMultiSimSupported()) {
           slotId =  getSlotIdInput();
           if (slotId == SlotId::INVALID_SLOT_ID)
              return;
        }

        telux::common::Status status = simProfileManager_->requestProfileList(slotId,
            MyRspCallback::onProfileListResponse);
        if (status == telux::common::Status::SUCCESS) {
            std::cout << "Request profile list sent successfully" << std::endl;
        } else {
            std::cout << "Request profile list failed, status:" << int(status) << std::endl;
            Utils::printStatus(status);
        }
    } else {
        std::cout << "ERROR - SimProfileManger is null" << std::endl;
    }
}

void RemoteSimProfileMenu::setProfile(std::vector<std::string> userInput) {
    if(simProfileManager_) {
        SlotId slotId = SlotId::DEFAULT_SLOT_ID;
        if (telux::common::DeviceConfig::isMultiSimSupported()) {
           slotId =  getSlotIdInput();
           if (slotId == SlotId::INVALID_SLOT_ID)
              return;
        }

        char delimiter = '\n';
        std::string profileIdSelection = "";
        std::string enableDisableSelection = "";
        int profileId = DEFAULT_PROFILE_ID;
        bool enable = false;

        std::cout << "Enter the desired Profile Id: ";
        std::getline(std::cin, profileIdSelection, delimiter);
        if (profileIdSelection.empty()) {
            std::cout << "ProfileId is empty" << std::endl;
            return;
        }
        try {
            profileId = std::stoi(profileIdSelection);
        } catch(const std::exception &e) {
            std::cout << "ERROR::Invalid input, please enter a numerical value" <<std::endl;
            return;
        }

        std::cout << "Enable/Disable Profile(1 - Enable/0 - Disable): ";
        std::getline(std::cin, enableDisableSelection, delimiter);
        if (enableDisableSelection.empty()) {
            std::cout << "Enable/Disable Selection is empty" << std::endl;
            return;
        }
        try {
            enable = std::stoi(enableDisableSelection);
            if (enable != 0 && enable != 1 ) {
                std::cout << "ERROR::Invalid input" <<std::endl;
                return;
            }
        } catch(const std::exception &e) {
            std::cout << "ERROR::Invalid input, please enter a numerical value" <<std::endl;
            return;
        }

        Status status = simProfileManager_->setProfile(slotId, profileId, enable,
                        MyRspCallback::onResponseCallback);
        if (status == Status::SUCCESS) {
            std::cout << "Enable/Disable profile request sent successfully" << std::endl;
        } else {
            std::cout << "ERROR - Failed to send setProfile request, Status:"
                      << static_cast<int>(status) << std::endl;
            Utils::printStatus(status);
        }
    } else {
        std::cout << "ERROR - SimProfileManger is null" << std::endl;
    }
}

void RemoteSimProfileMenu::updateNickName(std::vector<std::string> userInput) {
    if(simProfileManager_) {
        SlotId slotId = SlotId::DEFAULT_SLOT_ID;
        if (telux::common::DeviceConfig::isMultiSimSupported()) {
           slotId =  getSlotIdInput();
           if (slotId == SlotId::INVALID_SLOT_ID)
              return;
        }

        std::string profileIdSelection = "";
        std::string nickname = "";
        char delimiter = '\n';
        int profileId = DEFAULT_PROFILE_ID;

        std::cout << "Enter the desired Profile Id: ";
        std::getline(std::cin, profileIdSelection, delimiter);
        if (profileIdSelection.empty()) {
            std::cout << "ProfileId is empty" << std::endl;
            return;
        }
        try {
            profileId = std::stoi(profileIdSelection);
        } catch(const std::exception &e) {
            std::cout << "ERROR::Invalid input, please enter a numerical value" <<std::endl;
            return;
        }

        std::cout << "Enter the Nickname: ";
        std::getline(std::cin, nickname, delimiter);
        if (nickname.empty()){
            std::cout << "Nickname empty" << std::endl;
            return;
        }

        Status status = simProfileManager_->updateNickName(slotId, profileId, nickname,
                                                           MyRspCallback::onResponseCallback);
        if (status == Status::SUCCESS) {
            std::cout << "updateNickName request sent successfully" << std::endl;
        } else {
            std::cout << "ERROR - Failed to send updateNickName request, Status:"
                      << static_cast<int>(status) << std::endl;
            Utils::printStatus(status);
        }
    } else {
        std::cout << "ERROR - SimProfileManger is null" << std::endl;
    }
}

void RemoteSimProfileMenu::provideUserConsent(std::vector<std::string> userInput) {
    if(simProfileManager_) {
        SlotId slotId = SlotId::DEFAULT_SLOT_ID;
        if (telux::common::DeviceConfig::isMultiSimSupported()) {
           slotId =  getSlotIdInput();
           if (slotId == SlotId::INVALID_SLOT_ID)
              return;
        }
        char delimiter = '\n';
        std::string userConsentSelection = "";
        bool isUserConsent = false;

        std::cout << "User consent for profile download and install (1 - Yes/0 - No): ";
        std::getline(std::cin, userConsentSelection, delimiter);
        if (!userConsentSelection.empty()) {
            try {
                isUserConsent = std::stoi(userConsentSelection);
                if (isUserConsent != 0 && isUserConsent != 1 ) {
                    std::cout << "ERROR::Invalid input" <<std::endl;
                    return;
                }
            } catch(const std::exception &e) {
                std::cout << "ERROR::Invalid input, please enter a numerical value" <<std::endl;
                return;
            }
        }

        std::string userConsentReason = "";
        int reason = INVALID;
        if (!isUserConsent) {
            std::cout << "Reason for user consent not OK (0 - REJECT/1 - POSTPONE): ";
            std::getline(std::cin, userConsentReason, delimiter);
            if (!userConsentReason.empty()) {
                try {
                    reason = std::stoi(userConsentReason);
                    if (reason != 0 && reason != 1 ) {
                        std::cout << "ERROR::Invalid input" <<std::endl;
                        return;
                    }
                } catch(const std::exception &e) {
                    std::cout << "ERROR::Invalid input, please enter a numerical value" <<std::endl;
                    return;
                }
            }
        }

        Status status = simProfileManager_->provideUserConsent(slotId, isUserConsent,
            static_cast<telux::tel::UserConsentReasonType>(reason),
            MyRspCallback::onResponseCallback);
        if (status == Status::SUCCESS) {
            std::cout << "Provide user consent request sent successfully"
                      << std::endl;
        } else {
            std::cout << "ERROR - Failed to send provide user consent request,"
                      << "Status:" << static_cast<int>(status) << std::endl;
            Utils::printStatus(status);
        }
    } else {
        std::cout << "ERROR - SimProfileManger is null" << std::endl;
    }
}

void RemoteSimProfileMenu::provideConfirmationCode(std::vector<std::string> userInput) {
    if(simProfileManager_) {
        SlotId slotId = SlotId::DEFAULT_SLOT_ID;
        if (telux::common::DeviceConfig::isMultiSimSupported()) {
           slotId =  getSlotIdInput();
            if (slotId == SlotId::INVALID_SLOT_ID)
              return;
        }
        char delimiter = '\n';
        std::string confirmationCode = "";

        std::cout << "Enter confirmation code for profile download and install: ";
        std::getline(std::cin, confirmationCode, delimiter);
        if (confirmationCode.empty()) {
           std::cout << "Confirmation code is empty" << std::endl;
           return;
        }

        Status status = simProfileManager_->provideConfirmationCode(slotId, confirmationCode,
                                                               MyRspCallback::onResponseCallback);
        if (status == Status::SUCCESS) {
            std::cout << "Provide confirmation code request sent successfully"
                      << std::endl;
        } else {
            std::cout << "ERROR - Failed to send provide confirmation code request,"
                      << "Status:" << static_cast<int>(status) << std::endl;
            Utils::printStatus(status);
        }
    } else {
        std::cout << "ERROR - SimProfileManger is null" << std::endl;
    }
}

void RemoteSimProfileMenu::setServerAddress(std::vector<std::string> userInput) {
    if(simProfileManager_) {
        SlotId slotId = SlotId::DEFAULT_SLOT_ID;
        if (telux::common::DeviceConfig::isMultiSimSupported()) {
           slotId =  getSlotIdInput();
           if (slotId == SlotId::INVALID_SLOT_ID)
              return;
        }

        std::string smdpAddress = "";
        char delimiter = '\n';
        int profileId = DEFAULT_PROFILE_ID;

        std::cout << "Enter the SMDP Address: ";
        std::getline(std::cin, smdpAddress, delimiter);

        Status status = simProfileManager_->setServerAddress(slotId, smdpAddress,
            MyRspCallback::onResponseCallback);
        if (status == Status::SUCCESS) {
            std::cout << "setServerAddress request sent successfully" << std::endl;
        } else {
            std::cout << "ERROR - Failed to send setServerAddress request, Status:"
                      << static_cast<int>(status) << std::endl;
            Utils::printStatus(status);
        }
    } else {
        std::cout << "ERROR - SimProfileManger is null" << std::endl;
    }
}

void RemoteSimProfileMenu::requestServerAddress(std::vector<std::string> userInput) {
    if(simProfileManager_) {
        SlotId slotId = SlotId::DEFAULT_SLOT_ID;
        if (telux::common::DeviceConfig::isMultiSimSupported()) {
           slotId =  getSlotIdInput();
           if (slotId == SlotId::INVALID_SLOT_ID)
              return;
        }

        telux::common::Status status =
            simProfileManager_->requestServerAddress(slotId,
                MyRspCallback::onServerAddressResponse);
        if (status == telux::common::Status::SUCCESS) {
            std::cout << "Request Server Address sent successfully" << std::endl;
        } else {
            std::cout << "Request Server Address failed, status:" << static_cast<int>(status)
                << std::endl;
        }
    } else {
        std::cout << "ERROR - SimProfileManger is null" << std::endl;
    }
}