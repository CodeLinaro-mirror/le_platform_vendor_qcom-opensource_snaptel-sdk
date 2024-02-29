/*
 *  Copyright (c) 2020, The Linux Foundation. All rights reserved.
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
/*
 *  Changes from Qualcomm Innovation Center are provided under the following license:
 *  Copyright (c) 2021-2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


/**
 * file       LocationManagerStub.hpp
 * brief      Location manager provides APIs to get position reports
 *            and satellite vehicle information updates. The reports
 *            specific to particular location engine can also be obtained
 *            by choosing the required engine report.
 *
 */

#ifndef MY_LOCATION_MANAGER_HPP
#define MY_LOCATION_MANAGER_HPP

#include "telux/loc/LocationManager.hpp"
#include "ReportHandler.hpp"
#include "ReportReader.hpp"
#include "common/AsyncTaskQueue.hpp"

#include <grpcpp/grpcpp.h>
#include "protos/proto-src/loc.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;

using locStub::LocationManagerService;

namespace telux {

namespace loc {

/**
 * brief LocationManagerStub provides interface to register and remove listeners.
 * It also allows to set and get configuration/ criteria for position reports.
 * The new APIs(registerListenerEx, deRegisterListenerEx, startDetailedReports,
 * startBasicReports) and old/deprecated APIs(registerListener, removeListener,
 * setPositionReportTimeout, setHorizontalAccuracyLevel, setMinIntervalForReports)
 * should not be used interchangebly, either the new APIs should be used or the
 * old APIs should be used.
 *
 */
class LocationManagerStub : public ILocationManager {
public:

/**
 * This function is called with the response to getEnergyConsumedInfoUpdate API.
 *
 * param[in] energyConsumed - Information regarding energy consumed by Gnss engine.
 *
 * param[in] error - Return code which indicates whether the operation succeeded
 *                    or not.
 *
 * note Eval: This is a new API and is being evaluated. It is subject to change and
 *             could break backwards compatibilty.
 *
 */
    using GetEnergyConsumedCallback = std::function<void(telux::loc::GnssEnergyConsumedInfo
        energyConsumed, telux::common::ErrorCode error)>;

/**
 * This function is called with the response to getYearOfHw API.
 *
 * param[in] yearOfHw - Year of hardware information.
 *
 * param[in] error - Return code which indicates whether the operation succeeded
 *                    or not.
 */
    using GetYearOfHwCallback = std::function<void(uint16_t yearOfHw,
        telux::common::ErrorCode error)>;

/**
 * This function is called with the response to getTerrestrialPosition API.
 *
 * param[in] terrestrialInfo - basic position related information.
 *
 * note Eval: This is a new API and is being evaluated. It is subject to change and
 *             could break backwards compatibility.
 */
  using GetTerrestrialInfoCallback = std::function<void(
      const std::shared_ptr<ILocationInfoBase> terrestrialInfo)>;

/**
 * Checks the status of location subsystems and returns the result.
 *
 * returns True if location subsystem is ready for service otherwise false.
 *
 */
    bool isSubsystemReady() override;

/**
 * This status indicates whether the object is in a usable state.
 *
 * returns SERVICE_AVAILABLE    -  If location manager is ready for service.
 *          SERVICE_UNAVAILABLE  -  If location manager is temporarily unavailable.
 *          SERVICE_FAILED       -  If location manager encountered an irrecoverable failure.
 */
    telux::common::ServiceStatus getServiceStatus() override;

/**
 * Wait for location subsystem to be ready.
 *
 * returns  A future that caller can wait on to be notified when location
 *           subsystem is ready.
 *
 */
    std::future<bool> onSubsystemReady() override;

/**
 * Register a listener for specific updates from location manager like
 * location, jamming info and satellite vehicle info. If enhanced position,
 * using Dead Reckoning etc., is enabled, enhanced fixes will be provided.
 * Otherwise raw GNSS fixes will be provided.
 * The position reports will start only when startDetailedReports or
 * startBasicReports is invoked.
 *
 * param [in] listener - Pointer of ILocationListener object that processes
 *                        the notification.
 *
 * returns Status of registerListener i.e success or suitable status code.
 *
 *
 */
    telux::common::Status registerListenerEx(std::weak_ptr<ILocationListener> listener) override;

/**
 * Remove a previously registered listener.
 *
 * param [in] listener - Previously registered ILocationListener that needs
 *                        to be removed.
 *
 * returns Status of removeListener success or suitable status code
 *
 */
    telux::common::Status deRegisterListenerEx(std::weak_ptr<ILocationListener> listener) override;

/**
 * Starts the richer location reports by configuring the time between them as
 * the interval. Any of the 3 APIs that is startDetailedReports or startDetailedEngineReports
 * or startBasicReports can be called one after the other irrespective of order, without
 * calling stopReports in between any of them and the API which is called last will be honored
 * for providing the callbacks. If multiple clients invoke this API with different interval,
 * then all the clients will be benefited with interval which is smallest among all the intervals.
 *
 * This Api enables the onDetailedLocationUpdate, onGnssSVInfo,
 * onGnssSignalInfo, onGnssNmeaInfo and onGnssMeasurementsInfo Apis on the listener.
 *
 * param [in] interval - Minimum time interval between two consecutive
 * reports in milliseconds.
 *
 * E.g. If minInterval is 1000 milliseconds, reports will be provided with a
 * periodicity of 1 second or more depending on the number of applications
 * listening to location updates.
 *
 * param [in] callback - Optional callback to get the response of set
 *             minimum interval for reports.
 *
 * param [in] reportMask - Optional field to specify which reports a client is interested in.
 *
 * returns Status of startDetailedReports i.e. success or suitable status
 * code.
 *
 */
    telux::common::Status startDetailedReports(uint32_t interval,
        telux::common::ResponseCallback callback = nullptr,
            GnssReportTypeMask reportMask = DEFAULT_GNSS_REPORT) override;

/**
 * Starts a session which may provide richer default combined position reports
 * and position reports from other engines. The fused position report type will
 * always be supported if at least one engine in the system is producing valid report.
 * Any of the 3 APIs that is startDetailedReports or startDetailedEngineReports
 * or startBasicReports can be called one after the other irrespective of order, without
 * calling stopReports in between any of them and the API which is called last will be
 * honored for providing the callbacks. If multiple clients invoke this API with different
 * interval, then all the clients will be benefited with interval which is smallest among
 * all the intervals.
 * This Api enables the onDetailedLocationUpdate, onGnssSVInfo,
 * onGnssSignalInfo, onGnssNmeaInfo and onGnssMeasurementsInfo Apis on the listener.
 *
 * param [in] interval - Minimum time interval between two consecutive
 * reports in milliseconds.
 *
 * E.g. If minInterval is 1000 milliseconds, reports will be provided with a
 * periodicity of 1 second or more depending on the number of applications
 * listening to location updates.
 *
 * param [in] engineType - The type of engine requested for fixes such as
 * SPE or PPE or FUSED. The FUSED includes all the engines that are running to
 * generate the fixes such as reports from SPE, PPE and DRE.
 *
 * param [in] callback - Optional callback to get the response of set
 *             minimum interval for reports.
 *
 * param [in] reportMask - Optional field to specify which reports a client is interested in.
 *
 * returns Status of startDetailedEngineReports i.e. success or suitable status
 * code.
 *
 */
    telux::common::Status startDetailedEngineReports(uint32_t interval, LocReqEngine engineType,
        telux::common::ResponseCallback callback = nullptr,
            GnssReportTypeMask reportMask = DEFAULT_GNSS_REPORT) override;

/**
 * Starts the Location report by configuring the time and distance between
 * the consecutive reports. Any of the 3 APIs that is startDetailedReports or
 * startDetailedEngineReports or startBasicReports can be called one after the other
 * irrespective of order, without calling stopReports in between any of them and the
 * API which is called last will be honored for providing the callbacks. If multiple
 * clients invoke this API with different interval, then all the clients will be benefited
 * with interval which is smallest among all the intervals.
 *
 * This Api enables the onBasicLocationUpdate Api on the listener.
 *
 * param [in] distanceInMeters - distanceInMeters between two
 * consecutive reports in meters.
 * intervalInMs - Minimum time interval between two consecutive
 * reports in milliseconds.
 *
 * E.g. If intervalInMs is 1000 milliseconds and distanceInMeters is 100m,
 * reports will be provided according to the condition that happens first. So we need to
 * provide both the parameters for evaluating the report.
 *
 * The underlying system may have a minimum distance threshold(e.g. 1 meter).
 * Effective distance will not be smaller than this lower bound.
 *
 * The effective distance may have a granularity level higher than 1 m, e.g.
 * 5 m. So distanceInMeters being 59 may be honored at 60 m, depending on the system.
 *
 * Where there is another application in the system having a session with
 * shorter distance, this client may benefit and receive reports at that distance.
 *
 * param [in] callback - Optional callback to get the response of set
 *                        minimum distance for reports.
 *
 * returns Status of startBasicReports i.e. success or suitable status code.
 *
 */
    telux::common::Status startBasicReports(uint32_t distanceInMeters, uint32_t intervalInMs,
        telux::common::ResponseCallback callback = nullptr) override;
/**
 * This API registers a ILocationSystemInfoListener listener and will receive information related
 * to location system that are not tied with location fix session, e.g.: next leap second event.
 * The startBasicReports, startDetailedReports, startDetailedEngineReports does not need to be
 * called before calling this API, in order to receive updates.
 *
 * param [in] listener - Pointer of ILocationSystemInfoListener object.
 *
 * param [in] callback - Optional callback to get the response of location
 *                        system info.
 *
 * returns Status of getLocationSystemInfo i.e success or suitable status code.
 *
 * note Eval: This is a new API and is being evaluated. It is subject to change and
 *             could break backwards compatibility.
 *
 */
    telux::common::Status registerForSystemInfoUpdates(
        std::weak_ptr<ILocationSystemInfoListener> listener,
            telux::common::ResponseCallback callback = nullptr) override;

/**
 * This API removes a previously registered listener and will also stop receiving informations
 * related to location system for that particular listener.
 *
 * param [in] listener - Previously registered ILocationSystemInfoListener that needs to be
 *                        removed.
 *
 * param [in] callback - Optional callback to get the response of location
 *                        system info.
 *
 * returns Status of deRegisterForSystemInfoUpdates success or suitable status code.
 *
 * note Eval: This is a new API and is being evaluated. It is subject to change and
 *             could break backwards compatibility.
 *
 */
    telux::common::Status
        deRegisterForSystemInfoUpdates(std::weak_ptr<ILocationSystemInfoListener> listener,
            telux::common::ResponseCallback callback = nullptr) override;

/**
 * This API receives information on energy consumed by modem GNSS engine. If this API
 * is called on this object while this is already a pending request, then it will overwrite
 * the callback to be invoked and the callback from the previous invocation will not be
 * called.
 *
 * param [in] cb - callback to get the information of Gnss energy consumed.
 *
 * returns Status of requestEnergyConsumedInfo i.e success or suitable status code.
 *
 * note Eval: This is a new API and is being evaluated. It is subject to change and could
 *       break backwards compatibility.
 */
    telux::common::Status requestEnergyConsumedInfo(GetEnergyConsumedCallback cb) override;

/**
 * This API retrieves the year of hardware information.
 *
 * param[in] cb - callback to get information of year of hardware.
 *
 * returns Status of getYearOfHw i.e success or suitable status code.
 *
 */
  telux::common::Status getYearOfHw(GetYearOfHwCallback cb) override;

/**
 * This API retrieves capability information.
 *
 * @param[in] cb - callback to get information related to capability.
 *
 * returns Status of getCapabilities i.e success or suitable status code.
 *
 */
  telux::loc::LocCapability getCapabilities() override;

/**
 * This API will stop reports started using startDetailedReports or startBasicReports
 * or registerListener or setMinIntervalForReports.
 *
 * param [in] callback - Optional callback to get the response of stop reports.
 *
 * returns Status of stopReports i.e. success or suitable status code.
 *
 */
    telux::common::Status stopReports(telux::common::ResponseCallback callback = nullptr) override;

/**
 * This API retrieves single-shot terrestrial position using the set of specified terrestrial
 * technologies.
 * This API can be invoked even while there is an on-going tracking session that was started using
 * startBasicReports/startDetailedReports/startDetailedEngineReports.
 * If this API is invoked while there is already a pending request for terrestrial position, the
 * request will fail and @ref ResponseCallback will get invoked with
 * @ref ErrorCode::OP_IN_PROGRESS.
 * To cancel a pending request, use @ref ILocationManager::cancelTerrestrialPositionRequest.
 * Before using this API, user consent needs to be set true via
 * @ref ILocationConfigurator::provideConsentForTerrestrialPositioning.
 *
 * param[in] timeoutMsec - the time in milliseconds within which the client is expecting a
 *                          response. If the system is unable to provide a report within this
 *                          time, the @ref ResponseCallback will be invoked with
 *                          @ref ErrorCode::OPERATION_TIMEOUT.
 *
 * param[in] techMask - the set of terrestrial technologies that are allowed to be used for
 *                       producing the position.
 *
 * param[in] cb - callback to receive terrestrial position. This callback will only
 *                 be invoked when ResponseCallback is invoked with SUCCESS.
 *
 * param [in] callback - Optional callback to get the response of getTerrestrialPosition.
 *
 * returns Status of getTerrestrialPosition i.e success or suitable status code.
 *
 * note Eval: This is a new API and is being evaluated. It is subject to change and could
 *             break backwards compatibility.
 *
 */
  telux::common::Status getTerrestrialPosition(uint32_t timeoutMsec,
      TerrestrialTechnology techMask, GetTerrestrialInfoCallback cb, telux::common::
          ResponseCallback callback = nullptr) override;

/**
 * This API cancels the pending request invoked by @ref ILocationManager::getTerrestrialPosition.
 * If this API is invoked while there is no pending request for terrestrial position from
 * @ref ILocationManager::getTerrestrialPosition, then @ref ResponseCallback will be invoked
 * with @ref ErrorCode::INVALID_ARGUMENTS.
 *
 * param [in] callback - Optional callback to get the response of
 *                        cancelTerrestrialPositionRequest.
 *
 * returns Status of cancelTerrestrialPositionRequest i.e success or suitable status code.
 *
 * note Eval: This is a new API and is being evaluated. It is subject to change and could
 *             break backwards compatibility.
 *
 */
  telux::common::Status cancelTerrestrialPositionRequest(telux::common::
      ResponseCallback callback = nullptr) override;

/**
 * Constructor of ILocationManager
 */
    LocationManagerStub();

