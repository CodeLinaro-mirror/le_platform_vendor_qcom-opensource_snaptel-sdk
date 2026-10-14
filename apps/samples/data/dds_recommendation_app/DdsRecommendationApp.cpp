/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * This sample application demonstrates the usage of the DualDataManager API
 * to manage DDS (Default Data Subscription) recommendations.
 *
 * The steps are as follows:
 *
 *  1.  Get DataFactory instance.
 *  2.  Get IDualDataManager instance from DataFactory (with init callback).
 *  3.  Wait for the DualData service to become available.
 *  4.  Register a listener (IDualDataListener) to receive DDS recommendation
 *      and dual data usage recommendation change notifications.
 *  5.  Configure DDS switch recommendation using configureDdsSwitchRecommendation
 *      (enableTemporaryRecommendations, enablePermanentRecommendations, recommBasis).
 *  6.  Query dual data capability using getDualDataCapability.
 *  7.  Query current dual data usage recommendation using getDualDataUsageRecommendation.
 *  8.  Handle onDualDataUsageRecommendationChange notification (e.g., NOT_RECOMMENDED,
 *      NOT_ALLOWED) - stop data activities on nDDS slot as appropriate.
 *  9.  Handle onDdsSwitchRecommendation notification.
 * 10.  Perform DDS switch using requestDdsSwitch (TEMPORARY or PERMANENT).
 * 11.  requestDdsSwitch callback with SUCCESS or failure.
 * 12.  Deregister the listener when done.
 *
 * Terminology:
 *  - DDS  : Default Data Subscription - the SIM slot currently handling data traffic.
 *  - nDDS : Non-Default Data Subscription - the other SIM slot not handling data.
 *  - DSDA : Dual SIM Dual Active - both SIM slots active simultaneously.
 *  - DSDS : Dual SIM Dual Standby - only one SIM slot active at a time.
 *  - FC   : Full Concurrency - both SIM slots can handle voice and data concurrently.
 *  - TxS  : Transmission Sharing - modem shares transmission resources between SIM slots.
 *
 * Scenarios covered:
 *  - DDS recommendation basic flow (indication and request paths).
 *  - DSDA FC (Full Concurrency) to TxS (Transmission Sharing) mode change with active
 *      voice call on nDDS (Non-Default Data Subscription):
 *      Cause: TEMP_CAUSE_CODE_TX_SHARING, type: TEMPORARY/HIGH.
 *  - DSDA (Dual SIM Dual Active) to DSDS (Dual SIM Dual Standby) sub-mode transition
 *      with active voice call on nDDS:
 *      Cause: TEMP_CAUSE_CODE_DSDA_IMPOSSIBLE, type: TEMPORARY/HIGH.
 *  - Voice call on nDDS ends:
 *      Cause: TEMP_CAUSE_CODE_CALL_STATUS_CHANGED, type: REVOKE -> switch back PERMANENT.
 *
 * Usage:
 * # ./dds_recommendation_app
 */

#include <future>
#include <iostream>
#include <memory>
#include <string>

#include <telux/data/DataFactory.hpp>
#include <telux/data/DualDataManager.hpp>

// ---------------------------------------------------------------------------
// Helper utilities
// ---------------------------------------------------------------------------

static std::string usageRecommendationToStr(
    telux::data::DualDataUsageRecommendation recommendation) {
    switch (recommendation) {
        case telux::data::DualDataUsageRecommendation::ALLOWED:
            return "ALLOWED";
        case telux::data::DualDataUsageRecommendation::NOT_ALLOWED:
            return "NOT_ALLOWED";
        case telux::data::DualDataUsageRecommendation::NOT_RECOMMENDED:
            return "NOT_RECOMMENDED";
        default:
            return "UNKNOWN";
    }
}

static std::string ddsTypeToStr(telux::data::DdsType type) {
    switch (type) {
        case telux::data::DdsType::PERMANENT:
            return "PERMANENT";
        case telux::data::DdsType::TEMPORARY:
            return "TEMPORARY";
        default:
            return "UNKNOWN";
    }
}

