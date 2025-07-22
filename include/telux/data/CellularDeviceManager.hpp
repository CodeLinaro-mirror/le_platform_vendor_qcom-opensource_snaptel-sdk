/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       CellularDeviceManager.hpp
 *
 * @brief      The CellularDeviceManager class provides APIs
 */

#ifndef TELUX_DATA_CELLULAR_DEVICE_MANAGER_HPP
#define TELUX_DATA_CELLULAR_DEVICE_MANAGER_HPP


#include <cstdint>
#include <string>
#include <unordered_map>
#include <memory>
#include <vector>

#include <telux/common/CommonDefines.hpp>
#include <telux/data/DataDefines.hpp>

namespace telux {
namespace data {

/** Maximum length of string (in bytes). */
#define METRIC_INFO_STR_SIZE      100

/** Maximum array size (in bytes). */
#define METRIC_INFO_ARRAY_SIZE    20

enum class CellularDeviceEventType {
    INVALID = 0x00,               /**< Invalid event. */
    NO_SERVICE,                   /**< No service registration status event. */
    POWER_SAVE,                   /**< Power save registration status event. */
    LIMITED_SERVICE,              /**< Limited service registration status event. */
    LIMITED_REGIONAL_SERVICE,     /**< Limited regional service registration status event. */
    LTE_SERVICE_AVAILABLE,        /**< LTE service available registration status event. */
    NR_5G_NSA_SERVICE_AVAILABLE,  /**< NR5G non-standalone (NAS) service available registration status event. */
    NR_5G_SA_SERVICE_AVAILABLE,   /**< NR5G standalone service available registration status event. */
    MODEM_DOWN,                   /**< Modem down registration status event. */
    MODEM_UP,                     /**< Modem up registration status event.  */
    MAX
};

enum class CellularDeviceGetRequest {
    INVALID = 0x00,                         /**< Invalid request. */
    REGISTRATION_STATUS_SYNC,           /**< Registration status sync request. */
    LTE_SERVING_CELL_EARFCN_SYNC,       /**< LTE serving cell E-UTRA Absolute Radio Frequency Channel Number (EARFCN) sync request. */
    NR5G_SERVING_CELL_EARFCN_SYNC,      /**< NR5G serving cell E-UTRA Absolute Radio Frequency Channel Number (EARFCN) sync request. */
    LTE_SERVING_CELL_PCI_SYNC,          /**< LTE serving cell Physical Cell Identifier (PCI) sync request. */
    NR5G_SERVING_CELL_PCI_SYNC,         /**< NR5G serving cell Physical Cell Identifier (PCI) sync request. */
    LTE_SERVING_CELL_CGI_SYNC,          /**< LTE serving cell Cell Global Identity (CGI) sync request. */
    NR5G_SERVING_CELL_CGI_SYNC,         /**< NR5G serving cell Cell Global Identity (CGI) sync request. */
    LTE_SERVING_CELL_PLMN_SYNC,         /**< LTE serving cell Public Land Mobile Network (PLMN) sync request. */
    NR5G_SERVING_CELL_PLMN_SYNC,        /**< NR5G serving cell Public Land Mobile Network (PLMN) sync request. */
    LTE_SERVING_CELL_RSRP_SYNC,         /**< LTE serving cell Reference Signal Received Power (RSRP) sync request. */
    NR5G_SERVING_CELL_RSRP_SYNC,        /**< NR5G serving cell Reference Signal Received Power (RSRP) sync request. */
    LTE_SERVING_CELL_RSRQ_SYNC,         /**< LTE serving cell Reference Signal Received Quality (RSRQ) sync request. */
    NR5G_SERVING_CELL_RSRQ_SYNC,        /**< NR5G serving cell Reference Signal Received Quality (RSRQ) sync request. */
    LTE_SERVING_CELL_SINR_SYNC,         /**< LTE serving cell Signal Interference to Noise Ratio (SINR) sync request. */
    NR5G_SERVING_CELL_SINR_SYNC,        /**< NR5G serving cell Signal Interference to Noise Ratio (SINR) sync request. */
    LTE_SERVING_CELL_CQI_SYNC,          /**< LTE serving cell Channel Quality Indicator (CQI) sync request. */
    NR5G_SERVING_CELL_CQI_SYNC,         /**< NR5G serving cell Channel Quality Indicator (CQI) sync request. */
    LTE_SERVING_CELL_TAC_SYNC,          /**< LTE serving cell Tracking Area Code (TAC) sync request. */
    NR5G_SERVING_CELL_TAC_SYNC,         /**< NR5G serving cell Tracking Area Code (TAC) sync request. */
    LTE_SERVING_CELL_TX_POWER_SYNC,     /**< LTE serving cell Tx Power sync request. */
    NR5G_SERVING_CELL_TX_POWER_SYNC,    /**< NR5G serving cell Tx Power sync request. */
    LTE_NEIGHBOR_CELLS_EARFCN_ASYNC,    /**< LTE neighbor cells E-UTRA Absolute Radio Frequency Channel Number (EARFCN) async request. */
    NR5G_NEIGHBOR_CELLS_EARFCN_ASYNC,   /**< NR5G neighbor cells E-UTRA Absolute Radio Frequency Channel Number (EARFCN) async request. */
    LTE_NEIGHBOR_CELLS_PCI_ASYNC,       /**< LTE neighbor cells Physical Cell Identifier (PCI) async request. */
    NR5G_NEIGHBOR_CELLS_PCI_ASYNC,      /**< NR5G neighbor cells Physical Cell Identifier (PCI) async request. */
    LTE_NEIGHBOR_CELLS_CGI_ASYNC,       /**< LTE neighbor cells Cell Global Identity (CGI) async request. */
    NR5G_NEIGHBOR_CELLS_CGI_ASYNC,      /**< NR5G neighbor cells Cell Global Identity (CGI) async request. */
    LTE_NEIGHBOR_CELLS_PLMN_ASYNC,      /**< LTE neighbor cells Public Land Mobile Network (PLMN) async request. */
    NR5G_NEIGHBOR_CELLS_PLMN_ASYNC,     /**< NR5G neighbor cells Public Land Mobile Network (PLMN) async request. */
    LTE_NEIGHBOR_CELLS_RSRP_ASYNC,      /**< LTE neighbor cells Reference Signal Received Power (RSRP) async request. */
    NR5G_NEIGHBOR_CELLS_RSRP_ASYNC,     /**< NR5G neighbor cells Reference Signal Received Power (RSRP) async request. */
    LTE_NEIGHBOR_CELLS_RSRQ_ASYNC,      /**< LTE neighbor cells Reference Signal Received Quality (RSRQ) async request. */
    LTE_CELL_LOCK_LIST_SYNC,            /**< LTE cell lock list sync request. */
    NR5G_CELL_LOCK_LIST_SYNC,           /**< NR5G cell lock list sync request. */
    MCS_LTE_TX_RX_INFO_SYNC,            /**< LTE Modulation and Coding Scheme (MCS) sync request. */
    MCS_NR5G_TX_RX_INFO_SYNC,           /**< NR5G Modulation and Coding Scheme (MCS) sync request. */
    LTE_CPHY_CA_INFO_SYNC,              /**< LTE physical Carrier Aggregation (CA) sync request. */
    LTE_ENDC_MODE_INFO_SYNC,            /**< LTE NSA EN-DC Non-Standalone Eutra 5GNR Dual Connectivity (ENDC) mode sync request. */
    LTE_RRC_STATUS_SYNC,                /**< LTE Radio Resource Control (RRC) state sync request. */
    NR5G_RRC_STATUS_SYNC,               /**< NR5G Radio Resource Control (RRC) state sync request. */
    NR5G_SUB_CARRIER_SPACING_INFO_SYNC, /**< NR5G Sub Carrier Spacing (SCS) sync request. */
    RF_BANDWIDTH_LTE_INFO_SYNC,         /**< LTE RF bandwidth sync request. */
    RF_BANDWIDTH_NR5G_INFO_SYNC,        /**< NR5G RF bandwidth sync request. */
    MAX
};

enum class CellularDeviceSetRequest {
    INVALID = 0x00,                     /**< Invalid configuration request */
    LTE_CELL_LOCK_LIST_SYNC,           /**< LTE cell lock list sync configuration request. */
    NR5G_CELL_LOCK_LIST_SYNC,           /**< NR5G cell lock list sync configuration request. */
    DEPRIORITIZE_NR5G_SYNC,             /**< NR5G deprioritization sync configuration request. */
    MMW_PANEL_POSITION_CONFIG_SYNC,     /**< mmWave panel position configuration sync request. */
    MMW_PANEL_POSITION_UPDATE,          /**< mmWave panel position update sync request. */
    INSTALL_MODE_SYNC,                  /**< Install mode configuration sync request. */
    OOS_SCAN_SYNC,                      /**< Out of service scan sync request  */
    MAX
};

/** Asynchronous indication type.
 */
enum class CellularDeviceIndication {
    INVALID = 0x00,                    /**< Invalid indication. */
    MODEM_STATUS,                      /**< Modem status indication. */
    REGISTRATION_STATUS,               /**< Registration status indication. */
    LTE_SERVING_CELL_RSRP,             /**< LTE serving cell RSRP info indication. */
    NR5G_SERVING_CELL_RSRP,            /**< NR5G serving cell RSRP info indication. */
    LTE_SERVING_CELL_RSRQ,             /**< LTE serving cell RSRQ info indication. */
    NR5G_SERVING_CELL_RSRQ,            /**< NR5G serving cell RSRQ info indication. */
    LTE_SERVING_CELL_SINR,             /**< LTE serving cell SINR info indication. */
    NR5G_SERVING_CELL_SINR,            /**< NR5G serving cell SINR info indication. */
    MMW_PANEL_POSITION_STATUS,         /**< mmWave panel position status indication. */
    INSTALL_MODE_STATUS,               /**< Safe install status mode indication. */
    INSTALL_MEASUREMENT,               /**< Safe install measurement indication. */
    MAX
};

/** Asynchronous indication type.
 */
enum class CellularDeviceAsyncResponse {
    INVALID = 0x00,              /**< Invalid async response. */
    LTE_NEIGHBOR_INFO,           /**< LTE neighbor information async response. */
    NR5G_NEIGHBOR_INFO,          /**< NR5G neighbor information async response. */
    MAX
};

/** Metric information type.
 */
enum class CellularDeviceInfoType {
    INVALID = 0x00,                         /**< Invalid type. */
    BOOLEAN,                                /**< Boolean type. */
    INTEGER,                                /**< Integer type. */
    FLOAT,                                  /**< Float type. */
    EVENT,                                  /**< General event type. */
    BUFFER,                                 /**< Buffer type. */
    ARRAY,                                  /**< Array type. */
    CQI_EXTENSION,                          /**< Channel Quality Indicator (CQI) extension type. */
    MULTICELL_LOCK_EXTENSION,               /**< Multicell lock extension type. */
    LTE_NEIGHBOR_INFO_EXTENSION,            /**< LTE neighbor information extension type. */
    NR5G_NEIGHBOR_INFO_EXTENSION,           /**< NR5G neighbor information extension type. */
    INSTALL_MODE_CONFIG_EXTENSION,          /**< Install mode measurement configuration extension type.  */
    INSTALL_MODE_INFO_EXTENSION,            /**< Install mode measurements information extension type.  */
    PANEL_POSITION_CONFIG_EXTENSION,        /**< Panel position configuration extension type.  */
    PANEL_POSITION_UPDATE_CONFIG_EXTENSION, /**< Panel position update configuration extension type.  */
    PANEL_POSITION_STATUS_INFO_EXTENSION,   /**< Panel position status information extension type.  */
    LTE_CPHY_CA_INFO_EXTENSION,             /**< LTE physical carrier aggregation extension type.  */
    ENDC_INFO_EXTENSION,                    /**< ENDC information extension type.  */
    MCS_INFO_EXTENSION,                     /**< Modulation coding scheme extension.  */
    MAX
};

/** Metric identifier type.
 */
enum class CellularDeviceInfoIdType {
    INVALID = 0x00,    /**< Invalid identifier type. */
    REQUEST,           /**< Request identifier type. */
    ASYNC_RESPONSE,    /**< Asynchronous identifier type. */
    INDICATION,        /**< Indication identifier type. */
    MAX
};

/** Subscription type.
 */
enum class CellularDeviceSubscriptionType {
    INVALID = 0,    /**< Invalid subscription type. */
    DEFAULT = 1,    /**< Default subscription. */
    PRIMARY = 2,    /**< Primary subscription.*/
    SECONDARY = 3,  /**< Secondary subscription. */
};

/** NR5G cell configuration type.
 */
enum class CellularDeviceNr5gCellConfigType {
    INVALID = 0x00,     /**< Invalid configuration. **/
    PCI = 0x01,         /**< Configure cell identity for service acquisition.  */
    ARFCN = 0x02,       /**< Configure ARFCN list for service acquisition.  */
    UNLOCK = 0x03,      /**< Disable limiting service acquisition to one cell. */
};

/** Subcarrier space (SCS) type.
 */
enum class CellularDeviceNr5gSCSType {
    TYPE_INVALID = 0,       /**< Invalid SCS type. **/
    TYPE_15 = 1,            /**< NR5G subcarrier spacing 15 KHz. */
    TYPE_30 = 2,            /**< NR5G subcarrier spacing 30 KHz. */
    TYPE_60 = 3,            /**< NR5G subcarrier spacing 60 KHz. */
    TYPE_120 = 4,           /**< NR5G subcarrier spacing 120 KHz. */
    TYPE_240 = 5,           /**< NR5G subcarrier spacing 240 KHz. */
};

/** 5G QoS flow.
 */
enum class CellularDeviceQoSFlow5gQCI {
    QCI_INVALID = 0,
    QCI_0 = 1,
    QCI_1 = 2,
    QCI_2 = 3,
    QCI_3 = 4,
    QCI_4 = 5,
    QCI_5 = 6,
    QCI_6 = 7,
    QCI_7 = 8,
    QCI_8 = 9,
    QCI_9 = 10,
    QCI_65 = 65,
    QCI_66 = 66,
    QCI_67 = 67,
    QCI_69 = 69,
    QCI_70 = 70,
    QCI_75 = 75,
    QCI_79 = 79,
    QCI_80 = 80,
    QCI_82 = 82,
    QCI_83 = 83,
    QCI_84 = 84,
    QCI_85 = 85
};

/** Panel position operation mode.
 */
enum class CellularDevicePanelPositionMode {
    INVALID = 0,      /**< Invalid PPS mode. */
    START = 1,        /**< Panel position mode start. */
    MEASUREMENT = 2,  /**< Panel position mode measurement. */
    EXIT = 3,         /**< Panel position mode exit. */
};

/** NR5G install mode.
 */
enum class CellularDeviceNr5gInstallMode {
    INVALID = 0,  /**< Invalid install mode. */
    START = 1,    /**< Install mode start request. */
    STOP = 2,     /**< Install mode stop request.  */
};

/** Band ranges.
 */
enum class CellularDeviceARFCNFreqBandRange {
    RANGE_INVALID = 0,  /**< Invalid band selection. */
    RANGE_1_64    = 1,  /**< Band name value between 1-64 based on ARFCN. */
    RANGE_65_128  = 2,  /**< Band name value between 65-128 based on ARFCN. */
    RANGE_129_192 = 3,  /**< Band name value between 129-192 based on ARFCN. */
    RANGE_193_256 = 4,  /**< Band name value between 193-256 based on ARFCN. */
    RANGE_257_320 = 5,  /**< Band name value between 257-320 based on ARFCN. */
    RANGE_321_384 = 6,  /**< Band name value between 321-384 based on ARFCN. */
    RANGE_385_448 = 7,  /**< Band name value between 385-448 based on ARFCN. */
    RANGE_449_512 = 8,  /**< Band name value between 449-512 based on ARFCN. */
};

/** Panel position NR5G band.
 */
enum class CellularDeviceActiveNr5gBand {
    INVALID_BAND = 0,  /** Invalid NR5G Band **/
    BAND_257 = 257,    /** Active NR5G Band 257 **/
    BAND_258 = 258,    /** Active NR5G Band 258 **/
    BAND_259 = 259,    /** Active NR5G Band 259 **/
    BAND_260 = 260,    /** Active NR5G Band 260 **/
    BAND_261 = 261,    /** Active NR5G Band 261 **/
};

/** RF bandwidth.
 */
enum class CellularDeviceRFBandwidth {
    LTE_BANDWIDTH_INVALID = 0,       /**< Invalid bandwidth. */
    LTE_BANDWIDTH_NRB_6 = 1,         /**< LTE 1.4 MHz bandwidth. */
    LTE_BANDWIDTH_NRB_15 = 2,        /**< LTE 3 MHz bandwidth. */
    LTE_BANDWIDTH_NRB_25 = 3,        /**< LTE 5 MHz bandwidth. */
    LTE_BANDWIDTH_NRB_50 = 4,        /**< LTE 10 MHz bandwidth. */
    LTE_BANDWIDTH_NRB_75 = 5,        /**< LTE 15 MHz bandwidth. */
    LTE_BANDWIDTH_NRB_100 = 6,       /**< LTE 20 MHz bandwidth. */
    NR5G_BANDWIDTH_NRB_5 = 7,        /**< NR5G 5 MHz bandwidth. */
    NR5G_BANDWIDTH_NRB_10 = 8,       /**< NR5G 10 MHz bandwidth. */
    NR5G_BANDWIDTH_NRB_15 = 9,       /**< NR5G 15 MHz bandwidth. */
    NR5G_BANDWIDTH_NRB_20 = 10,      /**< NR5G 20 MHz bandwidth. */
    NR5G_BANDWIDTH_NRB_25 = 11,      /**< NR5G 25 MHz bandwidth. */
    NR5G_BANDWIDTH_NRB_30 = 12,      /**< NR5G 30 MHz bandwidth. */
    NR5G_BANDWIDTH_NRB_40 = 13,      /**< NR5G 40 MHz bandwidth. */
    NR5G_BANDWIDTH_NRB_50 = 14,      /**< NR5G 50 MHz bandwidth. */
    NR5G_BANDWIDTH_NRB_60 = 15,      /**< NR5G 60 MHz bandwidth. */
    NR5G_BANDWIDTH_NRB_80 = 16,      /**< NR5G 80 MHz bandwidth. */
    NR5G_BANDWIDTH_NRB_90 = 17,      /**< NR5G 90 MHz bandwidth. */
    NR5G_BANDWIDTH_NRB_100 = 18,     /**< NR5G 100 MHz bandwidth. */
    NR5G_BANDWIDTH_NRB_200 = 19,     /**< NR5G 200 MHz bandwidth. */
    NR5G_BANDWIDTH_NRB_400 = 20,     /**< NR5G 400 MHz bandwidth. */
    GSM_BANDWIDTH_NRB_2 = 21,        /**< GSM  0.2 MHz bandwidth. */
    TDSCDMA_BANDWIDTH_NRB_2 = 22,    /**< TDSCDMA 1.6 MHz bandwidth. */
    WCDMA_BANDWIDTH_NRB_5 = 23,      /**< WCDMA 5 MHz bandwidth. */
    WCDMA_BANDWIDTH_NRB_10 = 24,     /**< WCDMA 10 MHz bandwidth. */
};

/** Modulation and coding scheme type.
 */
enum class CellularDeviceModulationType {
    TYPE_INVALID = 0x00,         /**< Invalid modulation. */
    TYPE_BPSK = 0x01,            /**< Binary phase shift keying (BPSK). */
    TYPE_QPSK = 0x02,            /**< Quadrature phase shift keying (QPSK). */
    TYPE_16QAM = 0x03,           /**< 16-Quadrature Amplitude Modulation (16 QAM). */
    TYPE_64QAM = 0x04,           /**< 64-Quadrature Amplitude Modulation (64 QAM). */
    TYPE_256QAM = 0x05,          /**< 256-Quadrature Amplitude Modulation (256 QAM). */
    TYPE_1024QAM = 0x06,         /**< 1024-Quadrature Amplitude Modulation (1024 QAM). */
};

/** LTE physical carrier aggregation (CA) bandwidth type.
 */
enum class CellularDeviceLteCphyCABandwidth {
    INVALID = 0x00,          /**< Invalid bandwidth. */
    NRB_6 = 0x01,            /**< 1.4 MHz bandwidth. */
    NRB_15 = 0x02,           /**< 3 MHz bandwidth. */
    NRB_25 = 0x03,           /**< 5 MHz bandwidth. */
    NRB_50 = 0x04,           /**< 10 MHz bandwidth. */
    NRB_75 = 0x05,           /**< 15 MHz bandwidth. */
    NRB_100 = 0x06,          /**< 20 MHz bandwidth. */
};

/** RRC state for LTE and NR5G type.
 */
enum class CellularDeviceRRCState {
    RRC_STATE_INVALID = 0,                  /**< RRC ivalid state. */
    LTE_RRC_STATE_NULL = 1,                 /**< RRC null state. */
    LTE_RRC_STATE_IDLE_CAMPED_ANYCELL = 2,  /**< RRC idle camped on any cell. */
    LTE_RRC_STATE_IDLE_CAMPED_NORMAL = 3,   /**< RRC idle camped and normal. */
    LTE_RRC_STATE_CONNECTING = 4,           /**< RRC state connecting. */
    LTE_RRC_STATE_CONNECTED = 5,            /**< RRC state connected. */
    LTE_RRC_STATE_CLOSING = 6,              /**< RRC state releasing. */
    NR5G_RRC_STATE_IDLE_CAMPED = 7,         /**< RRC state idle camped. */
    NR5G_RRC_STATE_CONNECTED = 8,           /**< RRC state connected. */
    NR5G_RRC_STATE_INACTIVE_CAMPED = 9,     /**< RRC state inactive camped. */
};

/** LTE/NR5G Channel Quality indicator (CQI) entry.
 */
using CellularDeviceCQIEntry = std::vector<uint8_t>;

/** LTE/NR5G CQI information.
 */
using CellularDeviceCQIInfo = std::vector<CellularDeviceCQIEntry>;

/** NR5G band preference mask.
 */
using CellularDeviceNr5gBandPrefMaskType =
    std::unordered_map<CellularDeviceARFCNFreqBandRange, uint64_t>;

/**  NR5G frequency type.
 */
struct CellularDeviceNr5gFrequencyType {
    uint32_t arfcn;                 /* ARFCN value in KHz. */
    CellularDeviceNr5gSCSType scs;  /* Sub Carrier Spacing (SCS). */
};

/** NR5G cell identity information.
 */
struct CellularDeviceNr5gCellIdentityInfoType {
    uint16_t pci;                   /* Physical Cell ID. */
    CellularDeviceNr5gSCSType scs;  /* Sub Carrier Spacing (SCS). */
    uint32_t arfcn;                 /* ARFCN value in KHz. */
    CellularDeviceNr5gBandPrefMaskType band;    /* NR5G operating band. */
};

/** LTE cell identity information.
 */
struct CellularDeviceLteCellIdentityInfo {
    uint16_t pci;   /* Physical cell ID. */

