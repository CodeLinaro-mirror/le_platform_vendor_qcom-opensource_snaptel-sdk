/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <algorithm>
#include <future>
#include <iostream>
#include <string>

#include "../../common/utils/Utils.hpp"
#include <telux/tel/PhoneFactory.hpp>
#include "ApSimProfileClient.hpp"

#include "MyApSimProfileHandler.hpp"

#define FILE_PATH "/etc"
#define FILE_NAME "tel.conf"

#define PRINT_CB std::cout << "\033[1;35mCALLBACK: \033[0m"
#define PRINT_NOTIFICATION std::cout << "\033[1;35mNOTIFICATION: \033[0m"

// Parameter and response used by the command to get extra responses of an APDU command.
const std::string ISD_R_AID = "A0000005591010FFFFFFFF8900000100";
static const int INS_GET_MORE_RESPONSE = 192; // 0xC0
static const int SW1_MORE_RESPONSE = 97; // 0x61
static const int SW1_NO_ERROR = 144; // 0x90
static const int SW2_NO_ERROR = 0;
/* From APDU response, 3rd byte from last is the result for GET_RESPONSE command
   last byte = SW2
   last byte - 1 = SW1
   last byte - 2 = Result of the APDU command(Enable/Disable/Get profiles result)
*/
static const int GET_MORE_RESPONSE_RESULT_BYTE = 2;
// Error codes are defined in GSMA SGP.22. 0 is the code for success.
static const int CODE_OK = 0;

struct ApduExchangeResult {
    int sw1;
    int sw2;
    std::string payload;
    std::vector<int> data;
};

class ResponseData {
 public:
    ApduExchangeResult getResult() {
        return apduResult;
    }

    int getChannel() {
        return cardchannel;
    }

    std::future<telux::common::ErrorCode> getFuture() {
        return cbPromise_.get_future();
    }

 protected:
    ApduExchangeResult apduResult;
    int cardchannel = -1;
    std::promise<telux::common::ErrorCode> cbPromise_;
};

class TransmitApduCallback : public telux::tel::ICardCommandCallback,
                             public ResponseData {
 public:

    void onResponse(telux::tel::IccResult result, telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        if (error == telux::common::ErrorCode::SUCCESS) {
            PRINT_CB << "onResponse successful, " << result.toString() << std::endl
                << std::endl;
            apduResult.sw1     = result.sw1;
            apduResult.sw2     = result.sw2;
            apduResult.payload = result.payload;
            for (int data : result.data) {
                apduResult.data.emplace_back(data);
            }
        } else {
            PRINT_CB << "onResponse failed\n error: " << static_cast<int>(error)
                << ", description: " << Utils::getErrorCodeAsString(error)
                << std::endl;
        }
        cbPromise_.set_value(error);
    }
}; // end of TransmitApduCallback

class OpenLogicalChannelCallback : public telux::tel::ICardChannelCallback,
                                   public ResponseData {
 public:

    void onChannelResponse(
        int channel, telux::tel::IccResult result, telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        if (error == telux::common::ErrorCode::SUCCESS) {
            PRINT_CB << "onChannelResponse successful, channel: " << channel
                << "\n iccResult " << result.toString() << std::endl;
            cardchannel        = channel;
            apduResult.sw1     = result.sw1;
            apduResult.sw2     = result.sw2;
            apduResult.payload = result.payload;
            for (int data : result.data) {
                apduResult.data.emplace_back(data);
            }
        } else {
            PRINT_CB << "onChannelResponse failed\n error: "  << static_cast<int>(error)
                << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
        }
        cbPromise_.set_value(error);
    }

};  // end of OpenLogicalChannelCallback

class CloseLogicalChannelCallback : public telux::common::ICommandResponseCallback,
                                    public ResponseData {
 public:

    void commandResponse(telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        if (error == telux::common::ErrorCode::SUCCESS) {
            PRINT_CB << "onCloseLogicalChannel successful." << std::endl;
        } else {
            PRINT_CB << "onCloseLogicalChannel failed\n error: "
                << static_cast<int>(error)
                << ", description: " << Utils::getErrorCodeAsString(error)
                << std::endl;
        }
        cbPromise_.set_value(error);
    }

};  // end of CloseLogicalChannelCallback