static std::string tempRecommendationTypeToStr(telux::data::TemporaryRecommendationType type) {
    switch (type) {
        case telux::data::TemporaryRecommendationType::REVOKE:
            return "REVOKE";
        case telux::data::TemporaryRecommendationType::LOW:
            return "LOW";
        case telux::data::TemporaryRecommendationType::HIGH:
            return "HIGH";
        default:
            return "UNKNOWN";
    }
}

// TemporaryRecommendationCauseCodes is a bitmask/flags type; bitwise-AND checks are intentional.
static std::string tempCauseCodesToStr(telux::data::TemporaryRecommendationCauseCodes cause) {
    std::string result;
    if (cause & telux::data::TEMP_CAUSE_CODE_DSDA_IMPOSSIBLE)
        result += "DSDA_IMPOSSIBLE ";
    if (cause & telux::data::TEMP_CAUSE_CODE_DDS_INTERNET_UNAVAIL)
        result += "DDS_INTERNET_UNAVAIL ";
    if (cause & telux::data::TEMP_CAUSE_CODE_TX_SHARING)
        result += "TX_SHARING ";
    if (cause & telux::data::TEMP_CAUSE_CODE_CALL_STATUS_CHANGED)
        result += "CALL_STATUS_CHANGED ";
    if (cause & telux::data::TEMP_CAUSE_CODE_ACTIVE_CALL_ON_DDS)
        result += "ACTIVE_CALL_ON_DDS ";
    if (cause & telux::data::TEMP_CAUSE_CODE_TEMP_REC_DISABLED)
        result += "TEMP_REC_DISABLED ";
    if (cause & telux::data::TEMP_CAUSE_CODE_NON_DDS_INTERNET_UNAVAIL)
        result += "NON_DDS_INTERNET_UNAVAIL ";
    if (cause & telux::data::TEMP_CAUSE_CODE_DATA_OFF)
        result += "DATA_OFF ";
    if (cause & telux::data::TEMP_CAUSE_CODE_EMERGENCY_CALL_ON_GOING)
        result += "EMERGENCY_CALL_ON_GOING ";
    if (cause & telux::data::TEMP_CAUSE_CODE_DDS_SIM_REMOVED)
        result += "DDS_SIM_REMOVED ";
    if (result.empty())
        result = "UNKNOWN";
    return result;
}

// ---------------------------------------------------------------------------
// IDualDataListener implementation
// ---------------------------------------------------------------------------

/**
 * DdsRecommendationListener handles all DualData notifications:
 *  - onDualDataUsageRecommendationChange: notified when dual data usage recommendation changes.
 *  - onDdsSwitchRecommendation: notified when a DDS switch recommendation is available.
 *  - onDdsChange: notified when a DDS switch occurs.
 *  - onServiceStatusChange: notified when service status changes.
 */
class DdsRecommendationListener : public telux::data::IDualDataListener {
 public:
    explicit DdsRecommendationListener(std::shared_ptr<telux::data::IDualDataManager> dualDataMgr)
       : dualDataMgr_(dualDataMgr) {
    }

    /**
     * [8] Called when the dual data usage recommendation changes.
     *
     * Scenarios:
     *  - NOT_RECOMMENDED: UE transitioned from DSDA FC mode to DSDA TxS mode or DSDS sub-mode.
     *    OEM should stop data activities on nDDS slot.
     *  - NOT_ALLOWED: Long running data calls are not allowed on both SIM slots.
     *  - ALLOWED: Long running data calls on both SIM slots are allowed.
     */
    void onDualDataUsageRecommendationChange(
        telux::data::DualDataUsageRecommendation recommendation) override {
        std::cout << "\n[8] onDualDataUsageRecommendationChange: DualDataUsageRecommendation="
                  << usageRecommendationToStr(recommendation) << std::endl;

        switch (recommendation) {
            case telux::data::DualDataUsageRecommendation::NOT_ALLOWED:
                std::cout << "  -> Data activities MUST be stopped on the nDDS slot immediately."
                          << std::endl;
                break;
            case telux::data::DualDataUsageRecommendation::NOT_RECOMMENDED:
                std::cout << "  -> Recommendation: stop data activities on nDDS slot to avoid "
                             "performance degradation."
                          << std::endl;
                break;
            case telux::data::DualDataUsageRecommendation::ALLOWED:
                std::cout << "  -> Long running data calls on both SIM slots are allowed."
                          << std::endl;
                break;
            default:
                break;
        }
    }