    telux::common::Status init(telux::common::InitResponseCb callback);
/**
 * Clean-up method
 */
    void cleanup();

/**
 * Destructor of ILocationManager
 */
    ~LocationManagerStub();

private:
    std::mutex mutex_;
    std::mutex listenerMutex_;
    std::mutex energyMutex_;
    std::vector<std::weak_ptr<ILocationListener>> listeners_;
    std::vector<std::weak_ptr<ILocationSystemInfoListener>> systemInfoListener_;
    std::atomic<int> exitThread_;
    std::atomic<int> exited_;
    // Type is store the type of reports currently requested
    // 0 - No Report
    // 1 - Basic Report
    // 2 - Detailed Report
    // 3 - Detailed Engine Report
    std::atomic<int> type_;
    // Basic Report Interval in msecs
    std::atomic<uint32_t> brInterval_;
    // Basic Report Interval in 100 msecs steps (500 msecs gives brSeqDelta as 5)
    std::atomic<uint32_t> brSeqDelta_;
    // Last used Basic Sequence No. (100 msecs step)
    std::atomic<uint32_t> brSeqNo_;
    // Detailed Report Interval in msecs
    std::atomic<uint32_t> drInterval_;
    // Detailed Report Interval in 100 msecs steps (500 msecs gives drSeqDelta as 5)
    std::atomic<uint32_t> drSeqDelta_;
    // Last used Detailed Sequence No. (100 msecs step)
    std::atomic<uint32_t> drSeqNo_;
    // Detailed Engine Report Interval in msecs
    std::atomic<uint32_t> derInterval_;
    // Detailed Engine Report Interval in 100 msecs steps (500 msecs gives drSeqDelta as 5)
    std::atomic<uint32_t> derSeqDelta_;
    // Last used Detailed Engine Sequence No. (100 msecs step)
    std::atomic<uint32_t> derSeqNo_;
    GetYearOfHwCallback cbYearOfHw_ = nullptr;
    GetEnergyConsumedCallback cbStore_ = nullptr;
    GetTerrestrialInfoCallback cbTerrestrialPosition_ = nullptr;
    GnssReportTypeMask reportTypeMask_ = 0;
    LocCapability capabilityMask_ = 127;
    bool cbLock_ = false;
    // used to sync between cancelling and getting the terrestrial position
    bool isGetTerrestrialRequestActive_ = false;
    // used in System Info report
    time_t sysInfoHourTime_ = 0;
    // used in System Info report
    time_t usedSysInfoHourTime_ = 0;
    std::mutex terrestrialPositionMutex_;
    std::condition_variable cvTerrestrialPosition_;
    std::unique_ptr<::locStub::LocationManagerService::Stub> stub_;

