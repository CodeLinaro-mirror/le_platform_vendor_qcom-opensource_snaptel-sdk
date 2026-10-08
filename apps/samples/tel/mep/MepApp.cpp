/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file MepApp.cpp
 *
 * @brief This application demonstrates how to use the MEP (Multiple Enabled Profiles) APIs
 *        to manage SIM cards that support multiple enabled profiles on a single physical slot.
 *
 * The steps are as follows:
 *
 * 1. Get a PhoneFactory instance.
 * 2. Get an IMultiSimManager instance and wait for it to be ready.
 * 3. Get an ICardManager instance and wait for it to be ready.
 * 4. Register a listener for slot status change notifications (onSlotStatusChanged).
 * 5. Get physical slot status (getPhysicalSlotStatus) to check MEP capability.
 * 6. Get logical slot mapping (getLogicalSlotMapping) to retrieve current mapping.
 * 7. Get MEP information from ICard (getMepInfo) for each logical slot.
 * 8. Configure logical slot mapping (configureLogicalSlotMapping) for MEP A1 card.
 *
 * Usage:
 * # ./mep_app
 */

#include <condition_variable>
#include <future>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <telux/common/CommonDefines.hpp>
#include <telux/tel/CardDefines.hpp>
#include <telux/tel/CardManager.hpp>
#include <telux/tel/MultiSimDefines.hpp>
#include <telux/tel/MultiSimManager.hpp>
#include <telux/tel/PhoneFactory.hpp>

// ============================================================================
// Helper functions to convert enums to human-readable strings
// ============================================================================

/**
 * Converts SlotState enum to a human-readable string.
 */
static std::string slotStateToString(telux::tel::SlotState state) {
    switch (state) {
        case telux::tel::SlotState::ACTIVE:
            return "ACTIVE";
        case telux::tel::SlotState::INACTIVE:
            return "INACTIVE";
        default:
            return "UNKNOWN";
    }
}

/**
 * Converts CardState enum to a human-readable string.
 */
static std::string cardStateToString(telux::tel::CardState state) {
    switch (state) {
        case telux::tel::CardState::CARDSTATE_ABSENT:
            return "ABSENT";
        case telux::tel::CardState::CARDSTATE_PRESENT:
            return "PRESENT";
        case telux::tel::CardState::CARDSTATE_ERROR:
            return "ERROR";
        case telux::tel::CardState::CARDSTATE_RESTRICTED:
            return "RESTRICTED";
        default:
            return "UNKNOWN";
    }
}

/**
 * Converts PortState enum to a human-readable string.
 */
static std::string portStateToString(telux::tel::PortState state) {
    switch (state) {
        case telux::tel::PortState::ACTIVE:
            return "ACTIVE";
        case telux::tel::PortState::INACTIVE:
            return "INACTIVE";
        default:
            return "UNKNOWN";
    }
}

/**
 * Converts MEP Mode enum to a human-readable string.
 */
static std::string mepModeToString(telux::tel::Mode mode) {
    switch (mode) {
        case telux::tel::Mode::NONE:
            return "NONE (SEP - Single Enabled Profile)";
        case telux::tel::Mode::MEP_A1:
            return "MEP_A1 (port index starts from 1)";
        case telux::tel::Mode::MEP_B:
            return "MEP_B (port index starts from 0)";
        default:
            return "UNKNOWN";
    }
}

// ============================================================================
// Helper function to print physical slot status details
// ============================================================================

/**
 * Prints the physical slot status map in a formatted manner.
 *
 * @param [in] slotStatus  Map of physical slot IDs to their SimSlotStatus.
 */