void ApSimProfileClient::CardRefreshResponseCallback::commandResponse(
   telux::common::ErrorCode error) {
   std::cout << std::endl << std::endl;
   if(error == telux::common::ErrorCode::SUCCESS) {
      PRINT_CB << "Refresh command successful." << std::endl;
   } else {
      PRINT_CB << "Refresh command failed\n error: " << static_cast<int>(error)
             << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
   }
}

/**
 *  ICardListener events
 */
ApSimProfileClient::MyApCardListener::MyApCardListener(
    std::weak_ptr<ApSimProfileClient> apSimProfileClient) {
    apSimProfileClient_ = apSimProfileClient;
}

void ApSimProfileClient::MyApCardListener::onCardInfoChanged(int slotId) {
   std::cout << std::endl << std::endl;
   PRINT_NOTIFICATION << "CardInfo changed on SlotId :" << slotId << std::endl;
}

void ApSimProfileClient::MyApCardListener::onRefreshEvent(
    int slotId, telux::tel::RefreshStage stage, telux::tel::RefreshMode mode,
    std::vector<telux::tel::IccFile> efFiles, telux::tel::RefreshParams config) {
    std::cout << std::endl << std::endl;
    PRINT_NOTIFICATION << " onRefreshEvent on slot" << slotId
        << " ,Refresh Stage is " << refreshStageToString(stage)
        << " ,Refresh Mode is " << refreshModeToString(mode)
        << " ,Session Type is " << sessionTypeToString(config.sessionType)
        << ((!config.aid.empty()) ? " ,AID is " : "")
        << ((!config.aid.empty()) ? config.aid : "") << "\n";

    auto sp = apSimProfileClient_.lock();
    if (sp) {
        sp->refreshMode_ = mode;
        sp->refreshSlotId_ = slotId;
    } else {
        std::cout << "ApSimProfileClient is null" << "\n";
    }
}

// Notify subsystem status
void ApSimProfileClient::MyApCardListener::onServiceStatusChange(
    telux::common::ServiceStatus status) {
    std::string stat = "";
    switch(status) {
        case telux::common::ServiceStatus::SERVICE_AVAILABLE:
            stat = " SERVICE_AVAILABLE";
            break;
        case telux::common::ServiceStatus::SERVICE_UNAVAILABLE:
            stat =  " SERVICE_UNAVAILABLE";
            break;
        default:
            stat = " Unknown service status";
            break;
    }
    PRINT_NOTIFICATION << " Card onServiceStatusChange" << stat << "\n";
}

std::string ApSimProfileClient::MyApCardListener::refreshStageToString(
    telux::tel::RefreshStage stage) {
    std::string stageString;
    switch(stage) {
        case telux::tel::RefreshStage::WAITING_FOR_VOTES:
            stageString = "Waiting for votes";
            break;
        case telux::tel::RefreshStage::STARTING:
            stageString = "Starting";
            break;
        case telux::tel::RefreshStage::ENDED_WITH_SUCCESS:
            stageString = "Ended with success";
            break;
        case telux::tel::RefreshStage::ENDED_WITH_FAILURE:
            stageString = "Ended with failure";
            break;
        default:
            stageString = "Unknown";
            break;
    }
    return stageString;
}

std::string ApSimProfileClient::MyApCardListener::refreshModeToString(
    telux::tel::RefreshMode mode) {
    std::string modeString;
    switch(mode) {
        case telux::tel::RefreshMode::RESET:
            modeString = "RESET";
            break;
        case telux::tel::RefreshMode::INIT:
            modeString = "INIT";
            break;
        case telux::tel::RefreshMode::INIT_FCN:
            modeString = "INIT FCN";
            break;
        case telux::tel::RefreshMode::FCN:
            modeString = "FCN";
            break;
        case telux::tel::RefreshMode::INIT_FULL_FCN:
            modeString = "INIT FULL FCN";
            break;
        case telux::tel::RefreshMode::RESET_APP:
            modeString = "Reset Applications";
            break;
        case telux::tel::RefreshMode::RESET_3G:
            modeString = "Reset 3G session";
            break;
        default:
            modeString = "Unknown";
            break;
    }
    return modeString;
}