    bool waitForInitialization();
    void initSync(telux::common::InitResponseCb callback);
    std::condition_variable cv_;
    telux::common::AsyncTaskQueue<void> taskQ_;
    telux::common::ServiceStatus managerStatus_;

    std::shared_ptr<LocationInfoBase> getLastLocation(bool defaultLocInfo = false);

    // managerThread waits on Report Handler Condition variable cv
    // Based upon the Type invokes appropriate type of Reports
    // If listners are available for System Info invokes System Info report
    void managerThread();

    // system info report checks if the hour in system time has changed and invokes system
    // info report on every hour change
    // when object is created, 00 is stored for the variable, so that a system info report
    // is sent immediately
    void invokeSystemInfoReport(ReportHandler & rClass_);

    // Basic Report is invoked every DURATION time set for the object. checks if SeqNo
    // from the report handler has exceeded
    // DURATION(in Seq Nos: brSeqDelta_) + last sequence No used (brSeqNo_)
    void invokeBasicReport(ReportHandler & rClass_);

    // Detailed Report is invoked every DURATION time set for the object. checks if SeqNo
    // from the report handler has exceeded
    // DURATION(in Seq Nos: drSeqDelta_) + last sequence No used (drSeqNo_)
    void invokeDetailedReport(ReportHandler & rClass_);

    // Detailed Report is invoked every DURATION time set for the object. checks if SeqNo
    // from the report handler has exceeded
    // DURATION(in Seq Nos: derSeqDelta_) + last sequence No used (derSeqNo_)
    void invokeDetailedEngineReport(ReportHandler & rClass_);

};

} // end of namespace loc

} // end of namespace telux

#endif // LOCATION_MANAGER_HPP
