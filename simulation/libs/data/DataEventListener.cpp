/*
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <telux/data/DataDefines.hpp>
#include <telux/data/DataFactory.hpp>

#include "DataEventListener.hpp"
#include "DataConnectionManagerStub.hpp"
#include "DataUtilsStub.hpp"

namespace telux {
namespace data {

DataEventListener::DataEventListener(std::weak_ptr<DataConnectionManagerStub> manager)
: dataConnectionMngr_(manager) {
    LOG(DEBUG, __FUNCTION__);
}

DataEventListener::~DataEventListener() {
    LOG(DEBUG, __FUNCTION__);
}

void DataEventListener::onEventUpdate(std::string event) {
    LOG(DEBUG, __FUNCTION__);
    std::string token;
    if (EVENT_FLAG ==  EventParserUtil::getNextToken(event, DEFAULT_DELIMITER)) {
        token = EventParserUtil::getNextToken(event, DEFAULT_DELIMITER);
        handleEvent(token, event);
    }
    else {
        LOG(ERROR, __FUNCTION__, "The event flag is not set!");
    }
}

void DataEventListener::handleEvent(std::string token, std::string event) {
    LOG(DEBUG, __FUNCTION__, "The received event is: \"",token,"\"");
    if (token == "") {
        LOG(ERROR, __FUNCTION__, "The event flag is not set!");
        return;
    }
    LOG(DEBUG, __FUNCTION__, "The data event type is: ", token, " The leftover string is: ", event);
    if (token == START_DATA_CALL) {
        handleStartDataCallEvent(event);
    } else if (token == STOP_DATA_CALL) {
        handleStopDataCallEvent(event);
    }
}

void DataEventListener::handleStartDataCallEvent(std::string eventParams) {
    LOG(DEBUG, __FUNCTION__, "The received event Params are: \"",eventParams,"\"");
    int profileId = std::stoi(EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER));
    SlotId slotId =
        static_cast<SlotId>(std::stoi(EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER)));
    std::string ifaceName = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    IpFamilyType ipFamilyType = static_cast<IpFamilyType>(DataUtilsStub::convertIpFamilyStringToEnum(
        EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER)));
    std::string ipv4Address = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    std::string gwv4Address = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    std::string dnsPrimaryAddress = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    std::string dnsSecondaryAddress = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    std::string ipv6Address = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    std::string gwv6Address = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);

    auto mngr = dataConnectionMngr_.lock();
    if (mngr) {
        LOG(DEBUG, __FUNCTION__, " invoking handleStartDataCallEvent");
        mngr->handleStartDataCallEvent(profileId, slotId, ifaceName,
            ipFamilyType,  ipv4Address, gwv4Address, dnsPrimaryAddress,
            dnsSecondaryAddress, ipv6Address, gwv6Address);
    }
}

void DataEventListener::handleStopDataCallEvent(std::string eventParams) {
    LOG(DEBUG, __FUNCTION__, "The received event Params are: \"",eventParams,"\"");
    int profileId = std::stoi(EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER));
    SlotId slotId =
        static_cast<SlotId>(std::stoi(EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER)));
    IpFamilyType ipFamilyType = static_cast<IpFamilyType>(DataUtilsStub::convertIpFamilyStringToEnum(
        EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER)));

    auto mngr = dataConnectionMngr_.lock();
    if (mngr) {
        LOG(DEBUG, __FUNCTION__, " invoking handleStopDataCallEvent");
        mngr->handleStopDataCallEvent(profileId, slotId, ipFamilyType);
    }
}

}
}