    uint16_t freq;  /* Cell frequency. */
};

/** LTE cell lock configuration.
 */
struct CellularDeviceLteCellLockConfig {
    std::vector<CellularDeviceLteCellIdentityInfo> cellIdentity;
    /* Cell identity information. Max value is LTE_CELL_IDENTITY_MAX_ENTRIES */

    uint8_t enforce;    /* Enforces cell lock parameter without power cycle. */
};

/** NR5G cell identity information.
 */
struct CellularDeviceNr5gCellIdentityInfo {
    uint16_t pci;   /* Physical cell ID. */

    uint32_t arfcn; /* ARFCN of NR5G cell. */
};

/** NR5G cell lock configuration.
 */
struct CellularDeviceNr5gCellLockConfig {
    CellularDeviceNr5gCellConfigType type;   /* NR5G cell configuration type. */
    bool cellIdentityValid; /* Must be set to TRUE if cell_identity is being passed. */
    CellularDeviceNr5gCellIdentityInfoType cellIdentity;    /* Cell identity information. */
    std::vector<CellularDeviceNr5gFrequencyType> arfcnList;
    /**< ARFCN. Max value is NR5G_ARFCN_MAX_ENTRIES */
};

/** PLMN ID information.
 */
struct CellularDevicePlmnIdInfo {
    uint16_t mcc;
    /**< A 16 bit integer representation of the mobile country code (MCC).
         Range: 0 to 999. */