std::string ApSimProfileClient::MyApCardListener::sessionTypeToString(
    telux::tel::SessionType type) {
    std::string typeString;
    switch(type) {
        case telux::tel::SessionType::PRIMARY:
            typeString = "PRIMARY";
            break;
        case telux::tel::SessionType::SECONDARY:
            typeString = "SECONDARY";
            break;
        case telux::tel::SessionType::NONPROVISIONING_SLOT_1:
            typeString = "NONPROVISIONING SLOT1";
            break;
        case telux::tel::SessionType::NONPROVISIONING_SLOT_2:
            typeString = "NONPROVISIONING SLOT2";
            break;
        case telux::tel::SessionType::CARD_ON_SLOT_1:
            typeString = "CARD ON SLOT1";
            break;
        case telux::tel::SessionType::CARD_ON_SLOT_2:
            typeString = "CARD ON SLOT2";
            break;
        default:
            typeString = "Unknown";
            break;
    }
    return typeString;
}

/**
 *  IApSimProfileListener events
 */
ApSimProfileClient::MyApSimProfileListener::MyApSimProfileListener(
    std::weak_ptr<ApSimProfileClient> apSimProfileClient) {
    apSimProfileClient_ = apSimProfileClient;
}

void ApSimProfileClient::MyApSimProfileListener::onRetrieveProfileListRequest(
    SlotId slotId, uint32_t referenceId) {
    std::cout << std::endl << std::endl;
    PRINT_NOTIFICATION << "onRetrieveProfileListRequest Slot Id: " << static_cast<int>(slotId)
                       << ", and referenceId: " << referenceId << std::endl;
    auto sp = apSimProfileClient_.lock();
    if (sp) {
        sp->indSlotId_ = slotId;
        sp->referenceId_ = referenceId;
    } else {
        std::cout << "ApSimProfileClient is null" << "\n";
    }
}

void ApSimProfileClient::MyApSimProfileListener::onProfileOperationRequest(
    SlotId slotId, uint32_t referenceId, std::string iccid,  bool isEnable) {
    std::cout << std::endl << std::endl;
    PRINT_NOTIFICATION << "onProfileOperationRequest Slot Id: " << static_cast<int>(slotId)
                       << (isEnable ? "\nEnable Profile request" : "\nDisable Profile request")
                       << " with reference Id: " << referenceId
                       << " and profile ICCID is : " << iccid << std::endl;
    auto sp = apSimProfileClient_.lock();
    if (sp) {
        sp->indSlotId_ = slotId;
        sp->referenceId_ = referenceId;
        sp->indIccid_ = iccid;
    } else {
        std::cout << "ApSimProfileClient is null" << "\n";
    }
}

// Notify subsystem status
void ApSimProfileClient::MyApSimProfileListener::onServiceStatusChange(
    telux::common::ServiceStatus status) {
    std::string stat = "";
    switch(status) {
        case telux::common::ServiceStatus::SERVICE_AVAILABLE:
            stat = " SERVICE_AVAILABLE";
            break;
        case telux::common::ServiceStatus::SERVICE_UNAVAILABLE:
            stat =  " SERVICE_UNAVAILABLE";
            break;
        default:
            stat = " Unknown service status";
            break;
    }
    PRINT_NOTIFICATION << " ApSimProfile onServiceStatusChange" << stat << "\n";
}

ApSimProfileClient::ApSimProfileClient() {}

ApSimProfileClient::~ApSimProfileClient() {
    if (apSimProfileManager_ && apSimProfileListener_) {
        apSimProfileManager_->deregisterListener(apSimProfileListener_);
    }

    if (apSimProfileListener_) {
        apSimProfileListener_ = nullptr;
    }
    apSimProfileManager_ = nullptr;
    for (auto index = 0; index < cards_.size(); index++) {
        cards_[index] = nullptr;
    }
    if (cardManager_ && cardListener_) {
        cardManager_->removeListener(cardListener_);
    }

    if (cardListener_) {
        cardListener_ = nullptr;
    }
    cardManager_ = nullptr;
}