    /**
     * [9] Called when a DDS switch recommendation is received from the modem.
     *
     * Scenarios:
     *  - DSDA FC to TxS mode change (TEMP_CAUSE_CODE_TX_SHARING):
     *      Temporary HIGH recommendation to switch to nDDS (Sub2).
     *  - DSDA to DSDS sub-mode transition (TEMP_CAUSE_CODE_DSDA_IMPOSSIBLE):
     *      Temporary HIGH recommendation to switch to nDDS (Sub2).
     *  - Voice call on nDDS ends (TEMP_CAUSE_CODE_CALL_STATUS_CHANGED):
     *      Temporary REVOKE recommendation to switch back to original DDS (Sub1) permanently.
     */
    void onDdsSwitchRecommendation(
        const telux::data::DdsSwitchRecommendation ddsSwitchRecommendation) override {
        std::cout << "\n[9] onDdsSwitchRecommendation received:" << std::endl;
        std::cout << "  Recommended SlotId: "
                  << static_cast<int>(ddsSwitchRecommendation.recommendedDdsInfo.slotId)
                  << std::endl;
        std::cout << "  DDS Type: " << ddsTypeToStr(ddsSwitchRecommendation.recommendedDdsInfo.type)
                  << std::endl;

        const auto &details = ddsSwitchRecommendation.recommendationDetails;

        if (ddsSwitchRecommendation.recommendedDdsInfo.type == telux::data::DdsType::TEMPORARY) {
            std::cout
                << "  Temp Recommendation Type: " << tempRecommendationTypeToStr(details.tempType)
                << std::endl;
            std::cout << "  Temp Cause: " << tempCauseCodesToStr(details.tempCause) << std::endl;

            if (details.tempType == telux::data::TemporaryRecommendationType::REVOKE) {
                // Voice call on nDDS ended - switch back to original DDS permanently
                std::cout << "  -> Voice call ended. OEM logic: switch back to original DDS."
                          << std::endl;
                performDdsSwitch(ddsSwitchRecommendation.recommendedDdsInfo);
            } else if (details.tempType == telux::data::TemporaryRecommendationType::HIGH) {
                // Temporary switch to nDDS required (e.g., TX_SHARING or DSDA_IMPOSSIBLE)
                std::cout << "  -> Temporary DDS switch required. Performing switch to SlotId="
                          << static_cast<int>(ddsSwitchRecommendation.recommendedDdsInfo.slotId)
                          << std::endl;
                performDdsSwitch(ddsSwitchRecommendation.recommendedDdsInfo);
            } else {
                std::cout << "  -> Low priority recommendation. OEM may choose to switch."
                          << std::endl;
            }
        } else {
            // Permanent recommendation
            std::cout << "  Permanent Cause Code: 0x" << std::hex << details.permCause << std::dec
                      << std::endl;
            std::cout << "  -> Permanent DDS switch recommended." << std::endl;
            performDdsSwitch(ddsSwitchRecommendation.recommendedDdsInfo);
        }
    }

    /**
     * Called when a DDS switch occurs (confirmation of switch).
     */
    void onDdsChange(telux::data::DdsInfo currentState) override {
        std::cout << "\n[Indication] onDdsChange: SlotId=" << static_cast<int>(currentState.slotId)
                  << " Type=" << ddsTypeToStr(currentState.type) << std::endl;
    }