    uint16_t mnc;
    /**< A 16 bit integer representation of the mobile network code (MNC).
         Range: 0 to 999. */

    bool mncIncludesPcsDigit;
    /**< Interprets length of the corresponding MNC field\n
            - TRUE -- MNC is a three-digit value. \n For example, a reported value
                    of 90 corresponds to an MNC value of 090.\n
            - FALSE -- MNC is a two-digit value. \n For example, a reported value of
                    90 corresponds to an MNC value of 90. */
};

/** LTE neighbor cell information.
 */
struct CellularDeviceLteNeighborCellInfo {
    uint32_t freq;
    /**< Absolute Radio Frequency Channel Number (ARFCN). */

    uint16_t cellId;
    /**< Physical cell ID. */

    bool globalCellIdValid;
    /**< Must be TRUE if global_cell_id is being passed */

    uint32_t globalCellId;
    /**< Specific ID that can identify a cell globally. */

    std::vector<CellularDevicePlmnIdInfo> plmnId;
    /**< PLMN ID. Max value is LTE_NEIGHBOR_SCAN_MAX_NUM_PLMN */
};

/** LTE neighbor signal information.
 */
struct CellularDeviceLteNeighborSignalInfo {
    uint32_t freq;
    /**< Absolute Radio Frequency Channel Number. */

