/*
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef DATA_EVENT_LISTENER_HPP
#define DATA_EVENT_LISTENER_HPP

#include <memory>
#include "../common/event-manager/EventManager.hpp"

#define DEFAULT_DELIMITER " "
#define DATA_CONNECTION_FILTER "data_connection"
#define START_DATA_CALL "startDataCall"
#define STOP_DATA_CALL "stopDataCall"

namespace telux {
namespace data {

class DataConnectionManagerStub;

class DataEventListener : public telux::common::IEventListener {
public:
    DataEventListener(std::weak_ptr<DataConnectionManagerStub> manager);
    ~DataEventListener();
    void onEventUpdate(std::string event) override;
private:
    void handleEvent(std::string token, std::string event);
    void handleStartDataCallEvent(std::string eventParams);
    void handleStopDataCallEvent(std::string eventParams);

    std::weak_ptr<DataConnectionManagerStub> dataConnectionMngr_;
};

} // end of namespace data
} // end of namespace telux

#endif // DATA_EVENT_LISTENER_HPP