// Initialize the telephony subsystem
telux::common::Status ApSimProfileClient::init() {
    telux::common::Status status = telux::common::Status::FAILED;
    // Get the PhoneFactory, ApSimProfileManager and CardManager instances.
    std::promise<telux::common::ServiceStatus> apSimProfileMgrProm;
    std::promise<telux::common::ServiceStatus> cardMgrProm;
    auto &phoneFactory = telux::tel::PhoneFactory::getInstance();
    apSimProfileManager_ = phoneFactory.getApSimProfileManager(
        [&](telux::common::ServiceStatus status) {
           apSimProfileMgrProm.set_value(status);
    });

    if (apSimProfileManager_) {
        // Check if ApSimProfile subsystem is ready
        telux::common::ServiceStatus subSystemStatus = apSimProfileManager_->getServiceStatus();

        // If subsystem is not ready, wait for it to be ready
        if (subSystemStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "ApSimProfile subsystem is not ready, Please wait."
                << std::endl;
        }

        subSystemStatus = apSimProfileMgrProm.get_future().get();

        // return from the function, if SDK is unable to initialize ApSimProfile subsystem
        if (subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "ApSimProfile subsystem is ready \n ";
            apSimProfileListener_ = std::make_shared<MyApSimProfileListener>(shared_from_this());
            status = apSimProfileManager_->registerListener(apSimProfileListener_);
            if (status != telux::common::Status::SUCCESS) {
                std::cout << "ERROR - Failed to register listener" << std::endl;
                return telux::common::Status::FAILED;
            }
        } else {
            std::cout << "ERROR - Unable to initialize ApSimProfileManager subsystem"
                 << std::endl;
            return telux::common::Status::FAILED;
        }
    } else {
        std::cout << "ERROR - ApSimProfileManger is null" << std::endl;
        return telux::common::Status::FAILED;
    }

    cardManager_ =
        phoneFactory.getCardManager([&](telux::common::ServiceStatus status) {
            cardMgrProm.set_value(status);
    });
    if (cardManager_) {
       // Check if card subsystem is ready
       telux::common::ServiceStatus cardSubSystemStatus =  cardManager_->getServiceStatus();

       // If card subsystem is not ready, wait for it to be ready
       if (cardSubSystemStatus !=
           telux::common::ServiceStatus::SERVICE_AVAILABLE) {
           std::cout << "Card subsystem is not ready, Please wait" << std::endl;
       }
       cardSubSystemStatus = cardMgrProm.get_future().get();

       if (cardSubSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
           std::cout << "Card subsystem is ready \n";
           // registering listener for card events
           cardListener_ = std::make_shared<MyApCardListener>(shared_from_this());
           status = cardManager_->registerListener(cardListener_);
           if (status != telux::common::Status::SUCCESS) {
               std::cout << "Unable to registerListener" << " \n ";
           }
           std::vector<int> slotIds;
           telux::common::Status status = cardManager_->getSlotIds(slotIds);
           if (status == telux::common::Status::SUCCESS) {
               for (auto index = 1; index <= slotIds.size(); index++) {
                   // register for SIM REFRESH event
                   status = registerRefresh(static_cast<SlotId>(index));
                   if (status != telux::common::Status::SUCCESS) {
                       std::cout << "Unable to register for SIM REFRESH on slot " << index << "\n";
                   }
                   auto card = cardManager_->getCard(index, &status);
                   if (card != nullptr) {
                       cards_.emplace_back(card);
                   }
               }
           }
        } else {
           std::cout << "ERROR - Unable to initialize CardManager subsystem" << std::endl;
           return telux::common::Status::FAILED;
        }
     } else {
        std::cout << "ERROR - CardManager is null" << std::endl;
        return telux::common::Status::FAILED;
     }
     return telux::common::Status::SUCCESS;
}

std::vector<uint8_t> ApSimProfileClient::hexToBytes(const std::string &hex) {
    std::vector<uint8_t> bytes;
    try {
        for (unsigned int i = 0; i < hex.length(); i += 2) {
            std::string byteString = hex.substr(i, 2);
            uint8_t byte = (uint8_t)std::strtol(byteString.c_str(), nullptr, 16);
            bytes.push_back(byte);
        }
    } catch (const std::exception &e) {
        std::cout << "ERROR" << std::endl;
    }
    return bytes;
}

telux::common::Status ApSimProfileClient::registerRefresh(SlotId slotId) {
    telux::common::Status status = telux::common::Status::FAILED;
    telux::tel::RefreshParams config = {};
    std::vector<telux::tel::IccFile> efFiles = {};
    if (slotId == SLOT_ID_1) {
        config.sessionType = telux::tel::SessionType::CARD_ON_SLOT_1;
    } else if (slotId == SLOT_ID_2) {
        config.sessionType = telux::tel::SessionType::CARD_ON_SLOT_2;
    }
    if (cardManager_) {
        status = cardManager_->setupRefreshConfig(slotId, true, false, efFiles, config,
            CardRefreshResponseCallback::commandResponse);
    }
    return status;
}

/**
 * ICCID string as swapped string.
 */