    uint16_t cellId;
    /**< Physical cell ID. */

    bool rsrpValid;
    /**< Must be set to TRUE if rsrp is being passed. */

    int16_t rsrp;
    /**< Reference Signal Received Power (RSRP) of the LTE neighbor cell. */

    bool rsrqValid;
    /**< Must be set to TRUE if rsrq is being passed. */

    int16_t rsrq;
    /**< Combined Reference Signal Received Quality (RSRQ) of the LTE neighbor cell. */
};

/** LTE neighbor information.
 */
struct CellularDeviceLteNeighborInfo {
    std::vector<CellularDeviceLteNeighborCellInfo> neighborCellInfo;
    /**< Neighbor cell information of top five cells sorted in the order of RSRP.
         Max value is LTE_NEIGHBOR_SCAN_LIST. */

    bool signalInfoValid;
    /**< Must be set to TRUE if signalInfo is being passed.*/

    CellularDeviceLteNeighborSignalInfo signalInfo;
    /**< Neighbor signal information of top cell only. */
};

/** NR5G neighbor cell information.
 */
struct CellularDeviceNr5gNeighborCellInfo {
    uint32_t freq;  /* ARFCN. */
    uint16_t phyCellId; /* Physical cell ID. */

    bool rsrpValid;
    /**< Must be TRUE if rsrp is being passed. */
    int16_t rsrp;
    /**< Combined RSRP of only the top cell or band. */

