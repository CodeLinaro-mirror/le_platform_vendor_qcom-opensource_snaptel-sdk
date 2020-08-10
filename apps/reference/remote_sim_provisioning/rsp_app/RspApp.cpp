/*
 *  Copyright (c) 2020 The Linux Foundation. All rights reserved.
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

extern "C" {
#include <getopt.h>
}

#include <iostream>

#include "RspApp.hpp"
#include "Utils.hpp"
#include <telux/common/DeviceConfig.hpp>

#define MIN_SIM_SLOT_COUNT 1
#define MAX_SIM_SLOT_COUNT 2
#define PRINT_CB std::cout << "\033[1;35mCALLBACK: \033[0m"

RemoteSimProfile::RemoteSimProfile()
    :slotId_(SlotId::DEFAULT_SLOT_ID)
    ,profileId_(1)
    ,enableProfile_(0)
    ,userConsent_(0)
    ,activationCode_("")
    ,confirmationCode_("")
    ,nickname_("") {
}

RemoteSimProfile::~RemoteSimProfile() {
    if (simProfileManager_ && rspListener_) {
        simProfileManager_->deregisterListener(rspListener_);
    }

    rspListener_ = nullptr;
    simProfileManager_ = nullptr;
}

RemoteSimProfile &RemoteSimProfile::getInstance() {
    static RemoteSimProfile instance;
    return instance;
}

void RemoteSimProfile::init() {

    //  Get the SimProfileFactory instances.
    auto &simProfileFactory = telux::rsp::SimProfileFactory::getInstance();
    simProfileManager_ = simProfileFactory.getSimProfileManager();

    //  Check if subsystem is ready
    if (simProfileManager_) {
        //  Check if SimProfile subsystem is ready
        bool subSystemStatus = simProfileManager_->isSubsystemReady();

        //  If subsystem is not ready, wait for it to be ready
        if(!subSystemStatus) {
            std::cout << "\nSimProfile subsystem is not ready, Please wait" << std::endl;
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
}

void RemoteSimProfile::printUsage(char **argv) {
    std::cout << std::endl;
    std::cout << "Usage: " << argv[0] << " [options] \n";
    std::cout << "Options: \n";
    std::cout << "\t -h --help                              Print all the options\n";

    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        std::cout << "\t -s --slot-id <SLOT_ID>             Use the slot id\n";
    }

    std::cout << "\t -i --eid                               Request for eUICC Identifier\n";
    std::cout << "\t -a --add <ACTIVATION_CODE> <CONFIRMATION_CODE> <USER_CONSENT_REQUIRED>\n";
    std::cout << "\t                                        Add profile with activation code, \n";
    std::cout << "\t                                        confirmation code and ";
    std::cout << "user consent required (1 - YES, 0 - No)\n";
    std::cout << "\t -d --delete <PROFILE_ID>               Delete profile with profile id\n";
    std::cout << "\t -p --profile-list                      Request for the profile list\n";
    std::cout << "\t -e --enable <PROFILE_ID> <ENABLE>      Enable/Disable profile for the "
                 "given profile id\n";
    std::cout << "\t                                        1 - enable, 0 - disable\n";
    std::cout << "\t -u --nickname <PROFILE_ID> <NICKNAME>  Update nickname for the given "
                 "profile id\n";
    std::cout << "\t -c --user-consent-required <USER_OK>   User consent for profile ";
    std::cout << "download/install\n";
    std::cout << "\t                                        1 - YES, 0 - No\n";
    std::cout << "Example: \n";

    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        std::cout << "   rsp_app --slot-id 1 --add LPA:$XXX.xxx "" 0 \n";
    } else {
        std::cout << "   rsp_app --add LPA:$XXX.xxx "" 0 \n";
    }

    std::cout << std::endl;
}

Status RemoteSimProfile::parseArguments(int argc, char **argv) {
    int c;
    if (argc <= 1) {
       printUsage(argv);
       return telux::common::Status::FAILED;
    }
    while (1) {
        int optionIndex = 0;
        if (telux::common::DeviceConfig::isMultiSimSupported()) {
            static struct option longOptions[] = {{"slot-id", required_argument, 0, 's'},
                {"add", required_argument, 0, 'a'}, {"delete", required_argument, 0, 'd'},
                {"enable", required_argument, 0, 'e'}, {"nickname", required_argument, 0, 'u'},
                {"user-consent-required", required_argument, 0, 'c'},{"eid", no_argument, 0, 'i'},
                {"profile-list", no_argument, 0, 'p'}, {"help", no_argument, 0, 'h'},
                {0, 0, 0, 0}};

            c = getopt_long(argc, argv, "s:a:d:e:u:c:iph", longOptions, &optionIndex);
        } else {
            static struct option longOptions[] = {{"add", required_argument, 0, 'a'},
               {"delete", required_argument, 0, 'd'},{"enable", required_argument, 0, 'e'},
               {"nickname", required_argument, 0, 'u'},
               {"user-consent-required", required_argument, 0, 'c'}, {"eid", no_argument, 0, 'i'},
               {"profile-list", no_argument, 0, 'p'}, {"help", no_argument, 0, 'h'}, {0, 0, 0, 0}};

            c = getopt_long(argc, argv, "a:d:e:u:c:iph", longOptions, &optionIndex);
        }

        /* Detect the end of the options. */
        if (c == -1) {
            break;
        }
        switch (c) {
        case 's':
            try {
                slotId_ = static_cast<SlotId>(std::stoi(optarg));
            } catch(const std::exception &e) {
                return telux::common::Status::INVALIDPARAM;
            }
            std::cout << "Selected slot ID : " << std::string(optarg) << std::endl;
            if ((slotId_ < MIN_SIM_SLOT_COUNT) || (slotId_ > MAX_SIM_SLOT_COUNT)) {
               std::cout << "ERROR: Invalid slot Id provided" << std::endl;
               return telux::common::Status::INVALIDPARAM;
            }
            break;
        case 'a':
            activationCode_ = std::string(optarg);
            confirmationCode_ = std::string(argv[optind]);
            try {
                userConsent_ =  std::stoi(argv[++optind]);
            } catch(const std::exception &e) {
                return telux::common::Status::INVALIDPARAM;
            }
            std::cout << "Adding profile with activation code: "<< activationCode_ <<std::endl;
            std::cout << "User consent to add profile, isEnable: "<< userConsent_ <<std::endl;
            if (not confirmationCode_.empty()) {
                std::cout << "Adding profile with confirmation code: "<< confirmationCode_
                          << std::endl;
            }
            addProfile(activationCode_, confirmationCode_, userConsent_);
            break;
        case 'd':
            try {
                profileId_ = std::stoi(optarg);
            } catch(const std::exception &e) {
                return telux::common::Status::INVALIDPARAM;
            }
            deleteProfile(profileId_);
            break;
        case 'e':
            try {
                profileId_ = std::stoi(optarg);
                enableProfile_ = std::stoi(argv[optind]);
            } catch(const std::exception &e) {
                return telux::common::Status::INVALIDPARAM;
            }
            setProfile(profileId_, enableProfile_);
            break;
        case 'p':
            requestProfileList();
            break;
        case 'u':
            try {
                profileId_ = std::stoi(optarg);
                nickname_ = std::string(argv[optind]);
            } catch(const std::exception &e) {
                return telux::common::Status::INVALIDPARAM;
            }
            updateNickName(profileId_, nickname_);
            break;
        case 'i':
            requestEid();
            break;
        case 'c':
            try {
                userConsent_ = std::stoi(optarg);
            } catch(const std::exception &e) {
                return telux::common::Status::INVALIDPARAM;
            }
            provideUserConsent(userConsent_);
            break;
        case 'h':
            printUsage(argv);
            break;
        default:
            printUsage(argv);
            return telux::common::Status::INVALIDPARAM;
        }
    }
    return telux::common::Status::SUCCESS;
}