std::string ApSimProfileClient::getSwappedIccidString(const std::string &data) {
    std::string reverseString = "";
    std::string text(data);

    // ICCID can be length of 19 or 20 digits
    if (text.length() < 19 || text.length() > 20) {
        std::cout << "Not a valid ICCID. Returning empty string" << "\n";
        return "";
    }

    for (unsigned int i = 0; i < text.length(); i += 2) {
        reverseString.push_back(text[i + 1]);
        reverseString.push_back(text[i]);
    }
    if (!reverseString.empty()) {
       reverseString.erase(std::remove(reverseString.begin(),reverseString.end(),' '),
           reverseString.end());
    }
    return reverseString;
}

telux::common::Status ApSimProfileClient::requestProfileList() {
    telux::common::Status status = telux::common::Status::FAILED;
    telux::tel::ApduExchangeStatus apduResult = telux::tel::ApduExchangeStatus::FAILURE;
    SlotId slotId = static_cast<SlotId>(indSlotId_);
    uint32_t referenceId = referenceId_;

    std::vector<std::string> iccidList = {};
    std::string iccidValue = "";
    // parse the config file
    ConfigParser config(FILE_NAME, FILE_PATH);
    try {
       iccidValue = config.getValue("GET_PROFILE_LIST");

    } catch (const std::exception &e) {
        std::cout << "ERROR: "<< " Unable to read from file" << std::endl;
    }
    if (iccidValue.empty()) {
        std::cout << "ERROR: " << "ICCID list is not configured, configure in tel.conf" << "\n";
       return status;
    }
    int position = 0;
    while ((position = iccidValue.find(',')) != std::string::npos) {
        iccidList.emplace_back(iccidValue.substr(0, position));
        iccidValue.erase(0, position + 1);
    }
    iccidList.emplace_back(iccidValue);
    for (auto i : iccidList) {
         i.erase(std::remove(i.begin(), i.end(),' '),i.end());
         std::cout << "ICCID: " << i << "\n";
    }
    if (!iccidList.empty()) {
        apduResult = telux::tel::ApduExchangeStatus::SUCCESS;
    }
    if (apSimProfileManager_) {
        status = apSimProfileManager_->sendRetrieveProfileListResponse(slotId, apduResult,
            referenceId, iccidList, MyApSimProfileCallback::onResponseCallback);
    } else {
        std::cout << "ERROR - ApSimProfileManger is null" << std::endl;
    }
    return status;
}