    bool rsrqValid;
    /**< Must be TRUE if rsrq is being passed. */
    int16_t rsrq;
    /**< Reserved - not supported. */

    std::vector<CellularDevicePlmnIdInfo> plmnId;
    /**< PLMN ID. Max len is NR5G_NEIGHBOR_SCAN_MAX_NUM_PLMN */

    std::vector<uint64_t> globalCellId;
    /**< Global cell ID. Max len is NR5G_NEIGHBOR_SCAN_MAX_NUM_PLMN */
};

/** NR5G neighbor information.
 * Max len is NR5G_NEIGHBOR_SCAN_LIST
 */
using CellularDeviceNr5gNeighborInfo = std::vector<CellularDeviceNr5gNeighborCellInfo>;

/** NSA EN-DC Non-Standalone Eutra 5GNR Dual Connectivity (ENDC).
 */
struct CellularDeviceEndcModeInfo {
    bool endcAvailable; /**< ENDC mode is available. */
    bool restrictDcnrAvailable; /**< Restricted DCNR available. */
};

/** LTE physical carrier aggregation.
 */
struct CellularDeviceLteCphyCaInfo {
    CellularDeviceLteCphyCABandwidth cphyCaDlBandwidth;
    /**< Physical carrier aggregation downlink bandwidth for Scell. */
    CellularDeviceLteCphyCABandwidth cphyPcellBandwidthInfo;
    /**< Pcell bandwidth info. */