static void printPhysicalSlotStatus(
    const std::map<PhysicalSlotId, telux::tel::SimSlotStatus> &slotStatus) {
    if (slotStatus.empty()) {
        std::cout << "  No physical slot status entries found." << std::endl;
        return;
    }

    for (const auto &entry : slotStatus) {
        PhysicalSlotId physSlotId               = entry.first;
        const telux::tel::SimSlotStatus &status = entry.second;

        std::cout << "  Physical SlotId : " << static_cast<int>(physSlotId) << std::endl;
        std::cout << "    SlotState     : " << slotStateToString(status.slotState) << std::endl;
        std::cout << "    CardState     : " << cardStateToString(status.cardState) << std::endl;
        std::cout << "    IsMEP         : " << (status.isMep ? "true" : "false") << std::endl;
        std::cout << "    MEP Mode      : " << mepModeToString(status.mepSlotInfo.mode)
                  << std::endl;

        if (status.isMep) {
            const auto &ports = status.mepSlotInfo.port;
            if (!ports.empty()) {
                std::cout << "    MEP Port Details:" << std::endl;
                for (const telux::tel::PortInfo &port : ports) {
                    std::cout << "      LogicalSlotId : " << static_cast<int>(port.slotId)
                              << std::endl;
                    std::cout
                        << "      ICCID         : " << (port.iccId.empty() ? "<none>" : port.iccId)
                        << std::endl;
                    std::cout << "      PortState     : " << portStateToString(port.state)
                              << std::endl;
                }
            } else {
                std::cout << "    No MEP port details available." << std::endl;
            }
        } else {
            std::cout << "    MEP is not active on this physical slot." << std::endl;
        }
    }
}

// ============================================================================
// MepSlotStatusListener
//
// Implements IMultiSimListener to receive onSlotStatusChanged notifications.
//
// This indication is triggered when:
//   - configureLogicalSlotMapping is called to remap logical to physical slots.
//   - A MEP profile is enabled or disabled via APDU on a logical slot.
//
// Application must monitor @ref telux::tel::SimSlotStatus to determine when the transition is
// complete, as the transition may take a few seconds.
// ============================================================================

class MepSlotStatusListener : public telux::tel::IMultiSimListener {
 public:
    /**
     * Called whenever there is a change in physical SIM slot status for a
     * MEP or Non-MEP SIM card.
     *
     * @param [in] slotStatus  Map of physical slot IDs to their SimSlotStatus.
     */
    void onSlotStatusChanged(
        std::map<PhysicalSlotId, telux::tel::SimSlotStatus> slotStatus) override {
        std::cout << "\n[NOTIFICATION] onSlotStatusChanged received:" << std::endl;
        printPhysicalSlotStatus(slotStatus);
        std::cout << std::endl;
    }

    virtual ~MepSlotStatusListener() = default;
};

// ============================================================================
// main
// ============================================================================

