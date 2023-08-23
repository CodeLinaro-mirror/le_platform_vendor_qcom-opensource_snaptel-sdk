/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file       TelDefinesStub.hpp
 *
 * @brief      This header file contains all the structures, enums for the TelDefines class.
 *
 */

#ifndef TEL_STUB_DEFINES_HPP
#define TEL_STUB_DEFINES_HPP

namespace telux {
namespace tel {

/* Tel Filters */
const string  TEL_CALL_FILTER = "tel_call";
const string  TEL_CARD_FILTER = "tel_card";
const string  TEL_CELL_BROADCAST_FILTER = "tel_cell";
const string  TEL_HTTP_FILTER = "tel_http";
const string  TEL_IMS_SERVING_FILTER = "tel_ims_serv";
const string  TEL_IMS_SETTINGS_FILTER = "tel_ims_setting";
const string  TEL_MULTISIM_FILTER = "tel_multisim";
const string  TEL_NETWORK_SELECTION_FILTER = "tel_network_select";
const string  TEL_PHONE_FILTER = "tel_phone";
const string  TEL_REMOTE_SIM_FILTER = "tel_remote";
const string  TEL_SAP_CARD_FILTER = "tel_sap";
const string  TEL_SERVING_SYSTEM_FILTER = "tel_serv";
const string  TEL_SIM_PROFILE_FILTER = "tel_sim";
const string  TEL_SMS_FILTER = "tel_sms";
const string  TEL_SUBSCRIPTION_FILTER = "tel_sub";
const string  TEL_SUPP_SERVICES_FILTER = "tel_supp";

/* string received for SSR events. */
const string SSR_UP_EVENT = "ssr_up";
const string SSR_DOWN_EVENT = "ssr_down";
const string SSR_MODEM = "modem";
const string SSR_APPS = "apps";

#define DEFAULT_DELIMITER " "

} // end of namespace tel

} // end of namespace telux

#endif // TEL_STUB_DEFINES_HPP