    std::vector<CellularDeviceLteCphyCABandwidth> cphyScellBandwidthInfo;
    /**< Scell bandwidth list info. Max len is MAX_SCELL_LIST_LEN */
};

/** Modulation information.
 */
struct CellularDeviceModulationInfo {
    std::vector<CellularDeviceModulationType> dlModulation;
    /**< Downlink modulation. Max len is MCS_MODULATIONS_MAX */
    std::vector<CellularDeviceModulationType> ulModulation;
    /**< Uplink modulation. Max len is MCS_MODULATIONS_MAX */
};

/** NR5G panel position frequency information.
 */
struct CellularDeviceNr5gPanelPositionFreqInfo {
    uint32_t nr5gEarfcn;
    /**< NR5G EUTRAN Absolute RF channel number (EARFCN). */

    CellularDeviceActiveNr5gBand nr5gBand;
    /**< NR5G active band class. */

    CellularDeviceNr5gSCSType nr5gSubcarrierSpacing;
    /**< NR5G subcarrier spacing of P-Scell. */
};

/** Panel position configuration request to configure current panel position
 *  index and start panel position.
 */
struct CellularDevicePanelPositionConfig {
    uint8_t currentPanelPositionIndex;
    /**< Current panel position index of the external motor. */

    bool enablePanelPosition;
    /**< Start panel position selection. */

    std::vector<CellularDeviceNr5gPanelPositionFreqInfo> nr5gPanelPositionFreqInfo;
    /**< [Optional] NR5G frequency list to be used for measurement during panel position selection.
                    Modem would be using the cached frequencies if not passed.
                    Max len is MAX_NR5G_PPS_FREQ_LIST */
};

/** Update panel position configuration request.
 */
struct CellularDevicePanelPositionUpdateConfig {
    uint8_t panelPositionChanged;
    /**< Panel position changed to the requested position by the external motor. */