int main(int argc, char *argv[]) {
    std::cout << "============================================================" << std::endl;
    std::cout << "  MEP (Multiple Enabled Profiles) Sample Application" << std::endl;
    std::cout << "============================================================" << std::endl;

    // -------------------------------------------------------------------------
    // Step 1: Get PhoneFactory instance
    // PhoneFactory is the central factory to create all Telephony SDK services.
    // -------------------------------------------------------------------------
    auto &phoneFactory = telux::tel::PhoneFactory::getInstance();

    // -------------------------------------------------------------------------
    // Step 2: Get IMultiSimManager and wait for it to be ready.
    // IMultiSimManager provides APIs for multi-SIM operations including
    // logical slot mapping and physical slot status queries.
    // -------------------------------------------------------------------------
    std::cout << "\n Initializing MultiSimManager..." << std::endl;

    std::promise<telux::common::ServiceStatus> multiSimProm;
    auto multiSimMgr = phoneFactory.getMultiSimManager(
        [&multiSimProm](telux::common::ServiceStatus status) { multiSimProm.set_value(status); });

    if (!multiSimMgr) {
        std::cout << "ERROR: Failed to get IMultiSimManager instance." << std::endl;
        return -1;
    }

    telux::common::ServiceStatus multiSimStatus = multiSimProm.get_future().get();
    if (multiSimStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << "ERROR: MultiSimManager service unavailable, status: "
                  << static_cast<int>(multiSimStatus) << std::endl;
        return -1;
    }
    std::cout << "  MultiSimManager is ready." << std::endl;

    // -------------------------------------------------------------------------
    // Step 3: Get ICardManager and wait for it to be ready.
    // ICardManager provides access to ICard objects for each logical slot,
    // which expose getMepInfo() to query MEP details per slot.
    // -------------------------------------------------------------------------
    std::cout << "\n Initializing CardManager..." << std::endl;

    std::promise<telux::common::ServiceStatus> cardMgrProm;
    auto cardMgr = phoneFactory.getCardManager(
        [&cardMgrProm](telux::common::ServiceStatus status) { cardMgrProm.set_value(status); });

    if (!cardMgr) {
        std::cout << "ERROR: Failed to get ICardManager instance." << std::endl;
        return -1;
    }

    telux::common::ServiceStatus cardMgrStatus = cardMgrProm.get_future().get();
    if (cardMgrStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::cout
            << "ERROR: CardManager service unavailable, status: " << static_cast<int>(cardMgrStatus)
            << std::endl;
        return -1;
    }
    std::cout << "  CardManager is ready." << std::endl;

    // -------------------------------------------------------------------------
    // Step 4: Register listener for slot status change notifications.
    //
    // IMultiSimListener::onSlotStatusChanged is invoked when:
    //   1. configureLogicalSlotMapping is called.
    //   2. A MEP profile is enabled or disabled via APDU on a logical slot.
    //
    // -------------------------------------------------------------------------
    std::cout << "\n Registering MultiSim listener for slot status notifications..." << std::endl;

    auto listener                   = std::make_shared<MepSlotStatusListener>();
    telux::common::Status regStatus = multiSimMgr->registerListener(listener);
    if (regStatus != telux::common::Status::SUCCESS) {
        std::cout << "  WARNING: Failed to register MultiSim listener, status: "
                  << static_cast<int>(regStatus) << std::endl;
    } else {
        std::cout << "  MultiSim listener registered successfully." << std::endl;
    }

    // -------------------------------------------------------------------------
    // Step 5: Get physical slot status.
    //
    // getPhysicalSlotStatus retrieves the status of all physical SIM slots,
    // including:
    //   - Whether MEP is active on the slot (isMep)
    //   - The MEP mode (NONE/MEP_A1/MEP_B)
    //   - Port details: logical slot ID, ICCID, and port state for each port
    //
    // This is useful to determine the current MEP configuration before
    // making any changes via configureLogicalSlotMapping.
    // -------------------------------------------------------------------------
    std::cout << "\n Getting physical slot status (getPhysicalSlotStatus)..." << std::endl;

    std::map<PhysicalSlotId, telux::tel::SimSlotStatus> slotStatus;
    telux::common::ErrorCode errCode = multiSimMgr->getPhysicalSlotStatus(slotStatus);
    if (errCode == telux::common::ErrorCode::SUCCESS) {
        printPhysicalSlotStatus(slotStatus);
    } else {
        std::cout
            << "  ERROR: getPhysicalSlotStatus failed with ErrorCode: " << static_cast<int>(errCode)
            << std::endl;
    }

    // -------------------------------------------------------------------------
    // Step 6: Get logical slot mapping.
    //
    // getLogicalSlotMapping retrieves the current mapping of each logical slot
    // to its corresponding physical slot and port identifier.
    //
    // LogicalSlotId  -> { PhysicalSlotId, PortId }
    //
    // For a standard dual-SIM (DSDA) device with two physical slots:
    //   SLOT_ID_1 -> { SLOT_ID_1, 0 }
    //   SLOT_ID_2 -> { SLOT_ID_2, 0 }
    //
    // For a MEP A1 card with two profiles on physical slot 1:
    //   SLOT_ID_1 -> { SLOT_ID_1, 1 }
    //   SLOT_ID_2 -> { SLOT_ID_1, 2 }
    // -------------------------------------------------------------------------
    std::cout << "\n Getting logical slot mapping (getLogicalSlotMapping)..." << std::endl;

    std::map<LogicalSlotId, telux::tel::LogicalSlotMapInfo> mapInfo;
    errCode = multiSimMgr->getLogicalSlotMapping(mapInfo);
    if (errCode == telux::common::ErrorCode::SUCCESS) {
        if (mapInfo.empty()) {
            std::cout << "  Logical slot mapping is empty." << std::endl;
        } else {
            for (const auto &entry : mapInfo) {
                LogicalSlotId logicalId                    = entry.first;
                const telux::tel::LogicalSlotMapInfo &info = entry.second;
                std::cout << "  LogicalSlotId " << static_cast<int>(logicalId)
                          << " -> PhysicalSlotId: " << static_cast<int>(info.physicalSlot)
                          << ", PortId: " << info.portId << std::endl;
            }
        }
    } else {
        std::cout
            << "  ERROR: getLogicalSlotMapping failed with ErrorCode: " << static_cast<int>(errCode)
            << std::endl;
    }

    // -------------------------------------------------------------------------
    // Step 7: Get MEP information from ICard for each logical slot.
    //
    // getMepInfo provides per-logical-slot MEP information:
    //   - isMep:             Whether MEP is enabled on this logical slot.
    //   - portId:            Port identifier of the card on this logical slot.
    //                        SEP (Mode::NONE): portId = 0
    //                        MEP_A1:           portId starts from 1
    //                        MEP_B:            portId starts from 0
    //   - negotiatedMepMode: The MEP mode negotiated for the card.
    //
    // The portId from getMepInfo is required when exchanging APDUs in MEP A1
    // mode to enable or disable a profile on a specific port.
    // -------------------------------------------------------------------------
    std::cout << "\n Getting MEP information from ICard (getMepInfo)..." << std::endl;

    int slotCount                = 0;
    telux::common::Status status = cardMgr->getSlotCount(slotCount);
    if (status == telux::common::Status::SUCCESS && slotCount > 0) {
        std::cout << "  Slot count: " << slotCount << std::endl;
        for (int slotId = 1; slotId <= slotCount; ++slotId) {
            telux::common::Status cardStatus;
            auto card = cardMgr->getCard(slotId, &cardStatus);
            if (card && cardStatus == telux::common::Status::SUCCESS) {
                telux::tel::MepInfo mepInfo;
                card->getMepInfo(mepInfo);
                std::cout << "  Logical SlotId " << slotId << ":" << std::endl;
                std::cout << "    isMep             : " << (mepInfo.isMep ? "true" : "false")
                          << std::endl;
                std::cout << "    portId            : " << mepInfo.portId << std::endl;
                std::cout
                    << "    negotiatedMepMode : " << mepModeToString(mepInfo.negotiatedMepMode)
                    << std::endl;
            } else {
                std::cout << "  WARNING: Could not get card for slotId " << slotId
                          << ", status: " << static_cast<int>(cardStatus) << std::endl;
            }
        }
    } else {
        std::cout << "  ERROR: Failed to get slot count, status: " << static_cast<int>(status)
                  << std::endl;
    }

    // -------------------------------------------------------------------------
    // Step 8: Configure logical slot mapping.
    //
    // configureLogicalSlotMapping maps logical modem stacks to physical SIM
    // slots and port identifiers. This is used to:
    //   a) Configure a MEP card with two enabled profiles on one physical slot.
    //   b) Configure a SEP card and a MEP card with one enabled profile each.
    //
    // This example configures a MEP A1 card with two enabled profiles on
    // physical slot 1 (dual baseband, DSDA configuration):
    //
    //   LogicalSlotId::SLOT_ID_1 -> PhysicalSlotId::SLOT_ID_1, PortId: 1
    //   LogicalSlotId::SLOT_ID_2 -> PhysicalSlotId::SLOT_ID_1, PortId: 2
    //
    // Port index rules:
    //   MEP_A1: port identifiers start from 1 (e.g., 1, 2, ...)
    //   MEP_B:  port identifiers start from 0 (e.g., 0, 1, ...)
    //
    // Prerequisites:
    //   - Device must be configured with MULTISIM_CONFIG=dsda or dsds
    //     in /etc/tel.conf.
    //   - Existing profiles are NOT deleted; they remain linked to their ports.
    //
    // The configuration persists across device reboot.
    // Monitor onSlotStatusChanged to determine when the transition completes,
    // as it may take a few seconds.
    // -------------------------------------------------------------------------
    std::cout << "\n Configuring logical slot mapping (configureLogicalSlotMapping)..."
              << std::endl;
    std::cout << "  Enter the number of logical slot mappings to configure: ";

    int numMappings = 0;
    while (!(std::cin >> numMappings) || numMappings <= 0) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "  Invalid input. Please enter a positive integer: ";
    }

    std::map<LogicalSlotId, telux::tel::LogicalSlotMapInfo> configMap;

    for (int i = 0; i < numMappings; ++i) {
        std::cout << "\n  Mapping " << (i + 1) << " of " << numMappings << ":" << std::endl;

        int logicalSlotId = 0;
        std::cout << "    Enter LogicalSlotId (e.g., 1 for SLOT_ID_1, 2 for SLOT_ID_2): ";
        while (!(std::cin >> logicalSlotId) || logicalSlotId <= 0) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "    Invalid input. Please enter a positive integer: ";
        }

        int physicalSlotId = 0;
        std::cout << "    Enter PhysicalSlotId (e.g., 1 for SLOT_ID_1, 2 for SLOT_ID_2): ";
        while (!(std::cin >> physicalSlotId) || physicalSlotId <= 0) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "    Invalid input. Please enter a positive integer: ";
        }

        int portId = 0;
        std::cout << "    Enter PortId (MEP_A1: starts from 1; MEP_B: starts from 0): ";
        while (!(std::cin >> portId) || portId < 0) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "    Invalid input. Please enter a non-negative integer: ";
        }

        telux::tel::LogicalSlotMapInfo slotInfo;
        slotInfo.physicalSlot = static_cast<PhysicalSlotId>(physicalSlotId);
        slotInfo.portId       = portId;
        configMap[static_cast<LogicalSlotId>(logicalSlotId)] = slotInfo;

        std::cout << "    Mapped LogicalSlotId " << logicalSlotId << " -> PhysicalSlotId "
                  << physicalSlotId << ", PortId " << portId << std::endl;
    }

    std::cout << "\n  Summary of logical slot mapping to be configured:" << std::endl;
    for (const auto &entry : configMap) {
        std::cout << "    LogicalSlotId " << static_cast<int>(entry.first) << " -> PhysicalSlotId "
                  << static_cast<int>(entry.second.physicalSlot) << ", PortId "
                  << entry.second.portId << std::endl;
    }

    /*
     * Synchronization primitives used to block main() until the
     * configureLogicalSlotMapping response callback is invoked.
     */
    std::mutex callbackMutex;
    std::condition_variable callbackCv;
    bool callbackReceived                  = false;
    telux::common::ErrorCode callbackError = telux::common::ErrorCode::SUCCESS;

    telux::common::Status configStatus = multiSimMgr->configureLogicalSlotMapping(
        configMap, [&callbackMutex, &callbackCv, &callbackReceived, &callbackError](
                       telux::common::ErrorCode error) {
            std::cout << "\n configureLogicalSlotMapping response: "
                      << (error == telux::common::ErrorCode::SUCCESS ? "SUCCESS" : "FAILED")
                      << " (ErrorCode: " << static_cast<int>(error) << ")" << std::endl;

            if (error == telux::common::ErrorCode::SUCCESS) {
                std::cout << "  Logical slot mapping request accepted by modem." << std::endl;
                std::cout << "  Monitor onSlotStatusChanged notification for transition completion."
                          << std::endl;
            } else {
                std::cout << "  Logical slot mapping configuration failed." << std::endl;
            }

            /* Notify main() that the callback has been received */
            {
                std::lock_guard<std::mutex> lock(callbackMutex);
                callbackError    = error;
                callbackReceived = true;
            }
            callbackCv.notify_one();
        });

    if (configStatus == telux::common::Status::SUCCESS) {
        std::cout << "  configureLogicalSlotMapping request sent successfully." << std::endl;
        std::cout << "  Blocking until configureLogicalSlotMapping callback is received..."
                  << std::endl;

        /* Block until the response callback sets callbackReceived = true */
        std::unique_lock<std::mutex> lock(callbackMutex);
        callbackCv.wait(lock, [&callbackReceived] { return callbackReceived; });

        std::cout << " Callback received with ErrorCode: " << static_cast<int>(callbackError)
                  << std::endl;
    } else {
        std::cout << "  ERROR: configureLogicalSlotMapping request failed with status: "
                  << static_cast<int>(configStatus) << std::endl;
    }

    // -------------------------------------------------------------------------
    // Cleanup: Deregister listener
    // -------------------------------------------------------------------------
    std::cout << "\n Deregistering MultiSim listener..." << std::endl;
    multiSimMgr->deregisterListener(listener);
    std::cout << "  Listener deregistered." << std::endl;

    std::cout << "\n============================================================" << std::endl;
    std::cout << "  MEP Sample Application completed." << std::endl;
    std::cout << "============================================================" << std::endl;
    return 0;
}