    /**
     * Called when the DualDataManager service status changes.
     */
    void onServiceStatusChange(telux::common::ServiceStatus status) override {
        std::cout << "\n[Indication] onServiceStatusChange: status=" << static_cast<int>(status)
                  << std::endl;
    }

 private:
    /**
     * [10] Perform DDS switch based on the recommendation.
     * After a successful TEMPORARY switch:
     *   - OEM must perform TCP cleanup.
     *   - Release internet PDN/PDU connections on original DDS (Sub1).
     *   - Switch application traffic to temp DDS (Sub2).
     * After a successful PERMANENT switch back:
     *   - OEM must perform TCP cleanup.
     *   - Release internet PDN/PDU connections on temp DDS (Sub2).
     *   - Switch application traffic back to original DDS (Sub1).
     */
    void performDdsSwitch(const telux::data::DdsInfo &ddsInfo) {
        if (!dualDataMgr_) {
            std::cout << "  DualDataManager not available for DDS switch." << std::endl;
            return;
        }

        std::cout << "  [10] requestDdsSwitch: SlotId=" << static_cast<int>(ddsInfo.slotId)
                  << " Type=" << ddsTypeToStr(ddsInfo.type) << std::endl;

        // [10] Request DDS switch
        auto status = dualDataMgr_->requestDdsSwitch(
            ddsInfo, [ddsInfo](telux::common::ErrorCode errorCode) {
                // [11] Callback with SUCCESS or failure
                if (errorCode == telux::common::ErrorCode::SUCCESS) {
                    std::cout << "  [11] requestDdsSwitch Callback: SUCCESS" << std::endl;
                    if (ddsInfo.type == telux::data::DdsType::TEMPORARY) {
                        std::cout << "  OEM action: Perform TCP cleanup, release internet "
                                     "PDN/PDU connections on original DDS, switch traffic "
                                     "to temp DDS."
                                  << std::endl;
                    } else {
                        std::cout << "  OEM action: Perform TCP cleanup, release internet "
                                     "PDN/PDU connections on temp DDS, switch traffic "
                                     "back to original DDS."
                                  << std::endl;
                    }
                } else {
                    std::cout << "  [11] requestDdsSwitch Callback: FAILED, errorCode="
                              << static_cast<int>(errorCode) << std::endl;
                }
            });

        if (status != telux::common::Status::SUCCESS) {
            std::cout << "  requestDdsSwitch returned status=" << static_cast<int>(status)
                      << std::endl;
        }
    }

    std::shared_ptr<telux::data::IDualDataManager> dualDataMgr_;
};

// ---------------------------------------------------------------------------
// Main application class
// ---------------------------------------------------------------------------

/**
 * DdsRecommendationApp demonstrates the full DDS recommendation flow:
 *  - Initialization
 *  - Listener registration
 *  - DDS switch recommendation configuration
 *  - Dual data capability and usage recommendation query
 *  - Handling DDS switch recommendations asynchronously
 */
class DdsRecommendationApp {
 public:
    /**
     * Initialize the DualDataManager and configure DDS recommendation.
     * @returns true on success, false on failure.
     */
    bool init() {
        std::promise<telux::common::ServiceStatus> p{};

        // [1] Get DataFactory instance
        auto &dataFactory = telux::data::DataFactory::getInstance();

        // [2] Get IDualDataManager instance from DataFactory with init callback
        dualDataMgr_ = dataFactory.getDualDataManager(
            [&p](telux::common::ServiceStatus status) { p.set_value(status); });

        if (!dualDataMgr_) {
            std::cout << "Failed to get IDualDataManager from DataFactory." << std::endl;
            return false;
        }

        // [3] Wait for the DualData service to become available
        auto serviceStatus = p.get_future().get();
        if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "DualData service unavailable, status=" << static_cast<int>(serviceStatus)
                      << std::endl;
            return false;
        }
        std::cout << "[3] DualData service is AVAILABLE." << std::endl;