    uint8_t panelPositionIndex;
    /**< Panel position index to which the panel position change was requested.
        Same value received by the application in CellularDevicePanelPositionStatusInfo. */

    CellularDevicePanelPositionMode panelPositionMode;
    /**< Panel position selection mode.
        Same value received by the application in CellularDevicePanelPositionStatusInfo. */
};

/** Panel position status indication.
 */
struct CellularDevicePanelPositionStatusInfo {
    uint8_t panelPositionIndex;
    /**< Panel position index to which the panel position change was requested. */

    CellularDevicePanelPositionMode panelPositionMode;
    /**< Panel position selection mode. */
};

using CellularDeviceNr5gInstallModeFreqInfo =
    CellularDeviceNr5gPanelPositionFreqInfo;

/** Install mode configuration request information.
 */
struct CellularDeviceInstallModeConfig {
    CellularDeviceNr5gInstallMode installMode;
    /**< NR5G install mode. */

    std::vector<CellularDeviceNr5gInstallModeFreqInfo> freqInfo;
    /**< [Optional] NR5G frequency list to be used for measurement during CPE installation.
         Max len is MAX_NR5G_INSTALL_MODE_FREQ_LIST */
};

/** Install mode measurement entry.
 */
struct CellularDeviceInstallModeMeasurement {
    CellularDeviceNr5gInstallModeFreqInfo freqInfo;
    /**< NR5G install mode frequency information. */

    int16_t rsrp;
    /**< RSRP. */

    int16_t snr;
    /**< SNR. */

    std::vector<uint16_t> cellId;
/*! Cell ID corresponding to metrics reported
    Max len is MAX_NR5G_PPS_FREQ_LIST */
};

/** Install mode response information.
 * NR5G measurement information list during CPE installation.
 * Max len is MAX_NR5G_INSTALL_MODE_FREQ_LIST
 */
using CellularDeviceInstallModeInfo =
    std::vector<CellularDeviceInstallModeMeasurement>;

/** Generic structure to pass metric request information to the service from the application
 *  and get the metric response and indication information to the application.
 */
struct MetricInfo {
    CellularDeviceInfoIdType idType;        /* Metric information ID type. */
    uint32_t id;                            /* Metric information ID */
    CellularDeviceInfoType infoType;        /* Metric Value Info type */
    union {
        bool boolValue;                     /* Metric value as bool. */
        int64_t intValue;                   /* Metric value as an integer. */
        float floatValue;                   /* Metric value as a floating point. */

        struct {
            char string[METRIC_INFO_STR_SIZE];
            /**< String length in characters, up to maximum length of string (in bytes). */
            uint32_t stringLen;
        } stringValue;                      /* Metric value as a string. */

        struct {
            uint64_t array[METRIC_INFO_ARRAY_SIZE];
            uint32_t arrayLen;
        } arrayValue;                       /* Metric value as an array. */

        CellularDeviceEventType eventValue; /* Metric value as an event. */
        void *infoExtendValue;              /* Metric value as extended parameter. */
    } value;
};

class ICellularDeviceListener;

class ICellularDeviceManager {
public:

    /**
     * Checks the status of the CellularDeviceManager object and returns the result.
     *
     * @returns SERVICE_AVAILABLE    -  If CellularDeviceManager is ready for service.
     *          SERVICE_UNAVAILABLE  -  If CellularDeviceManager is temporarily unavailable.
     *          SERVICE_FAILED       -  If CellularDeviceManager encountered an irrecoverable
     *                                  failure.
     *
     */
    virtual telux::common::ServiceStatus getServiceStatus() = 0;

    /**
     * Set Modem Metric Info API
     * Sets diagnostic/management control information in service for a specific metric.
     *
     * @param [in] metricInfo          Control information to configure
     *
     * @returns operation error code (if any). @ref telux::common::ErrorCode.
     *
     */
    virtual telux::common::ErrorCode setModemMetricInfo(MetricInfo &metricInfo) = 0;

    /**
     * Get Modem Metric Info API
     * Gets diagnostic/management control information in service for a specific metric.
     *
     * @param [in] metricInfo          Diagnostic/Management information query and response
     *
     * @returns operation error code (if any). @ref telux::common::ErrorCode.
     *
     */
    virtual telux::common::ErrorCode getModemMetricInfo(
        CellularDeviceGetRequest metricId,
        MetricInfo &metricInfo) = 0;

    /**
     * Get Modem Metric Info List API
     * Gets diagnostic/management control information in service for multiple metrics.
     *
     * @param [in] metricInfoList          Diagnostic/Management information list query and response
     *
     * @returns operation error code (if any). @ref telux::common::ErrorCode.
     *
     */
    virtual telux::common::ErrorCode getModemMetricInfoList(
        std::vector<MetricInfo> &metricInfoList) = 0;

    /**
     * Register Modem Metric Indication API
     * Sets asynchronous indication in service for a specific metric.
     *
     * @param [in] metricInfo          Asynchronous indication information to configure
     *
     * @returns operation error code (if any). @ref telux::common::ErrorCode.
     *
     */
    virtual telux::common::ErrorCode registerModemMetricIndication(
        MetricInfo &metricInfo) = 0;