telux::common::Status ApSimProfileClient::enableProfile() {
    telux::common::Status status = telux::common::Status::FAILED;
    SlotId slotId = static_cast<SlotId>(indSlotId_);
    uint32_t referenceId = referenceId_;
    std::string iccidString = indIccid_;
    if (apSimProfileManager_) {
        // Exchange APDU command to enable the profile
        telux::tel::ApduExchangeStatus apduResult = telux::tel::ApduExchangeStatus::FAILURE;
        auto card = cards_[static_cast<int>(slotId) - 1];
        if (card) {
            int channel = -1;
            auto openLogicalCb = std::make_shared<OpenLogicalChannelCallback>();

            status = card->openLogicalChannel(ISD_R_AID, openLogicalCb);
            if (status == telux::common::Status::SUCCESS) {
                std::cout << "Open logical channel request sent successfully" << "\n";
                if (openLogicalCb->getFuture().get() == telux::common::ErrorCode::SUCCESS) {
                    std::cout << "Open logical channel is success" << "\n";
                    auto responseData =
                        std::static_pointer_cast<ResponseData>(openLogicalCb).get();
                    channel = responseData->getChannel();
                    if (channel < 0) {
                        std::cout << "Logical channel is invalid" << "\n";
                        return telux::common::Status::FAILED;
                    }
                }
            } else {
                std::cout << "Open logical channel request failed" << "\n";
                return telux::common::Status::FAILED;
            }

            // Refer GlobalPlatform Card Specification v.2.3. for more details
            uint8_t cla = 130; // 0x82
            uint8_t ins = 226; // 0xE2 - STORE_DATA
            uint8_t p1 = 145; // 0x91
            uint8_t p2 = 0;
            uint8_t p3 = 20; // size of the payload
            /* TAG(BF31) + length of payload(11) + choice tag(A0) + length of
               subsequent payload(0C) + Iccid tag(5A) + length of Iccid(0A)+ Iccid
               value + refresh tag(81) + length of refresh(01) + refresh value(FF) */
            std::stringstream ss;
            ss << "BF3111A00C5A0A" << getSwappedIccidString(iccidString) << "8101FF";
            std::string s = ss.str();
            std::vector<uint8_t> data = hexToBytes(s);

            auto apduLogicalCb = std::make_shared<TransmitApduCallback>();
            status = card->transmitApduLogicalChannel(channel, cla, ins, p1, p2, p3, data,
                apduLogicalCb);
            if (status == telux::common::Status::SUCCESS) {
                std::cout << "Transmit APDU request sent successfully" << "\n";
                if (apduLogicalCb->getFuture().get() == telux::common::ErrorCode::SUCCESS) {
                    auto apduData  = std::static_pointer_cast<ResponseData>(apduLogicalCb).get();

                    if ((apduData->getResult().sw1 == SW1_NO_ERROR) &&
                        (apduData->getResult().sw2 == SW2_NO_ERROR)) {
                         int result =
                             apduData->getResult().data[(apduData->getResult().data.size()) -
                             GET_MORE_RESPONSE_RESULT_BYTE];
                         std::cout << "Enable profile result from card : " << result <<"\n";
                         if (result == CODE_OK) {
                             apduResult = telux::tel::ApduExchangeStatus::SUCCESS;
                             std::cout << "Profile enabled successfully" << "\n";
                         } else {
                             apduResult = telux::tel::ApduExchangeStatus::FAILURE;
                             std::cout << "Enable profile is failed" << "\n";
                         }
                    } else if (apduData->getResult().sw1 == SW1_MORE_RESPONSE) {
                        std::vector<uint8_t> data = {};
                        int remainingBytes = apduData->getResult().sw2;
                        std::cout << "Remaining bytes: " << remainingBytes << "\n";
                        auto apduMoreLogicalCb = std::make_shared<TransmitApduCallback>();
                        status = card->transmitApduLogicalChannel(channel, 0,
                            INS_GET_MORE_RESPONSE, 0, 0, remainingBytes, data, apduMoreLogicalCb);
                        if (status == telux::common::Status::SUCCESS) {
                            std::cout << "Transmit APDU for more data sent successfully" << "\n";
                            if (apduMoreLogicalCb->getFuture().get() ==
                                telux::common::ErrorCode::SUCCESS) {
                                std::cout << "Transmit APDU for more data is success" << "\n";
                                auto apduMoreData = std::static_pointer_cast<ResponseData>
                                    (apduMoreLogicalCb).get();
                                if ((apduMoreData->getResult().sw1 == SW1_NO_ERROR) &&
                                    (apduMoreData->getResult().sw2 == SW2_NO_ERROR)) {
                                    int result = apduMoreData->getResult().data[(
                                        apduMoreData->getResult().data.size()) -
                                        GET_MORE_RESPONSE_RESULT_BYTE];
                                    std::cout << "Enable profile result from card : " << result
                                        <<"\n";
                                    if (result == CODE_OK) {
                                        apduResult = telux::tel::ApduExchangeStatus::SUCCESS;
                                        std::cout << "Profile enabled successfully" << "\n";
                                    } else {
                                        apduResult = telux::tel::ApduExchangeStatus::FAILURE;
                                        std::cout << "Enable profile is failed" << "\n";
                                    }
                                } else {
                                    apduResult = telux::tel::ApduExchangeStatus::FAILURE;
                                    std::cout << "APDU command for GET_RESPONSE is failed" << "\n";
                                }
                            } else {
                                apduResult = telux::tel::ApduExchangeStatus::FAILURE;
                                std::cout << "Transmit APDU for more data is failed" << "\n";
                            }
                        } else {
                            apduResult = telux::tel::ApduExchangeStatus::FAILURE;
                            std::cout << "Transmit APDU for more data request is failed" << "\n";
                        }
                    } else {
                         apduResult = telux::tel::ApduExchangeStatus::FAILURE;
                         std::cout << "Transmit APDU for more data request is failed" << "\n";
                    }
                } else {
                    apduResult = telux::tel::ApduExchangeStatus::FAILURE;
                    std::cout << "Transmit APDU for data is failed" << "\n";
                }
            } else {
                std::cout << "Transmit APDU request failed" << "\n";
            }

            auto closeLogicalCb = std::make_shared<CloseLogicalChannelCallback>();
            status = card->closeLogicalChannel(channel, closeLogicalCb);
            if (status == telux::common::Status::SUCCESS) {
                if (closeLogicalCb->getFuture().get() == telux::common::ErrorCode::SUCCESS) {
                    std::cout << "Logical channel closed successfully" << "\n";
                } else if (refreshSlotId_ == static_cast<int>(slotId) &&
                    refreshMode_ == telux::tel::RefreshMode::RESET) {
                    std::cout << "Logical channel closed due to SIM refresh" << "\n";
                } else {
                    std::cout << "Close logical channel is failed" << "\n";
                }
            }

            /* workaround: If the profile is enabled and the response of GET_RESPONSE is error/not
               received but in between SIM Refresh ind is received which indicates profile is
               enabled, considering that apdu result will be sent as success. */
            if (refreshSlotId_ == static_cast<int>(slotId) &&
                refreshMode_ == telux::tel::RefreshMode::RESET) {
                apduResult = telux::tel::ApduExchangeStatus::SUCCESS;
                std::cout << "Profile enabled successfully" << "\n";
            }
            std::cout << "Enable APDU response : " << static_cast<int>(apduResult) << "\n";
        }
        status = apSimProfileManager_->sendProfileOperationResponse(slotId, apduResult,
            referenceId, MyApSimProfileCallback::onResponseCallback);
    } else {
        std::cout << "ERROR - ApSimProfileManger is null" << std::endl;
    }
    return status;
}

