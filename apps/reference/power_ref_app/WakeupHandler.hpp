/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef WAKEUP_HANDLER_HPP
#define WAKEUP_HANDLER_HPP

#include <telux/common/Log.hpp>
#include <telux/power/WakeupManager.hpp>

#include "EventManager.hpp"
#include "common/socket/ConnectionHandler.hpp"

class WakeupHandler : public telux::power::IWakeupListener,
                      public IEventListener,
                      public enable_shared_from_this<WakeupHandler> {

 public:
    static std::shared_ptr<WakeupHandler> getInstance(std::shared_ptr<EventManager> eventManager);

    WakeupHandler(const WakeupHandler &)            = delete;
    WakeupHandler &operator=(const WakeupHandler &) = delete;

    ~WakeupHandler();
    bool init();

    void onEventRejected(shared_ptr<Event> event, EventStatus reason) override;
    void onEventProcessed(shared_ptr<Event> event, bool success) override;
    void preProcessEvent(shared_ptr<Event> event) override;
    void onWakeup(const telux::power::WakeupEventInfo &wakeupInfo) override;
    void onServiceStatusChange(telux::common::ServiceStatus status) override;

 private:
    WakeupHandler(std::shared_ptr<EventManager> eventManager);

    std::shared_ptr<EventManager> eventManager_;
    std::shared_ptr<telux::power::IWakeupManager> wakeupManager_;
    std::bitset<32> resumeWakeupConfig_;

    static constexpr size_t RESUME_ON_QMI_BIT = 0;  // conf bit 0 => WakeupType::QMI
    static constexpr size_t RESUME_ON_WOW_BIT = 1;  // conf bit 1 => WakeupType::WOW

    static const char *wowCategoryToStr(telux::power::WowWakeupCategory category) {
        switch (category) {
            case telux::power::WowWakeupCategory::WLAN_PROTOCOL:
                return "WLAN_PROTOCOL";
            case telux::power::WowWakeupCategory::OFFLOAD:
                return "OFFLOAD";
            case telux::power::WowWakeupCategory::PATTERN_FILTER:
                return "PATTERN_FILTER";
            case telux::power::WowWakeupCategory::MAGIC_PACKET:
                return "MAGIC_PACKET";
            case telux::power::WowWakeupCategory::SYSTEM:
                return "SYSTEM";
            default:
                return "UNSPECIFIED";
        }
    }
};

#endif  // WAKEUP_HANDLER_HPP