    /**
     * Get Modem Metric Indication Count API
     * Gets asynchronous indication count from service.
     *
     * @param [in] count          Asynchronous indication count
     *
     * @returns operation error code (if any). @ref telux::common::ErrorCode.
     *
     */
    virtual telux::common::ErrorCode getModemMetricIndicationCount(uint32_t &count) = 0;

    /**
     * Get Modem Metric Indication List API
     * Gets asynchronous indication list from serivce.
     *
     * @param [in] metricIndicationList          Asynchronous indication list
     *
     * @returns operation error code (if any). @ref telux::common::ErrorCode.
     *
     */
    virtual telux::common::ErrorCode getModemMetricIndicationList(
        std::vector<CellularDeviceIndication> &metricIndicationList) = 0;

    /**
     * Clear Modem Metric Indication List API
     * Clears asynchronous indications from CDM service.
     *
     * @param [in] metricIndicationList          Asynchronous indication clear list
     *
     * @returns operation error code (if any). @ref telux::common::ErrorCode.
     *
     */
    virtual telux::common::ErrorCode clearModemMetricIndicationList(
        std::vector<CellularDeviceIndication> &metricIndicationList) = 0;

    /**
     * Registers with the ICellularDeviceManager as a listener for service status and other events.
     *
     * @param [in] listener    Pointer to the ICellularDeviceListener object that processes the
     *                         notification
     *
     * @returns Status of registerListener.
     *
     */
     virtual telux::common::Status registerListener(
        std::weak_ptr<ICellularDeviceListener> listener) = 0;

    /**
     * Removes a previously added listener.
     *
     * @param [in] listener    Pointer to the ICellularDeviceListener object that needs to be removed
     *
     * @returns Status of deregisterListener.
     *
     */
    virtual telux::common::Status deregisterListener(
        std::weak_ptr<ICellularDeviceListener> listener) = 0;

    /**
     * Destructor for ICellularDeviceManager
     */
    ~ICellularDeviceManager() {};
};

/**
 * Interface for Cellular Device listener object. Client needs to implement this interface to get
 * access to Cellular Device services notifications like onServiceStatusChange.
 *
 * The methods in listener can be invoked from multiple different threads. The implementation
 * should be thread safe.
 *
 */
 class ICellularDeviceListener : public telux::common::ISDKListener {
    public:
        /**
         * This function is called when the service status changes.
         *
         * @param [in] status - @ref ServiceStatus
         */
        virtual void onServiceStatusChange(telux::common::ServiceStatus status) {}

        /**
         * This function is called when the Modem status indication changes.
         *
         * @param [in] metricInfo - @ref MetricInfo
         */
        virtual void onModemStatusChanged(MetricInfo &metricInfo) {}

        /**
         * This function is called when the Registration status indication changes.
         *
         * @param [in] metricInfo - @ref MetricInfo
         */
        virtual void onRegistrationStatusChanged(MetricInfo &metricInfo) {}

        /**
         * This function is called when the LTE serving cell RSRP info indication changes.
         *
         * @param [in] metricInfo - @ref MetricInfo
         */
        virtual void onLteServingCellRsrpChanged(MetricInfo &metricInfo) {}

        /**
         * This function is called when the NR5G serving cell RSRP info indication changes.
         *
         * @param [in] metricInfo - @ref MetricInfo
         */
        virtual void onNr5gServingCellRsrpChanged(MetricInfo &metricInfo) {}

        /**
         * This function is called when the LTE serving cell RSRQ info indication changes.
         *
         * @param [in] metricInfo - @ref MetricInfo
         */
        virtual void onLteServingCellRsrqChanged(MetricInfo &metricInfo) {}

        /**
         * This function is called when the NR5G serving cell RSRQ info indication changes.
         *
         * @param [in] metricInfo - @ref MetricInfo
         */
        virtual void onNr5gServingCellRsrqChanged(MetricInfo &metricInfo) {}

        /**
         * This function is called when the LTE serving cell SINR info indication changes.
         *
         * @param [in] metricInfo - @ref MetricInfo
         */
        virtual void onLteServingCellSinrChanged(MetricInfo &metricInfo) {}

        /**
         * This function is called when the NR5G serving cell SINR info indication changes.
         *
         * @param [in] metricInfo - @ref MetricInfo
         */
        virtual void onNr5gServingCellSinrChanged(MetricInfo &metricInfo) {}

        /**
         * This function is called when the mmWave panel position status indication changes.
         *
         * @param [in] metricInfo - @ref MetricInfo
         */
        virtual void onMmwPanelPositionStatusChanged(MetricInfo &metricInfo) {}

        /**
         * This function is called when the Safe install status mode indication changes.
         *
         * @param [in] metricInfo - @ref MetricInfo
         */
        virtual void onInstallModeStatusChanged(MetricInfo &metricInfo) {}

        /**
         * This function is called when the Safe install measurement indication changes.
         *
         * @param [in] metricInfo - @ref MetricInfo
         */
        virtual void onInstallMeasurementChanged(MetricInfo &metricInfo) {}

        /**
         * Destructor for ICellularDeviceListener
         */
        virtual ~ICellularDeviceListener() {}
        };

}  // namespace data
}  // namespace telux

#endif  // TELUX_DATA_CELLULAR_DEVICE_MANAGER_HPP