void RemoteSimProfile::requestProfileList() {
    auto respCb = [&](const std::vector<std::shared_ptr<telux::rsp::SimProfile>> &profiles,
        telux::common::ErrorCode errorCode) { onProfileListResponse(profiles, errorCode); };

    if(simProfileManager_) {
        telux::common::Status status = simProfileManager_->requestProfileList(slotId_, respCb);
        if (status == telux::common::Status::SUCCESS) {
            std::cout << "Request profile list sent successfully" << std::endl;
        } else {
            std::cout << "Request profile list failed, status:" << static_cast<int>(status)
                      << std::endl;
        }
    } else {
        std::cout << "ERROR - SimProfileManger is null" << std::endl;
    }
}

void RemoteSimProfile::addProfile(const std::string &actCode, const std::string &confCode,
    bool isUserConsentRequired) {

    auto respCb = [&](telux::common::ErrorCode errorCode) { onResponseCallback(errorCode); };
    if(simProfileManager_) {
        Status status = simProfileManager_->addProfile(slotId_, actCode, confCode,
            isUserConsentRequired, respCb);
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

void RemoteSimProfile::deleteProfile(int profileId) {
    auto respCb = [&](telux::common::ErrorCode errorCode) { onResponseCallback(errorCode); };

    if(simProfileManager_) {
        Status status = simProfileManager_->deleteProfile(slotId_, profileId, respCb);
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

void RemoteSimProfile::setProfile(int profileId, bool enableProfile) {
    auto respCb = [&](telux::common::ErrorCode errorCode) { onResponseCallback(errorCode); };
    if(simProfileManager_) {
        Status status = simProfileManager_->setProfile(slotId_, profileId, enableProfile, respCb);
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

void RemoteSimProfile::updateNickName(int profileId, const std::string &nickname) {
    auto respCb = [&](telux::common::ErrorCode errorCode) { onResponseCallback(errorCode); };

    if(simProfileManager_) {
        Status status = simProfileManager_->updateNickName(slotId_, profileId, nickname, respCb);
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

void RemoteSimProfile::onProfileListResponse(
    const std::vector<std::shared_ptr<telux::rsp::SimProfile>> &profiles,
    telux::common::ErrorCode errorCode) {
    if (errorCode == telux::common::ErrorCode::SUCCESS) {
        PRINT_CB << "Profile List: \n";
        for (size_t index = 0; index < profiles.size(); index++) {
            std::cout << profiles[index]->toString() << std::endl;
        }
    } else {
        PRINT_CB << "\n requestProfileList failed, ErrorCode: " <<static_cast<int>(errorCode)
                 << " Description : " << Utils::getErrorCodeAsString(errorCode)<< std::endl;
    }
}

void RemoteSimProfile::requestEid() {
    auto respCb = [&](std::string eid, telux::common::ErrorCode errorCode)
       { onEidResponse(eid, errorCode); };

    if(simProfileManager_) {
        telux::common::Status status = simProfileManager_->requestEid(slotId_, respCb);
        if (status == telux::common::Status::SUCCESS) {
            std::cout << "Request EID sent successfully" << std::endl;
        } else {
            std::cout << "Request EID failed, status:" << static_cast<int>(status) << std::endl;
            Utils::printStatus(status);
        }
    } else {
        std::cout << "ERROR - SimProfileManger is null" << std::endl;
    }
}

void RemoteSimProfile::onEidResponse(
    std::string eid, telux::common::ErrorCode errorCode) {
    if (errorCode == telux::common::ErrorCode::SUCCESS) {
        PRINT_CB << "EID : " << eid <<std::endl;;
    } else {
        PRINT_CB << "Request EID failed, ErrorCode: " <<static_cast<int>(errorCode)
                 << " Description : " << Utils::getErrorCodeAsString(errorCode)<< std::endl;
    }
}

void RemoteSimProfile::provideUserConsent(bool isUserConsentRequired) {
    auto respCb = [&](telux::common::ErrorCode errorCode) { onResponseCallback(errorCode); };
    if(simProfileManager_) {
        Status status = simProfileManager_->provideUserConsent(slotId_, isUserConsentRequired,
            respCb);
        if (status == Status::SUCCESS) {
            std::cout << "provideUserConsent request sent successfully"
                      << std::endl;
        } else {
            std::cout << "ERROR - Failed to send provideUserConsent request,"
                      << "Status:" << static_cast<int>(status) << std::endl;
            Utils::printStatus(status);
        }
    } else {
        std::cout << "ERROR - SimProfileManger is null" << std::endl;
    }
}

void RemoteSimProfile::onResponseCallback(telux::common::ErrorCode error) {
    std::cout << std::endl;
    if (error != telux::common::ErrorCode::SUCCESS) {
        PRINT_CB << "Request failed with errorCode: " << static_cast<int>(error)
                 << " Description : " << Utils::getErrorCodeAsString(error)<< std::endl;
    } else {
        PRINT_CB << "Received success response for sent request \n";
    }
}

int main(int argc, char *argv[]) {
    std::cout << "\nRemote SIM Provisioning Application";
    auto &remoteSimProfile = RemoteSimProfile::getInstance();
    remoteSimProfile.init();
    auto status = remoteSimProfile.parseArguments(argc, argv);
    if (status != Status::SUCCESS) {
        std::cout << "ERROR::Invalid arguments \n";
        return EXIT_FAILURE;
    }

    std::cout << "\nPress ENTER to exit. \n";
    std::cin.ignore();

    return EXIT_SUCCESS;
}