telux::common::Status ApSimProfileClient::disableProfile() {
    telux::common::Status status = telux::common::Status::FAILED;
    SlotId slotId = static_cast<SlotId>(indSlotId_);
    uint32_t referenceId = referenceId_;
    std::string iccidString = indIccid_;
    if (apSimProfileManager_) {
        // Exchange APDU command to disable the profile
        telux::tel::ApduExchangeStatus apduResult = telux::tel::ApduExchangeStatus::FAILURE;
        auto card = cards_[static_cast<int>(slotId) - 1];
        if (card) {
            int channel = -1;
            auto openLogicalCb = std::make_shared<OpenLogicalChannelCallback>();

            status = card->openLogicalChannel(ISD_R_AID, openLogicalCb);
            if (status == telux::common::Status::SUCCESS) {
                std::cout << "Open logical channel request sent successfully" << "\n";
                if (openLogicalCb->getFuture().get() == telux::common::ErrorCode::SUCCESS) {
                    std::cout << "Open logical channel is success" << "\n";
                    auto responseData = std::static_pointer_cast<ResponseData>(openLogicalCb).get();
                    channel = responseData->getChannel();
                    if (channel < 0) {
                        std::cout << "Logical channel is invalid" << "\n";
                        return telux::common::Status::FAILED;
                    }
                }
            } else {
                std::cout << "Open logical channel request failed" << "\n";
                return telux::common::Status::FAILED;
            }

            // Refer GlobalPlatform Card Specification v.2.3. for more details
            uint8_t cla = 130; // 0x82
            uint8_t ins = 226; // 0xE2 - STORE_DATA
            uint8_t p1 = 145; // 0x91
            uint8_t p2 = 0;
            uint8_t p3 = 20; //size of the payload
            /* TAG(BF32) + length of payload(11) + choice tag(A0) + length of
               subsequent payload(0C) + Iccid tag(5A) + length of Iccid(0A)+ Iccid
               value + refresh tag(81) + length of refresh(01) + refresh value(FF) */
            std::stringstream ss;
            ss << "BF3211A00C5A0A" << getSwappedIccidString(iccidString) << "8101FF";
            std::string s = ss.str();
            std::vector<uint8_t> data = hexToBytes(s);
            auto apduLogicalCb = std::make_shared<TransmitApduCallback>();
            status = card->transmitApduLogicalChannel(channel, cla, ins, p1, p2, p3, data,
                apduLogicalCb);
            if (status == telux::common::Status::SUCCESS) {
                std::cout << "Transmit APDU request sent successfully" << "\n";
                if (apduLogicalCb->getFuture().get() == telux::common::ErrorCode::SUCCESS) {
                    auto apduData  = std::static_pointer_cast<ResponseData>(apduLogicalCb).get();

                    if ((apduData->getResult().sw1 == SW1_NO_ERROR) &&
                        (apduData->getResult().sw2 == SW2_NO_ERROR)) {
                         int result = apduData->getResult().data[
                             (apduData->getResult().data.size())-3];
                         std::cout << "Disable profile result from card : " << result <<"\n";
                         if (result == CODE_OK) {
                             apduResult = telux::tel::ApduExchangeStatus::SUCCESS;
                             std::cout << "Profile disabled successfully" << "\n";
                         } else {
                             apduResult = telux::tel::ApduExchangeStatus::FAILURE;
                             std::cout << "Disable profile is failed" << "\n";
                         }
                    } else if (apduData->getResult().sw1 == SW1_MORE_RESPONSE) {
                         std::cout << "Transmit APDU for more data sent successfully" << "\n";
                         std::vector<uint8_t> data = {};
                         int remainingBytes = apduData->getResult().sw2;
                         std::cout << "Remaining bytes: " << remainingBytes << "\n";
                         auto apduMoreLogicalCb = std::make_shared<TransmitApduCallback>();
                         status = card->transmitApduLogicalChannel(channel, 0,
                             INS_GET_MORE_RESPONSE, 0, 0, remainingBytes, data,
                             apduMoreLogicalCb);
                         if (status == telux::common::Status::SUCCESS) {
                             std::cout << "Transmit APDU for more data sent successfully" << "\n";
                             if (apduMoreLogicalCb->getFuture().get() ==
                                 telux::common::ErrorCode::SUCCESS) {
                                 std::cout << "Transmit APDU for more data is success" << "\n";
                                 auto apduMoreData = std::static_pointer_cast<ResponseData>
                                     (apduMoreLogicalCb).get();
                                 if ((apduMoreData->getResult().sw1 == SW1_NO_ERROR) &&
                                     (apduMoreData->getResult().sw2 == SW2_NO_ERROR)) {
                                     int result = apduMoreData->getResult().data[
                                         (apduMoreData->getResult().data.size()) -
                                         GET_MORE_RESPONSE_RESULT_BYTE];
                                     std::cout << "Disable profile result from card : " << result
                                         <<"\n";
                                     if (result == CODE_OK) {
                                         apduResult = telux::tel::ApduExchangeStatus::SUCCESS;
                                         std::cout << "Profile disabled successfully" << "\n";
                                     } else {
                                         apduResult = telux::tel::ApduExchangeStatus::FAILURE;
                                         std::cout << "Disable profile is failed" << "\n";
                                     }
                                 } else {
                                     apduResult = telux::tel::ApduExchangeStatus::FAILURE;
                                     std::cout << "APDU command for GET_RESPONSE is failed" << "\n";
                                 }
                              } else {
                                 apduResult = telux::tel::ApduExchangeStatus::FAILURE;
                                 std::cout << "Transmit APDU for more data is failed" << "\n";
                              }
                         } else {
                              apduResult = telux::tel::ApduExchangeStatus::FAILURE;
                              std::cout << "Transmit APDU for more data is failed" << "\n";
                         }
                    } else {
                         apduResult = telux::tel::ApduExchangeStatus::FAILURE;
                         std::cout << "Transmit APDU for more data request is failed" << "\n";
                    }
                } else {
                   apduResult = telux::tel::ApduExchangeStatus::FAILURE;
                   std::cout << "Transmit APDU for data is failed" << "\n";
                }
            } else {
                std::cout << "Transmit APDU request failed" << "\n";
            }

            auto closeLogicalCb = std::make_shared<CloseLogicalChannelCallback>();
            status = card->closeLogicalChannel(channel, closeLogicalCb);
            if (status == telux::common::Status::SUCCESS) {
                if (closeLogicalCb->getFuture().get() == telux::common::ErrorCode::SUCCESS) {
                    std::cout << "Logical channel closed successfully" << "\n";
                } else if (refreshSlotId_ == static_cast<int>(slotId) &&
                   refreshMode_ == telux::tel::RefreshMode::RESET) {
                   std::cout << "Logical channel closed due to SIM refresh" << "\n";
               } else {
                   std::cout << "Close logical channel is failed" << "\n";
               }
            }
            /* workaround: If the profile is enabled and the response of GET_RESPONSE is error/not
               received but in between SIM Refresh ind is received which indicates profile is
               enabled, considering that apdu result will be sent as success. */
            if (refreshSlotId_ == static_cast<int>(slotId) &&
                refreshMode_ == telux::tel::RefreshMode::RESET) {
                apduResult = telux::tel::ApduExchangeStatus::SUCCESS;
                std::cout << "Profile disabled successfully" << "\n";
            }

            std::cout << "Disable APDU response :" << static_cast<int>(apduResult) << "\n";
        }
        status = apSimProfileManager_->sendProfileOperationResponse(slotId, apduResult,
            referenceId, MyApSimProfileCallback::onResponseCallback);
    } else {
        std::cout << "ERROR - ApSimProfileManger is null" << std::endl;
    }
    return status;
}