        // [4] Register listener for DDS recommendation and usage recommendation notifications
        listener_      = std::make_shared<DdsRecommendationListener>(dualDataMgr_);
        auto regStatus = dualDataMgr_->registerListener(listener_);
        if (regStatus != telux::common::Status::SUCCESS) {
            std::cout << "[4] registerListener failed, status=" << static_cast<int>(regStatus)
                      << std::endl;
            return false;
        }
        std::cout << "[4] registerListener(IDualDataListener): SUCCESS" << std::endl;

        // [5] Configure DDS switch recommendation
        //     DdsSwitchRecommendationConfig:
        //       enableTemporaryRecommendations: true
        //       enablePermanentRecommendations: true
        //       recommBasis: THROUGHPUT
        telux::data::DdsSwitchRecommendationConfig config;
        config.enableTemporaryRecommendations = true;
        config.enablePermanentRecommendations = true;
        config.recommBasis                    = telux::data::DDSRecommendationBasis::THROUGHPUT;

        auto errCode = dualDataMgr_->configureDdsSwitchRecommendation(config);
        if (errCode != telux::common::ErrorCode::SUCCESS) {
            std::cout << "[5] configureDdsSwitchRecommendation failed, errorCode="
                      << static_cast<int>(errCode) << std::endl;
            cleanUp();
            return false;
        }
        std::cout << "[5] configureDdsSwitchRecommendation: SUCCESS "
                     "(enableTemporary=true, enablePermanent=true, basis=THROUGHPUT)"
                  << std::endl;

        // [6] Query dual data capability
        bool isCapable = false;
        errCode        = dualDataMgr_->getDualDataCapability(isCapable);
        if (errCode != telux::common::ErrorCode::SUCCESS) {
            std::cout << "[6] getDualDataCapability failed, errorCode=" << static_cast<int>(errCode)
                      << std::endl;
            cleanUp();
            return false;
        }
        std::cout << "[6] getDualDataCapability: isCapable=" << (isCapable ? "true" : "false")
                  << std::endl;

        if (!isCapable) {
            std::cout << "Device does not support dual data. Exiting." << std::endl;
            cleanUp();
            return false;
        }

        // [7] Query current dual data usage recommendation
        telux::data::DualDataUsageRecommendation usageRecommendation;
        errCode = dualDataMgr_->getDualDataUsageRecommendation(usageRecommendation);
        if (errCode != telux::common::ErrorCode::SUCCESS) {
            std::cout << "[7] getDualDataUsageRecommendation failed, errorCode="
                      << static_cast<int>(errCode) << std::endl;
        } else {
            std::cout << "[7] getDualDataUsageRecommendation: "
                      << usageRecommendationToStr(usageRecommendation) << std::endl;
        }

        return true;
    }

    /**
     * Clean up: deregister listener and release manager reference.
     */
    void cleanUp() {
        if (dualDataMgr_ && listener_) {
            dualDataMgr_->deregisterListener(listener_);
            std::cout << "Listener deregistered." << std::endl;
        }
        dualDataMgr_ = nullptr;
        listener_    = nullptr;
    }

 private:
    std::shared_ptr<telux::data::IDualDataManager> dualDataMgr_;
    std::shared_ptr<DdsRecommendationListener> listener_;
};

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(int argc, char *argv[]) {
    std::cout << "=== DDS Recommendation Sample App ===" << std::endl;
    std::cout << "Demonstrates DDS recommendation management using IDualDataManager." << std::endl;
    std::cout << std::endl;

    DdsRecommendationApp app;

    if (!app.init()) {
        std::cout << "Failed to initialize DdsRecommendationApp." << std::endl;
        return 1;
    }

    std::cout << "\nApp running. Waiting for DDS recommendations..." << std::endl;
    std::cout << "Press ENTER to exit.\n" << std::endl;
    std::cin.ignore();

    app.cleanUp();
    std::cout << "DDS Recommendation App exiting." << std::endl;
    return 0;
}
