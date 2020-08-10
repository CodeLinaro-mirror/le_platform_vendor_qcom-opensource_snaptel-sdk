/*
 *  Copyright (c) 2019-2020, The Linux Foundation. All rights reserved.
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


#ifndef CV2XTELUX_HPP
#define CV2XTELUX_HPP

#include <mutex>

#include <Cv2xUtils.hpp>
#include <Cv2xLog.hpp>
#include <telux/cv2x/Cv2xFactory.hpp>
#include <telux/cv2x/Cv2xRadioManager.hpp>
#include <telux/data/DataProfileManager.hpp>
#include <telux/data/DataConnectionManager.hpp>
#include <telux/data/DataFactory.hpp>
#include <telux/cv2x/Cv2xRadioListener.hpp>

using telux::cv2x::Cv2xStatus;
using telux::cv2x::Cv2xStatusType;
using telux::data::ProfileParams;
using telux::data::DataCallStatus;
using telux::data::DataProfile;
using telux::data::DataCallEndReason;
using telux::data::EndReasonType;
using telux::data::IDataCall;
using telux::data::IDataConnectionManager;
using telux::data::IDataConnectionListener;
using telux::data::IpFamilyType;
using telux::cv2x::Cv2xFactory;
using telux::data::DataFactory;
using telux::data::IDataProfileManager;
using telux::data::TechPreference;
using telux::data::IpFamilyType;
using telux::cv2x::ICv2xRadioManager;
using telux::common::IServiceStatusListener;
using telux::common::ServiceStatus;
using telux::common::Status;
using telux::cv2x::ICv2xListener;

/** Internal class for data call */
struct ProfileIds {
    public:
        int ip;
        int nonIp;
} ;

enum DataCallType {
    CV2X_DATA_CALL_IP = 0,
    CV2X_DATA_CALL_NON_IP,
};

class DataCallInfo {
    public:
        DataCallType type;
        uint8_t profileIndex;
        std::string ifaceName;
};

/** TELUX callbacks */
class Cv2xTelux;

class QueryProfileCallback : public telux::data::IDataProfileListCallback
{
    public:
        QueryProfileCallback(std::shared_ptr<std::promise<ProfileIds>> prom);
        void onProfileListResponse(const std::vector<std::shared_ptr<DataProfile>> &profiles,
                telux::common::ErrorCode error) override;
    private:
        const std::string APN_NAME_V2X_IP = std::string("v2x_ip");
        const std::string APN_NAME_V2X_NON_IP = std::string("v2x_non_ip");
        // V2x profile Ids set by the modem in the HW MBN
        const int MIN_V2X_PROFILE_ID = 38;
        const int MAX_V2X_PROFILE_ID = 40;

        std::shared_ptr<std::promise<ProfileIds>> prom_;
};

class DataConnectionListener: public telux::data::IDataConnectionListener
{
    public:
        /**
         * Data Connection Listener
         */
        void onDataCallInfoChanged(const std::shared_ptr<IDataCall> &dataCall) override;
        /**
         * SSR Listener
         */
        void onServiceStatusChange(ServiceStatus status) override;

        DataConnectionListener(std::weak_ptr<Cv2xTelux> instance);

        void waitDataCallConnect(DataCallType callType, bool& connect);

    private:
        std::weak_ptr<Cv2xTelux> cv2xTelux_;
        std::condition_variable dcv_;
        std::mutex dmutex_;
        DataCallStatus ipStatus_;
        DataCallStatus nonIpStatus_;
};

class Cv2xTelux : public telux::cv2x::ICv2xListener,
                  public std::enable_shared_from_this<Cv2xTelux> {

    public:

        Cv2xTelux();

        /**
         * Initialize the library, allocate state variables
         */
        Status initV2xLibrary();

        /**
         * Initialize the data library
         */
        Status initDataLibrary();

        /**
         * De-initializes the library, de-allocates memory
         */
        Status deinitV2xLibrary();

        /**
         * Register for V2x Radio Listener
         */
        Status registerListeners();

        /**
         * Register for Data Listener
         */
        Status registerDataListeners();

        /**
         * Read the V2X radio status for TX and RX into the status variable.
         * The CBR value is not valid and has to be obtained via RRC indication
         */
        Status getV2xRadioStatus(Cv2xStatus &status);

        /**
         * Start the V2X radio operation
         */
        Status startV2xRadio();

        /**
         * Stop the V2X radio operation
         */
        Status stopV2xRadio();

        /**
         * find data profiles.
         * Start a data connection in V2X mode, there are two options, IP and NON_IP.
         * .
         */
        Status findProfilesAndStartDataCalls();

        /**
         * Tear-down a V2X data connection.
         */
        int stopV2xDataCalls();

        /**
         * V2X Radio Listener
         */
        void onStatusChanged(Cv2xStatus status) override;

        /**
         * SSR Listener
         */
        void onServiceStatusChange(ServiceStatus status) override;

        /**
         * Identify V2X ip data call via profileID
         */
        bool isIpDataCall(uint8_t profileID);

        /**
         * Identify V2X non-ip data call via profileID
         */
        bool isNonIpDataCall(uint8_t profileID);

        bool isPostSSRV2XDone_;
        std::condition_variable cv_;
        std::mutex mutex_;

    private:
        const std::string APN_NAME_V2X_IP = std::string("v2x_ip");
        const std::string APN_NAME_V2X_NON_IP = std::string("v2x_non_ip");

        std::mutex dcMutex_;
        std::shared_ptr<DataCallInfo> dcInfoIP_;
        std::shared_ptr<DataCallInfo> dcInfoNonIP_;
        Cv2xStatus cv2xStatus_;
        bool isInitializationDone_;
        bool cv2xRxActiveDone_ = false;
        bool cv2xTxActiveDone_ = false;

        /* Telux objects */
        std::shared_ptr<ICv2xRadioManager> cv2xRadioMgr_;
        std::shared_ptr<IDataProfileManager> dataProfileMgr_;
        std::shared_ptr<IDataConnectionManager> dataConnectionMgr_;
        std::shared_ptr<DataConnectionListener> dataConnectionListener_;

        /**
         * Stop data call, used internally by stopV2xDataCall()
         */
        int stopDataCall(std::shared_ptr<DataCallInfo>, IpFamilyType ipFamilyType);

        /**
         * Handle finding V2X data profile, used internally by findProfilesAndStartDataCalls()
         */
        Status findProfiles();

        /**
         * Handle starting V2X data calls, used internally by findProfilesAndStartDataCalls()
         */
        Status startDataCalls();

        /**
         * Print log messages when cv2x status changes
         */
        void logStatusChanged(Cv2xStatus &status);

        /**
         * Start data call, used internally by startV2xDataCall()
         */
        Status startDataCall(std::shared_ptr<DataCallInfo> dataCall, IpFamilyType ipFamilyType);
};
#endif /* CV2XTELUX_H */
