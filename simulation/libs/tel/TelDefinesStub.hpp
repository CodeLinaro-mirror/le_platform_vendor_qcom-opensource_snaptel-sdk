/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       TelDefinesStub.hpp
 *
 * @brief      This header file contains all the structures, enums for the TelDefines class.
 *
 */

#ifndef TEL_STUB_DEFINES_HPP
#define TEL_STUB_DEFINES_HPP

#include <string>

#define DEFAULT_DELAY 100

namespace telux {
namespace tel {

/* Tel Filters */
const std::string  TEL_CALL_FILTER = "tel_call";
const std::string  TEL_CARD_FILTER = "tel_card";
const std::string  TEL_CELL_BROADCAST_FILTER = "tel_cell";
const std::string  TEL_HTTP_FILTER = "tel_http";
const std::string  TEL_IMS_SERVING_FILTER = "tel_ims_serv";
const std::string  TEL_IMS_SETTINGS_FILTER = "tel_ims_setting";
const std::string  TEL_MULTISIM_FILTER = "tel_multisim";
const std::string  TEL_NETWORK_SELECTION_FILTER = "tel_network_select";
const std::string  TEL_PHONE_FILTER = "tel_phone";
const std::string  TEL_REMOTE_SIM_FILTER = "tel_remote";
const std::string  TEL_SAP_CARD_FILTER = "tel_sap";
const std::string  TEL_SERVING_SYSTEM_FILTER = "tel_serv";
const std::string  TEL_SERVING_SYSTEM_SELECTION_PREF = "tel_serv_sel_pref";
const std::string  TEL_SERVING_SYSTEM_INFO = "tel_serv_sys_info";
const std::string  TEL_SERVING_SYSTEM_NETWORK_TIME = "tel_serv_network_time";
const std::string  TEL_SERVING_SYSTEM_RF_BAND_INFO = "tel_serv_rf_band_info";
const std::string  TEL_SERVING_SYSTEM_NETWORK_REJ_INFO = "tel_serv_network_reject_info";
const std::string  TEL_SERVING_SYSTEM_ARFCN_INFO = "tel_serv_arfcn_info";
const std::string  TEL_SIM_PROFILE_FILTER = "tel_sim";
const std::string  TEL_SMS_FILTER = "tel_sms";
const std::string  TEL_SUBSCRIPTION_FILTER = "tel_sub";
const std::string  TEL_SUPP_SERVICES_FILTER = "tel_supp";
const std::string  TEL_AP_SIM_PROFILE_FILTER = "tel_apsimprofile";

/* string received for SSR events. */
const std::string SSR_UP_EVENT = "ssr_up";
const std::string SSR_DOWN_EVENT = "ssr_down";
const std::string SSR_MODEM = "modem";
const std::string SSR_APPS = "apps";

#define DEFAULT_DELIMITER " "

} // end of namespace tel

} // end of namespace telux

#endif // TEL_STUB_DEFINES_HPP
