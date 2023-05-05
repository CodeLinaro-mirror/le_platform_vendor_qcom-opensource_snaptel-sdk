/*
 *  Copyright (c) 2019-2021, The Linux Foundation. All rights reserved.
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
 *
 *  Copyright (c) 2021-2023 Qualcomm Innovation Center, Inc. All rights reserved.
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
  * @file: Application.cpp
  *
  * @brief: Base class for ITS stack application
  */

#include <ifaddrs.h>
#include <netdb.h>
#include "ApplicationBase.hpp"
#include "VehicleReceive.h"
using std::cout;
using std::string;
using std::map;
using std::pair;
using namespace telux::cv2x::prop;
using telux::cv2x::prop::V2xPropFactory;
using telux::cv2x::prop::CongestionControlUtility;
// Congestion Control related variables
CongestionControlData ApplicationBase::congestionControlOut;
sem_t ApplicationBase::congCtrlCbSem;
CongestionControlUserData ApplicationBase::congCtrlCbData;
bool ApplicationBase::cbSuccess;
shared_ptr<telux::cv2x::prop::ICongestionControlManager> ApplicationBase::congestionControlManager;
FILE* ApplicationBase::csvfp;
std::mutex ApplicationBase::csvMutex;
bool ApplicationBase::securityEnabled;
bool ApplicationBase::congCtrlEnabled;
std::string getCurrentTimestamp()
{
    using std::chrono::system_clock;
    auto currentTime = std::chrono::system_clock::now();
    char buffer[MAX_TIMESTAMP_BUFFER_SIZE];
    auto sinceEpoch = currentTime.time_since_epoch().count() / 1000000;
    auto millis = sinceEpoch % 1000;
    std::time_t tt = system_clock::to_time_t ( currentTime );
    auto timeinfo = localtime (&tt);
    int ret = strftime (buffer,80,"%F-%H:%M:%S.",timeinfo);
    snprintf(&buffer[ret], MAX_TIMESTAMP_BUFFER_SIZE, "%03d", (int)millis);
    return std::string(buffer);
}

void ApplicationBase::writeSecurityLog(char* tmpLogStr, uint32_t maxBufSize, FILE *myfp){
    // can pass mbd, signing, and verif stats here and other settings
    //std::vector<SignStats> stats = thrSignLatencies[std::this_thread::get_id()];
    //std::vector<VerifStats> stats = thrVerifLatencies[std::this_thread::get_id()];
}

/* Log Format:
 * TimeStamp    TimeStamp_ms    Time_monotonic
 * LogRecType   L2 ID    CBR Percent    CPU_Util
 * TXInterval   msgCnt  TempId  GPGSAMode
 * secMark  lat long    semi_major_dev  speed
 * heading  longAccel   latAccel    Tracking_Error
 * vehicleDensityInRange    ChannelQualityIndication
 * BSMValid max_ITT GPS-Time    Events  DCC random time Hysterisis
 */
void ApplicationBase::writeCongCtrlLog(char* tmpLogStr, uint32_t maxBufSize, FILE *myfp,
    shared_ptr<CongestionControlCalculations> congestionControlCalculations, bool validPkt) {
    if (!congestionControlCalculations) {
        std::cerr << "Invalid congestionControl output struct provided\n";
        return;
    }

    if(tmpLogStr == nullptr){
        std::cerr << "Invalid input buffer\n";
        return;
    }

    char* tmpPtr = tmpLogStr;
    char* const endBuf = tmpPtr + maxBufSize;

    if (congestionControlCalculations->trackingError) {
        tmpPtr += snprintf(tmpPtr, endBuf-tmpPtr, "%f,",
            congestionControlCalculations->trackingError);
    }
    else {
        tmpPtr += snprintf(tmpPtr, endBuf-tmpPtr, ",");
    }
    tmpPtr += snprintf(tmpPtr, endBuf-tmpPtr, "%f,",
        congestionControlCalculations->smoothDens);


    if (congestionControlCalculations->channData) {
        tmpPtr += snprintf(tmpPtr, endBuf-tmpPtr, "%f,",
            congestionControlCalculations->channData->channQualInd);
    }
    else {
        tmpPtr += snprintf(tmpPtr, endBuf-tmpPtr, ",");
    }
    tmpPtr += snprintf(tmpPtr, endBuf-tmpPtr, ",");
    tmpPtr += snprintf(tmpPtr, endBuf-tmpPtr, "%lu,",
        congestionControlCalculations->maxITT);

    //sps enhancement data
    if (congestionControlCalculations->spsEnhanceData) {
        tmpPtr += snprintf(tmpPtr, endBuf-tmpPtr, "%f,%d,%lu,%d,",
            0.0, 0, (long unsigned int)0, this->congCtrlConfig.spsEnhHysterPerc);
    }
    else {
       tmpPtr += snprintf(tmpPtr, endBuf-tmpPtr,  "0.0,0,0,%d",
        this->congCtrlConfig.spsEnhHysterPerc);
    }
}

class QitsCongCtrlListener :public ICongestionControlListener {
public:
void onCongestionControlDataReady (
    std::shared_ptr<CongestionControlUserData> congestionControlUserData,
        bool success) override {
    // once the user data is updated, the thread in qits
    // can now schedule a transmission
    // cast void pointer
    RadioTransmit* spsTransmit_ = (RadioTransmit*)congestionControlUserData->spsTransmit;
    // if sps enhancements enabled, we should make sure that the sps flow reservation is redone
    if(spsTransmit_ && congestionControlUserData->spsEnhancementsEnabled){
        // create a new sps flow with the rounded max ITT that congestionControl calculates
        SpsFlowInfo spsInfo;
        // congestionControl rounds it already to valid values for sps periodicity
        spsInfo.periodicityMs =
            (congestionControlUserData->congestionControlCalculations->maxITT);

        // set sps priority to same value
        spsInfo.priority = spsTransmit_->getSpsPriority();

        // set sps size to same value
        spsInfo.nbytesReserved = spsTransmit_->getSpsResSize();

        spsTransmit_->updateSpsFlow(spsInfo);
    }
    sem_post(congestionControlUserData->congestionControlSem);
}
};


// thread function to periodically change ID and cert
void ApplicationBase::changeIdTimer(unsigned int interval)
{
    printf("Time interval for pseudonym and id change is: %d\n", interval);
    std::thread([this, interval]() {
        while (true)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(interval));
            (this->*thrFn)();
        }
    }).detach();
}

// first need to call setup function to initialize the lcm id change
// then periodically call "idChange" and check return value and updates in idChangeData
void ApplicationBase::changeIdentity(){
    //  pseudonym cert change
    sem_wait(&idChangeData.idSem);
    int ret = SecService->idChange();
    if( ret < 0 ){
        if(appVerbosity > 1)
            fprintf(stderr,"Id Change Failure\n");
    }
    else{
        if(appVerbosity > 1)
            printf("Id Change Success\n");
        // if not simulation, perform l2 src randomization
        if (!this->isTxSim) { // radio
            for(int index = 0 ; index < spsTransmits.size(); index++){
                this->spsTransmits[index].updateSrcL2();
            }
        }
    }
    sem_post(&idChangeData.idSem);
}

void ApplicationBase::updateL2RvMap(uint32_t l2SrcId, rv_specs* rvSpec) {

    lock_guard<mutex> lk(l2MapMtx);
    this->l2RvMap[l2SrcId] = *rvSpec;
}

uint32_t ApplicationBase::vehiclesInRange() {
    lock_guard<mutex> lk(l2MapMtx);
    return this->l2RvMap.size();
}

void ApplicationBase::setL2RvFilteringList(int rate) {
    if (appVerbosity > 5) {
        std::cout << "L2 list filtering rate is" << rate << std::endl;
    }
    //Assuming RV are sending at 10 HZ, find the no. of vehicles to filter
    //Should we make this divisor dynamic?
    int vehsToFilter = rate / 10;
    int vehsFiltered = 0;
    lock_guard<mutex> lk(l2MapMtx);
    std::vector<L2FilterInfo> rvListToFilter;

    for (pair<uint32_t, rv_specs> element : this->l2RvMap) {
        L2FilterInfo rvSrc = {0};
        rvSrc.srcL2Id = element.first;
        rvSrc.pppp = 0;
        rvSrc.durationMs = this->configuration.l2FilteringTime;
        const auto now = timestamp_now();
        const auto diff = now - element.second.hv_timestamp_ms;
        //Remove the L2 id entery for Rv that has not sent a message in a long time.
        if (this->configuration.l2IdTimeThreshold * 10000 < diff) {
            if (appVerbosity > 5)
                cout << "Removing L2 id:" << element.first << endl;
            this->l2RvMap.erase(element.first);
        } else {
            //RV is out of HV zone
            if (element.second.out_of_zone == true) {
                rvListToFilter.push_back(rvSrc);
                vehsFiltered++;

            }
            //RV ttc is greater than max ttc and it's not deacclerating. If a car ahead of us
            //start deacc chances are it might crash with us.
            else if ((element.second.ttc >= 10000) && (element.second.rapid_decl == false)) {
                rvListToFilter.push_back(rvSrc);
                vehsFiltered++;
            }

            //RV behind us are deacc then we can filter it
            else if ((element.second.ttc >= 10000) && (element.second.rapid_decl == true) &&
                    (element.second.lt == SAME_LANE_BACK_SAMEDIR || element.second.lt ==
                    ADJLEFT_LANE_BACK_SAMEDIR ||
                    element.second.lt == ADJRIGHT_LANE_BACK_SAMEDIR)) {
                rvListToFilter.push_back(rvSrc);
                vehsFiltered++;
            }

            //RV is stopped and not in the same lane ahead
            else if (element.second.stopped == true && element.second.lt != 1) {
                rvListToFilter.push_back(rvSrc);
                vehsFiltered++;
            }

            //After checking the cases, check if the no of RV to filter is met.
            if (vehsFiltered == vehsToFilter) {
                radioReceives[0].setL2Filters(rvListToFilter);
                break;
            }
        }
    }
}

ApplicationBase::ApplicationBase(char* fileConfiguration, MessageType msgType,
    bool enableCsvLog){
    enableCsvLog_ = enableCsvLog;
    // set parameters according to config file
    if (this->loadConfiguration(fileConfiguration)) {
        return;
    }

    // set up kinematics listener
    if(configuration.enableLocationFixes){
        std::cout << "Enabling location fixes\n";
        appLocListener_ = make_shared<LocListener>();
        appLocListener_->setLocCbFn(&ApplicationBase::locCbFn);
        locListeners.push_back(appLocListener_);
        kinematicsReceive = std::make_shared<KinematicsReceive>
                (locListeners, this->configuration.locationInterval);
    }

    if(configuration.enableL2Filtering) {
        cv2xTmListener=std::make_shared<Cv2xTmListener>(appVerbosity);
    }

    uint8_t keyGenMethod = NO_KEY_GEN;
    // setup radio flows
    this->setup(msgType);
    if(!this->isTx)
        keyGenMethod = ASYMMETRIC_KEY_GEN;

    // one-time initialization for security ; if any
    if (this->configuration.enableSecurity == true) {
    #ifdef AEROLINK
        try{
          // LCM Constructor for Aerolink
          if(!this->configuration.lcmName.empty() && this->configuration.idChangeInterval){
              SecService = unique_ptr<SecurityService>(AerolinkSecurity::Instance(
                      configuration.securityContextName,
                      configuration.securityCountryCode,
                      configuration.lcmName.c_str(),
                      std::ref(idChangeData)
                      ));

              // lcm id change timer thread
              sem_init(&idChangeData.idSem, 0, 1);
              fprintf(stdout, "Performing ID Changes at time interval of: %f secs\n",
                  this->configuration.idChangeInterval/1000.0);
              changeIdTimer(this->configuration.idChangeInterval);

          }else{
              // Non-LCM Constructor for Aerolink
              SecService = unique_ptr<SecurityService>(AerolinkSecurity::Instance(
                      configuration.securityContextName,
                      configuration.securityCountryCode));
          }
        }catch(const std::runtime_error& error){
            fprintf(stderr, "Aerolink init failed: Please check config params \n");
            fprintf(stderr, "Attempting to close all radio flows\n");
            closeAllRadio();
            exit(0);
        }
    #else
        // If no Aerolink security library is specified
        SecService = unique_ptr<SecurityService>(NullSecurity::Instance(
                    configuration.securityContextName,
                    configuration.securityCountryCode));
    #endif
    }
    sem_init(&this->rx_sem, 0, 1);
    sem_init(&this->log_sem, 0, 1);
    cb =
        [this](bool emergent,
               const current_dynamic_vehicle_state_t* const vehicle_state = nullptr) {
            vehicleEventReport(emergent, vehicle_state);
    };

    VehRec.enableVehicleReceive(cb);

    if (configuration.qMonEnabled) // Add to config
    {
        //cout << "New qMon added\n";
        qMon = new QMonitor(*qMonConfig);
    }
}

ApplicationBase::ApplicationBase(const string txIpv4, const uint16_t txPort,
            const string rxIpv4, const uint16_t rxPort,
            char* fileConfiguration, bool enableCsvLog) {
    enableCsvLog_ = enableCsvLog;
    if (this->loadConfiguration(fileConfiguration)) {
        return;
    }

    if(configuration.enableLocationFixes){
        std::cout << "Enabling location fixes\n";
        appLocListener_ = make_shared<LocListener>();
        appLocListener_->setLocCbFn(&ApplicationBase::locCbFn);
        locListeners.push_back(appLocListener_);
        kinematicsReceive = std::make_shared<KinematicsReceive>
                (locListeners, this->configuration.locationInterval);
    }

    if(configuration.enableL2Filtering) {
        cv2xTmListener=std::make_shared<Cv2xTmListener>(appVerbosity);
    }

    // set to no encryption key generation by default
    uint8_t keyGenMethod = NO_KEY_GEN;
    if (txPort)
    {
        this->simTxSetup(txIpv4, txPort);
        this->isTxSim = true;
        // default is asymmetric in tx mode
        keyGenMethod = ASYMMETRIC_KEY_GEN;
    }
    if (rxPort) {
        this->simRxSetup(rxIpv4, rxPort);
        this->isRxSim = true;
        // check if we want to also send packets while we receive over Ethernet
        if(this->configuration.enableTxAlways){
            if(this->configuration.tx_port &&
                    !this->configuration.ipv4_dest.empty()){
                printf("Attempting RX and TX over Ethernet at same time\n");
                this->simTxSetup(this->configuration.ipv4_dest,
                        this->configuration.tx_port);
                this->isTxSim = true;
                // default is asymmetric in tx mode
                keyGenMethod = ASYMMETRIC_KEY_GEN;
            }else{
                // turn the flag off so that driver program knows
                printf("Please provide TX Port and Dest IP in config file\n");
                printf("Entering only RX mode\n");
                this->configuration.enableTxAlways = false;
            }
        }
    }
    if (this->configuration.enableSecurity == true) {
#ifdef AEROLINK
        SecService = unique_ptr<SecurityService>(AerolinkSecurity::Instance(
                    configuration.securityContextName,
                    configuration.securityCountryCode));
#else
        SecService = unique_ptr<SecurityService>(NullSecurity::Instance(
                    configuration.securityContextName,
                    configuration.securityCountryCode));
#endif
    }

    sem_init(&this->rx_sem, 0, 1);
    sem_init(&this->log_sem, 0, 1);

    cb =
        [this](bool emergent,
               const current_dynamic_vehicle_state_t* const vehicle_state) {
            vehicleEventReport(emergent, vehicle_state);
    };

    VehRec.enableVehicleReceive(cb);
    if (configuration.qMonEnabled) // Add to config
    {
        //cout << "New qMon added\n";
        qMon = new QMonitor(*qMonConfig);
    }
}

ApplicationBase::~ApplicationBase() {
    if (qMon) {
        delete qMon;
        std::cout << "Closed qMon\n";
    }
    if (qMonConfig) {
        delete qMonConfig;
        std::cout << "Closed qMonConfig\n";
    }

     closeAllRadio();
     {
         std::unique_lock<std::mutex> loc(stateMtx);
         exitApp = true;
         stateCv.notify_all();
     }
     sem_destroy(&rx_sem);
     sem_destroy(&log_sem);
     std::cout << "ApplicationBase destructing" << std::endl;
     {
        lock_guard<std::mutex> lock(csvMutex);
        if (nullptr != csvfp) {
            fclose(csvfp);
            csvfp = nullptr;
        }
     }
}

void ApplicationBase::vehicleEventReport(bool emergent,
    const current_dynamic_vehicle_state_t* const vehicle_state) {
    bool notify = false;
    if (emergent) {
        notify = true;
        {
            std::unique_lock<std::mutex> loc(stateMtx);
            criticalState = true;
            newEvent = true;
        }
        //TODO: use vehicle_state to construct critical BSM messages
    } else {
        if (criticalState) {
            notify = true;
            std::unique_lock<std::mutex> loc(stateMtx);
            criticalState = false;
            newEvent = false;
        }
    }

    if (notify) {
        // Wonder if should lockIDChange first, then do notify ???
        // if do lockIdChange first, might introduce some latency of sending event BSM
        // if notify first, low probability that ID would change when sending the event
        stateCv.notify_all();
        if (criticalState) {

           if (this->configuration.enableSecurity == true) {
               if (SecService->lockIdChange()) {
                   printf("Fail to lock ID change\n");
               }
           }
           // if congestion control enabled, notify the congestion control library
           if(this->configuration.enableCongCtrl && congestionControlManager != NULL){
               congestionControlManager->notifyCriticalEvent();
           }
        } else {
            // unblock id change
           if (this->configuration.enableSecurity == true) {
               if (SecService->unlockIdChange()) {
                   printf("Fail to lock ID change\n");
               }
           }
        }
    }
}
void ApplicationBase::prepareForExit() {
    std::unique_lock<std::mutex> loc(stateMtx);
    exitApp = true;
    stateCv.notify_all();
}

bool ApplicationBase::pendingTillEmergency() {
    bool ret = true;
    std::unique_lock<std::mutex> loc(stateMtx);
    stateCv.wait(loc,
                 [this, &ret] {
                     if (true == newEvent) {
                         ret = true;
                         // reset the event flag
                         newEvent = false;
                         return true;
                     }
                     if (exitApp) {
                         ret = false;
                         return true;
                     }
                     return false;
                 });

    return ret;
}

bool ApplicationBase::pendingTillNoEmergency() {
    bool ret = true;
    if (criticalState) {
        std::unique_lock<std::mutex> loc(stateMtx);
        stateCv.wait(loc,
                     [this, &ret] {
                         if (false == criticalState) {
                             ret = true;
                             return true;
                         }

                         if (exitApp) {
                             ret = false;
                             return true;
                         }

                         return false;
                     });
    }

    return true;
}

//Calculates received packets per second for Throttle Manager
void ApplicationBase::tmCommunication() {
    int load = 0;
    sem_wait(&this->log_sem);
    if (appVerbosity > 3) {
        printf("Arrival rate is: %d\n", this->totalRxSuccessPerSecond);
    }
    load = this->totalRxSuccessPerSecond;
    this->totalRxSuccessPerSecond = 0;
    sem_post(&this->log_sem);

    if (load != 0) {
        this->prevFilterRate = this->filterRate;
        if (abs((load)-(this->prevArrivalRate)) >= this->configuration.deltaInRxRate) {
            //set load to throttle manager
            cv2xTmListener->setLoad(load);
            this->prevArrivalRate=load;
        }
    }
}

uint16_t ApplicationBase::delimiterPos(string line, vector<string> delimiters){
    uint16_t pos = 65535; //Largest possible value of 16 bits.
    for (int i = 0; i < delimiters.size(); i++)
    {
        uint16_t delimiterPos = line.find(delimiters[i]);
        if (pos > delimiterPos) {
            pos = delimiterPos;
        }
    }
    return pos;
}

int ApplicationBase::loadConfiguration(char* file) {
    map <string, string> configs;
    string line;
    vector<string> delimiters = { " ", "\t", "#", "="};
    ifstream configFile(file);
    if (configFile.is_open())
    {
        while (getline(configFile, line))
        {
            if (line[0] != '#' && !line.empty())
            {
                uint8_t pos = 0;
                uint8_t end = ApplicationBase::delimiterPos(line, delimiters);
                string key = line.substr(pos, end);
                line.erase(0, end);
                while (line[0] == ' ' || line[0] == '=' || line[0] == '\t') {
                    line.erase(0, 1);
                }
                end = ApplicationBase::delimiterPos(line, delimiters);
                string value = line.substr(pos, end);
                configs.insert(pair<string, string>(key, value));
            }
        }
        this->saveConfiguration(configs);
        return 0;
    }

    cout<<"Error opening config file.\n";
    return -1;
}


void ApplicationBase::saveConfiguration(map<string, string> configs) {
    if (configs.end() != configs.find("EnablePreRecorded")) {
        istringstream is(configs["EnablePreRecorded"]);
        is >> boolalpha >> this->configuration.enablePreRecorded;
    }

    if (configs.end() != configs.find("PreRecordedFile")) {
        this->configuration.preRecordedFile = configs["PreRecordedFile"];
    }

    if (configs.end() != configs.find("SpsTransmitRate")) {
        this->configuration.transmitRate = stoi(configs["SpsTransmitRate"], nullptr, 10);
    }

    stringstream stream;
    if (configs.end() != configs.find("SpsFlows")) {
        auto num = stoi(configs["SpsFlows"], nullptr, 10);
        if (configs.end() != configs.find("SpsPorts")) {
            stream.str(configs["SpsPorts"]);
            for (uint32_t i = 0; i < num; i++) {
                string port;
                getline(stream, port, ',');
                if (port.empty()) {
                    break;
                }
                this->configuration.spsPorts.push_back(stoi(port, nullptr, 10));
            }
            stream.str("");
            stream.clear();
        }

        if (configs.end() != configs.find("SpsDestAddrs")) {
            stream.str(configs["SpsDestAddrs"]);
            for (uint32_t i = 0; i < num; i++)
            {
                string spsDestAddrs;
                getline(stream, spsDestAddrs, ',');
                if (spsDestAddrs.empty()) {
                    break;
                }
                this->configuration.spsDestAddrs.push_back(spsDestAddrs);
            }
            stream.str("");
            stream.clear();
        }

        if (configs.end() != configs.find("SpsDestNames")) {
            stream.str(configs["SpsDestNames"]);
            for (uint32_t i = 0; i < num; i++)
            {
                string spsDestNames;
                getline(stream, spsDestNames, ',');
                if (spsDestNames.empty()) {
                    break;
                }
                this->configuration.spsDestNames.push_back(spsDestNames);
            }
            stream.str("");
            stream.clear();
        }

        if (configs.end() != configs.find("SpsDestPorts")) {
            stream.str(configs["SpsDestPorts"]);
            for (uint32_t i = 0; i < num; i++)
            {
                string port;
                getline(stream, port, ',');
                if (port.empty()) {
                    break;
                }
                this->configuration.spsDestPorts.push_back(stoi(port, nullptr, 10));
            }
            stream.str("");
            stream.clear();
        }

        if (configs.end() != configs.find("SpsServiceIDs")) {
            stream.str(configs["SpsServiceIDs"]);
            for (uint32_t i = 0; i < num; i++)
            {
                string spsServiceIDs;
                getline(stream, spsServiceIDs, ',');
                if (spsServiceIDs.empty()) {
                    break;
                }
                this->configuration.spsServiceIDs.push_back(stoi(spsServiceIDs, nullptr, 10));
            }
            stream.str("");
            stream.clear();
        }
    }

    if (configs.end() != configs.find("EventFlows")) {
        auto num = stoi(configs["EventFlows"], nullptr, 10);
        if (configs.end() != configs.find("EventPorts")) {
            stream.str(configs["EventPorts"]);
            for (uint32_t i = 0; i < num; i++)
            {
                string port;
                getline(stream, port, ',');
                if (port.empty()) {
                    break;
                }
                this->configuration.eventPorts.push_back(stoi(port, nullptr, 10));
            }
            stream.str("");
            stream.clear();
        }

        if (configs.end() != configs.find("EventDestAddrs")) {
            stream.str(configs["EventDestAddrs"]);
            for (uint32_t i = 0; i < num; i++)
            {
                string EventDestAddrs;
                getline(stream, EventDestAddrs, ',');
                if (EventDestAddrs.empty()) {
                    break;
                }
                this->configuration.eventDestAddrs.push_back(EventDestAddrs);
            }
            stream.str("");
            stream.clear();
        }

        if (configs.end() != configs.find("EventDestNames")) {
            stream.str(configs["EventDestNames"]);
            for (uint32_t i = 0; i < num; i++)
            {
                string EventDestNames;
                getline(stream, EventDestNames, ',');
                if (EventDestNames.empty()) {
                    break;
                }
                this->configuration.eventDestNames.push_back(EventDestNames);
            }
            stream.str("");
            stream.clear();
        }

        if (configs.end() != configs.find("EventDestPorts")) {
            stream.str(configs["EventDestPorts"]);
            for (uint32_t i = 0; i < num; i++)
            {
                string port;
                getline(stream, port, ',');
                if (port.empty()) {
                    break;
                }
                this->configuration.eventDestPorts.push_back(stoi(port, nullptr, 10));
            }
            stream.str("");
            stream.clear();
        }

        if (configs.end() != configs.find("EventServiceIDs")) {
            stream.str(configs["EventServiceIDs"]);
            for (uint32_t i = 0; i < num; i++)
            {
                string eventServiceIDs;
                getline(stream, eventServiceIDs, ',');
                if (eventServiceIDs.empty()) {
                    break;
                }
                this->configuration.eventServiceIDs.push_back(
                    stoi(eventServiceIDs, nullptr, 10));
            }
            stream.str("");
            stream.clear();
        }
    }

    if (configs.end() != configs.find("ReceiveFlows")
        and configs.end() != configs.find("ReceivePorts")) {
        stream.str(configs["ReceivePorts"]);
        for (uint32_t i = 0; i < stoi(configs["ReceiveFlows"], nullptr, 10); i++)
        {
            string port;
            getline(stream, port, ',');
            if (port.empty()) {
                break;
            }
            this->configuration.receivePorts.push_back(stoi(port, nullptr, 10));
        }
        stream.str("");
        stream.clear();
    }


    if (configs.end() != configs.find("ReceiveSubIds")){
        stream.str(configs["ReceiveSubIds"]);
        for (uint32_t i = 0; i < stoi(configs["ReceiveSubIds"], nullptr, 10); i++)
        {
            string rxSubId;
            getline(stream, rxSubId, ',');
            if (rxSubId.empty()) {
                break;
            }
            this->configuration.receiveSubIds.push_back(stoi(rxSubId, nullptr, 10));
        }
        stream.str("");
        stream.clear();
    }

    // if empty, add rx sub id
    if(this->configuration.receiveSubIds.empty()){
        this->configuration.receiveSubIds.push_back(DEFAULT_BSM_PSID);
    }

    if (configs.end() != configs.find("LocationInterval")) {
        this->configuration.locationInterval = stoi(configs["LocationInterval"], nullptr, 10);
    }

    if (configs.end() != configs.find("enableLocationFixes")) {
        if (configs["enableLocationFixes"].find("true") != std::string::npos)
            this->configuration.enableLocationFixes = true;
        else
            this->configuration.enableLocationFixes = false;
    }

    if (configs.end() != configs.find("WraServiceID")) {
        this->configuration.wraServiceId = stoi(configs["WraServiceID"], nullptr, 10);
    }

    if (configs.end() != configs.find("BsmJitter")) {
        this->configuration.bsmJitter = stoi(configs["BsmJitter"], nullptr, 10);
    }

    if (configs.end() != configs.find("EnableVehicleExt")) {
        istringstream is2(configs["EnableVehicleExt"]);
        is2 >> boolalpha >> this->configuration.enableVehicleExt;
    }

    if (configs.end() != configs.find("PathHistoryPoints")) {
        this->configuration.pathHistoryPoints = stoi(configs["PathHistoryPoints"], nullptr, 10);
    }

    if (configs.end() != configs.find("VehicleWidth")) {
        this->configuration.vehicleWidth = stoi(configs["VehicleWidth"], nullptr, 10);
    }

    if (configs.end() != configs.find("VehicleLength")) {
        this->configuration.vehicleLength = stoi(configs["VehicleLength"], nullptr, 10);
    }

    if (configs.end() != configs.find("VehicleHeight")) {
        this->configuration.vehicleHeight = stoi(configs["VehicleHeight"], nullptr, 10);
    }

    if (configs.end() != configs.find("FrontBumperHeight")) {
        this->configuration.frontBumperHeight = stoi(configs["FrontBumperHeight"], nullptr, 10);
    }

    if (configs.end() != configs.find("RearBumperHeight")) {
        this->configuration.rearBumperHeight = stoi(configs["RearBumperHeight"], nullptr, 10);
    }

    if (configs.end() != configs.find("VehicleMass")) {
        this->configuration.vehicleMass = stoi(configs["VehicleMass"], nullptr, 10);
    }

    if (configs.end() != configs.find("BasicVehicleClass")) {
        this->configuration.vehicleClass = stoi(configs["BasicVehicleClass"], nullptr, 10);
    }

    if (configs.end() != configs.find("SirenInUse")) {
        this->configuration.sirenUse = stoi(configs["SirenInUse"], nullptr, 10);
    }

    if (configs.end() != configs.find("LightBarInUse")) {
        this->configuration.lightBarUse = stoi(configs["LightBarInUse"], nullptr, 10);
    }

    if (configs.end() != configs.find("SpecialVehicleTypeEvent")) {
        this->configuration.specialVehicleTypeEvent = stoi(configs["SpecialVehicleTypeEvent"],
                nullptr, 10);
    }

    if (configs.end() != configs.find("VehicleType")) {
        this->configuration.vehicleType = stoi(configs["VehicleType"], nullptr, 10);
    }

    if (configs.end() != configs.find("LdmSize")) {
        this->configuration.ldmSize = stoi(configs["LdmSize"], nullptr, 10);
    }

    if (configs.end() != configs.find("LdmGbTime")) {
        this->configuration.ldmGbTime = stoi(configs["LdmGbTime"], nullptr, 10);
    }

    if (configs.end() != configs.find("LdmGbTimeThreshold")) {
        this->configuration.ldmGbTimeThreshold =
            stoi(configs["LdmGbTimeThreshold"], nullptr, 10);
    }

    if (configs.end() != configs.find("TTunc")) {
        this->configuration.tunc = stoi(configs["TTunc"], nullptr, 10);
    }

    if (configs.end() != configs.find("TAge")) {
        this->configuration.age = stoi(configs["TAge"], nullptr, 10);
    }

    if (configs.end() != configs.find("TPacketError")) {
        this->configuration.packetError = stoi(configs["TPacketError"], nullptr, 10);
    }

    if (configs.end() != configs.find("TUncertainty3D")) {
        this->configuration.uncertainty3D = stoi(configs["TUncertainty3D"], nullptr, 10);
    }

    if (configs.end() != configs.find("TDistance")) {
        this->configuration.distance3D = stoi(configs["TDistance"], nullptr, 10);
    }

    if (configs.end() != configs.find("SourceIpv4Address")) {
        this->configuration.ipv4_src = configs["SourceIpv4Address"];
    }

    if (configs.end() != configs.find("EnableUDP")) {
        istringstream is3(configs["EnableUDP"]);
        is3 >> boolalpha >> this->configuration.enableUdp;
    }

    if (configs.end() != configs.find("enableTxAlways")) {
        // for tx and rx at same time
        istringstream is4(configs["enableTxAlways"]);
        is4 >> boolalpha >> this->configuration.enableTxAlways;
    }

    if (configs.end() != configs.find("DestIpv4Address")) {
        // only used when enableTxAlways and for Ethernet
        this->configuration.ipv4_dest = configs["DestIpv4Address"];
    }

    if (configs.end() != configs.find("TxPort")) {
        this->configuration.tx_port = stoi(configs["TxPort"], nullptr, 10);
    }

    /* ETSI config items */
    if (configs.find("MacAddr") != configs.end()) {
        int i = 0;
        auto pos = 0, prev = 0;
        do {
            pos = configs["MacAddr"].find(":", prev);
            if (pos != std::string::npos) {
                this->configuration.MacAddr[i] =
                    stoi(configs["MacAddr"].substr(prev, pos), 0, 16);
            }
            prev = pos + 1;
            i++;
        } while(pos != std::string::npos);
        pos = configs["MacAddr"].rfind(" ");
        this->configuration.MacAddr[5] =
            stoi(configs["MacAddr"].substr(pos + 1, std::string::npos), 0, 16);
    }
    if(configs.find("StationType") != configs.end()) {
        this->configuration.StationType = stoi(configs["StationType"]);
    }
    if (configs.find("CAMDestinationPort") != configs.end()) {
        this->configuration.CAMDestinationPort = (uint16_t)stoi(configs["CAMDestinationPort"]);
    }
    if (configs.find("psidValue") != configs.end()) {
        configuration.psid = stoi(configs["psidValue"],0,16);
    }
    /* Security service */
    if (configs.find("EnableSecurity") != configs.end()) {
        if (configs["EnableSecurity"].find("true") != std::string::npos)
            this->configuration.enableSecurity = true;
        else
            this->configuration.enableSecurity = false;
    }

    if (configuration.enableSecurity == true) {
        ApplicationBase::securityEnabled = true;
        if (configs.find("SecurityContextName") != configs.end()) {
            configuration.securityContextName = configs["SecurityContextName"];
        }
        if (configs.find("SecurityCountryCode") != configs.end()) {
            configuration.securityCountryCode = stoi(configs["SecurityCountryCode"], 0, 16);
        }
        if(configs.find("enableSsp") != configs.end()){
            if (configs["enableSsp"].find("true") != std::string::npos){
                this->configuration.enableSsp = true;
                if (configs.find("sspValue") != configs.end()) {
                    char* end;
                    uint8_t num = (uint8_t)std::count(configs["sspValue"].begin(),
                                    configs["sspValue"].end(), ':');
                    if(configs["sspValue"].back() != ':'){
                        num++;
                    }
                    this->configuration.sspLength = num;
                    stream.str(configs["sspValue"]);
                    for (uint32_t i = 0; i < num; i++)
                    {
                        string s;
                        getline(stream, s, ':');
                        if (s.empty()) {
                            break;
                        }
                        this->configuration.sspValueVect.push_back(s);
                        this->configuration.ssp[i] =
                                (uint8_t)strtol(
                                    this->configuration.sspValueVect.at(i).c_str(), &end,16);
                    }
                    stream.str("");
                    stream.clear();
                }
            }else{
                this->configuration.enableSsp = false;
                this->configuration.sspLength = 0;
            }
        }

        if(configs.find("enableSspMask") != configs.end()){
            if (configs["enableSspMask"].find("true") != std::string::npos){
                this->configuration.enableSspMask = true;
                if(configs.find("sspMask") != configs.end() &&
                        this->configuration.enableSsp == true &&
                        this->configuration.enableSspMask == true){
                    char* end;
                    uint8_t num = (uint8_t)std::count(configs["sspMask"].begin(),
                                    configs["sspMask"].end(), ':');
                    if(configs["sspMask"].back() != ':'){
                        num++;
                    }
                    this->configuration.sspMaskLength = num;
                    stream.str(configs["sspMask"]);
                    for (uint32_t i = 0; i < num; i++)
                    {
                        string s;
                        getline(stream, s, ':');
                        if (s.empty()) {
                            break;
                        }
                        this->configuration.sspMaskVect.push_back(s);
                        this->configuration.sspMask[i] =
                                (uint8_t)strtol(
                                    this->configuration.sspMaskVect.at(i).c_str(), &end,16);
                    }
                    stream.str("");
                    stream.clear();
                }
            }else{
                this->configuration.enableSspMask = false;
                this->configuration.sspMaskLength = 0;
            }
        }


        if(configs.find("enableAsync") != configs.end()) {
            istringstream is4(configs["enableAsync"]);
            is4 >> boolalpha >> configuration.enableAsync;
        }
        if(configs.find("enableEncrypt") != configs.end()) {
            istringstream is4(configs["enableEncrypt"]);
            is4 >> boolalpha >> configuration.enableEncrypt;
        }

        /* Signing-related statistics */
        if(configs.find("enableSignStatLog") != configs.end()){
           istringstream is7(configs["enableSignStatLog"]);
           is7 >> boolalpha >> configuration.enableSignStatLog;
        }

        if(configs.find("signStatLogListSize") != configs.end()){
            this->configuration.signStatsSize =
            (uint32_t)stoi(configs["signStatLogListSize"]);
        }

        if(configs.find("signStatLogFile") != configs.end()){
            this->configuration.signStatLogFile = configs["signStatLogFile"];
        }

        if(configuration.enableSignStatLog){
            std::cout << "Signing statistic logging is ON" << std::endl;
            std::cout << "Statistics for last " << configuration.signStatsSize <<
                " signs will be reported by each thread" << std::endl;
            std::cout << "Upon closure, statistics will be dumped to logfile: " <<
                configuration.signStatLogFile << std::endl;
        } else{
            std::cout << "Signing statistic logging is off" << std::endl;
        }

        /* Verification-related statistics */
        if(configs.find("enableVerifStatLog") != configs.end()){
           istringstream is8(configs["enableVerifStatLog"]);
           is8 >> boolalpha >> configuration.enableVerifStatLog;
        }

        if(configs.find("verifStatLogListSize") != configs.end()){
            this->configuration.verifStatsSize =
                (uint32_t)stoi(configs["verifStatLogListSize"]);
        }

        if(configs.find("verifStatLogFile") != configs.end()){
            this->configuration.verifStatLogFile = configs["verifStatLogFile"];
        }

        if(configuration.enableVerifStatLog){
            std::cout << "Verification statistic logging is ON" << std::endl;
            std::cout << "Statistics for last " << configuration.verifStatsSize <<
                " verifications will be reported by each thread" << std::endl;
            std::cout << "Upon closure, statistics will be dumped to logfile: " <<
                configuration.verifStatLogFile << std::endl;
        } else{
            std::cout << "Verification statistic logging is off" << std::endl;
        }

        /** Pseudonym/ID Change */
        if(configs.find("lcmName") != configs.end()){
            this->configuration.lcmName = configs["lcmName"];
        }

        if(configs.find("idChangeInterval") != configs.end()) {
            this->configuration.idChangeInterval =
                (unsigned int)stoi(configs["idChangeInterval"]);
        }

        /** Process both signed and unsigned packets */
        if(configs.find("acceptAll") != configs.end()){
            istringstream is9(configs["acceptAll"]);
            is9 >> boolalpha >> configuration.acceptAll;
            if(configuration.acceptAll){
                printf("Accepting both signed and unsigned messages\n");
            }else{
                printf("Only accepting signed messages\n");
            }
        }
        /* Misbehavior-related statistics */
        if(configs.find("enableMbd") != configs.end()){
            istringstream is1(configs["enableMbd"]);
            is1 >> boolalpha >> configuration.enableMbd;
            if(configuration.enableMbd) {
                if(configs.find("enableMbdStatLog") != configs.end()){
                    istringstream is8(configs["enableMbdStatLog"]);
                    is8 >> boolalpha >> configuration.enableMbdStatLog;
                    if(configuration.enableMbdStatLog){
                        if(configs.find("mbdStatLogListSize") != configs.end()){
                            this->configuration.mbdStatLogListSize =
                                (uint32_t)stoi(configs["mbdStatLogListSize"]);
                        }
                        if(configs.find("mbdStatLogFile") != configs.end()){
                            this->configuration.mbdStatLogFile = configs["mbdStatLogFile"];
                        }
                        std::cout << "Misbehavior statistic logging is ON" << std::endl;
                        std::cout << "Statistics for last " <<
                            configuration.mbdStatLogListSize <<
                            " misbehavior will be reported by each thread" << std::endl;
                        std::cout << "Upon closure, statistics will be dumped to logfile: " <<
                            configuration.mbdStatLogFile << std::endl;
                    } else{
                        std::cout << "Misbehavior statistic logging is off" << std::endl;
                    }
                }
            }
        }
    }

    /* codec debug */
    if (configs.find("codecVerbosity") != configs.end()) {
        this->configuration.codecVerbosity =
            (uint8_t)stoi(configs["codecVerbosity"]);
        set_codec_verbosity(stoi(configs["codecVerbosity"]));
    }

    /*app debug */
    if (configs.find("appVerbosity") != configs.end()) {
        setAppVerbosity(stoi(configs["appVerbosity"]));
        //qMon
    }

    /* ldm debug */
    if (configs.find("ldmVerbosity") != configs.end()) {
        this->configuration.ldmVerbosity =
            (uint8_t)stoi(configs["ldmVerbosity"]);
    }

    /* driver debug */
    if (configs.find("driverVerbosity") != configs.end()) {
        this->configuration.driverVerbosity =
            (uint8_t)stoi(configs["driverVerbosity"]);
    }
    /* security debug */
    if (configs.find("secVerbosity") != configs.end()) {
        this->configuration.secVerbosity =
            (uint8_t)stoi(configs["secVerbosity"]);
    }

    /* Multi-parallelism */
    if(configs.find("numRxThreadsEth") != configs.end()) {
        this->configuration.numRxThreadsEth = (uint8_t)stoi(configs["numRxThreadsEth"]);
    }
    if(configs.find("numRxThreadsRadio") != configs.end()) {
        this->configuration.numRxThreadsRadio = (uint8_t)stoi(configs["numRxThreadsRadio"]);
    }

    /** Filtering */
    if (configs.find("filterInterval") != configs.end()) {
        this->configuration.filterInterval = (unsigned int)stoi(configs["filterInterval"]);
    }

    /** Increase/decrease in the rx rate required to communicate to TM */
    if (configs.find("deltaInRxRate") != configs.end()) {
        this->configuration.deltaInRxRate = (unsigned int)stoi(configs["deltaInRxRate"]);
    }

    /** Enable L2 src filtering */
    if (configs.find("enableL2SrcFiltering") != configs.end()) {
        istringstream ipstream1(configs["enableL2SrcFiltering"]);
        ipstream1 >> boolalpha >> configuration.enableL2Filtering;
    }

    if (configs.find("l2SrcFilteringTime") != configs.end()) {
        istringstream ipstream2(configs["l2SrcFilteringTime"]);
        ipstream2 >> boolalpha >> configuration.l2FilteringTime;
    }

    if (configs.find("l2SrcIdTimeThresholdSec") != configs.end()) {
        istringstream ipstream3(configs["l2SrcIdTimeThresholdSec"]);
        ipstream3 >> boolalpha >> configuration.l2IdTimeThreshold;
    }

    /* WSA */
    configuration.routerLifetime = 0;
    configuration.ipPrefixLength = 0;
    if(configs.find("routerLifetime") != configs.end()){
        configuration.routerLifetime = stoi(configs["routerLifetime"]);
    }
    if (configs.find("ipPrefix") != configs.end()) {
        configuration.ipPrefix = configs["ipPrefix"];
    }
    if (configs.find("ipPrefixLength") != configs.end()) {
        configuration.ipPrefixLength = stoi(configs["ipPrefixLength"]);
    }
    if (configs.find("defaultGateway") != configs.end()) {
        configuration.defaultGateway = configs["defaultGateway"];
    }
    if (configs.find("primaryDns") != configs.end()) {
        configuration.primaryDns = configs["primaryDns"];
    }
    if (configs.find("wsaInfoFile") != configs.end()) {
        configuration.wsaInfoFile = configs["wsaInfoFile"];
    }
    if (configs.end() != configs.find("wsaInterval")) {
        this->configuration.wsaInterval = stoi(configs["wsaInterval"], nullptr, 10);
        // assume WSA Tx interval < 100ms is incorrect, re-set it to 100ms.
        if (this->configuration.wsaInterval < 100) {
            this->configuration.wsaInterval = 100;
        }
    }

    if(configs.find("wildcardRx") != configs.end()){
       istringstream is8(configs["wildcardRx"]);
       is8 >> boolalpha >> configuration.wildcardRx;
    }

    if (configs.end() != configs.find("Padding")) {
        this->configuration.padding = stoi(configs["Padding"], nullptr, 10);
        this->configuration.padding = (this->configuration.padding > MAX_PADDING_LEN) ?
                                       MAX_PADDING_LEN : this->configuration.padding;
    }

    if (configs.end() != configs.find("UnsignedBsmResSize")) {
        this->configuration.spsReservationSize =
                    (this->configuration.enableSecurity) ?
                                    stoi(configs["SignedBsmResSize"], nullptr, 10)
                                    : stoi(configs["UnsignedBsmResSize"], nullptr, 10);
        if(this->configuration.padding >= 0) {
            this->configuration.spsReservationSize += this->configuration.padding;
        }
    }

    if (configs.end() != configs.find("SpsPriority")) {
        this->configuration.spsPriority = static_cast<Priority>(
            stoi(configs["SpsPriority"], nullptr, 10));
    }

    if (configs.end() != configs.find("EventPriority")) {
        this->configuration.eventPriority = static_cast<Priority>(
            stoi(configs["EventPriority"], nullptr, 10));
    }

    this->configuration.isValid = true;

    // qMonitor Configuration
    if (configs.find("qMonEnabled") != configs.end())
    {
        //cout<< "qMon Config found!\n";
        istringstream is(configs["qMonEnabled"]);
        is >> boolalpha >> this->configuration.qMonEnabled;
        qMonConfig = new QMonitor::Configuration();
    }
    // Add qMonConfig elements here after this line.
    // e.g. qMonConfig->sockDomain = AF_INET; // etc etc...

    /*
      check if congestion control is enabled and begin setting the cong ctrl config parameters
    */
    if (configs.end() != configs.find("enableCongCtrl")) {
        istringstream is(configs["enableCongCtrl"]);
        is >> boolalpha >> this->configuration.enableCongCtrl;
        if(this->configuration.enableCongCtrl == true){
            ApplicationBase::congCtrlEnabled = true;
            saveCongCtrlConfig(configs);
        }
    }
}

void ApplicationBase::saveCongCtrlConfig(map<string, string> configs){
    std::cout << "Proceeding to find and save cong ctrl config parameters\n";

    if(configs.find("congCtrlType") != configs.end()){
        congCtrlConfig.congCtrlType = stoi(configs["congCtrlType"]);
    }
    if(configs.find("enableCongCtrlLogging") != configs.end()){
        congCtrlConfig.enableCongCtrlLogging = stoi(configs["enableCongCtrlLogging"]);
    }
    if(configs.find("cbpMeasInterval") != configs.end()){
        congCtrlConfig.cbpMeasInterval = stoi(configs["cbpMeasInterval"]);
    }
    if(configs.find("cbpWeightFactor") != configs.end()){
        congCtrlConfig.cbpWeightFactor = stod(configs["cbpWeightFactor"]);
    }
    if(configs.find("perInterval") != configs.end()){
        congCtrlConfig.perInterval = stoi(configs["perInterval"]);
    }
    if(configs.find("perSubInterval") != configs.end()){
        congCtrlConfig.perSubInterval = stoi(configs["perSubInterval"]);
    }
    if(configs.find("perMax") != configs.end()){
        congCtrlConfig.perMax = stod(configs["perMax"]);
    }
    if(configs.find("minChanQualInd") != configs.end()){
        congCtrlConfig.minChanQualInd = stod(configs["minChanQualInd"]);
    }
    if(configs.find("maxChanQualInd") != configs.end()){
        congCtrlConfig.maxChanQualInd = stod(configs["maxChanQualInd"]);
    }
    if(configs.find("vDensityWeightFactor") != configs.end()){
        congCtrlConfig.vDensityWeightFactor = stod(configs["vDensityWeightFactor"]);
    }
    if(configs.find("vDensityCoefficient") != configs.end()){
        congCtrlConfig.vDensityCoefficient = stod(configs["vDensityCoefficient"]);
    }
    if(configs.find("vDensityMinPerRange") != configs.end()){
        congCtrlConfig.vDensityMinPerRange = stoi(configs["vDensityMinPerRange"]);
    }
    if(configs.find("UseStaticVDensity") != configs.end()){
        congCtrlConfig.UseStaticVDensity = stoi(configs["UseStaticVDensity"]);
    }
    if(configs.find("vDensity") != configs.end()){
        congCtrlConfig.vDensity = stoi(configs["vDensity"]);
    }
    if(configs.find("txCtrlInterval") != configs.end()){
        congCtrlConfig.txCtrlInterval = stoi(configs["txCtrlInterval"]);
    }
    if(configs.find("hvTEMinTimeDiff") != configs.end()){
        congCtrlConfig.hvTEMinTimeDiff = stoi(configs["hvTEMinTimeDiff"]);
    }
    if(configs.find("hvTEMaxTimeDiff") != configs.end()){
        congCtrlConfig.hvTEMaxTimeDiff = stoi(configs["hvTEMaxTimeDiff"]);
    }
    if(configs.find("rvTEMinTimeDiff") != configs.end()){
        congCtrlConfig.rvTEMinTimeDiff = stoi(configs["rvTEMinTimeDiff"]);
    }
    if(configs.find("rvTEMaxTimeDiff") != configs.end()){
        congCtrlConfig.rvTEMaxTimeDiff = stoi(configs["rvTEMaxTimeDiff"]);
    }
    if(configs.find("teErrSensitivity") != configs.end()){
        congCtrlConfig.teErrSensitivity = stoi(configs["teErrSensitivity"]);
    }
    if(configs.find("teMinThresh") != configs.end()){
        congCtrlConfig.teMinThresh = stod(configs["teMinThresh"]);
    }
    if(configs.find("teMaxThresh") != configs.end()){
        congCtrlConfig.teMaxThresh = stod(configs["teMaxThresh"]);
    }
    if(configs.find("minItt") != configs.end()){
        congCtrlConfig.minItt = stoi(configs["minItt"]);
    }
    if(configs.find("txRand") != configs.end()){
        congCtrlConfig.txRand = stoi(configs["txRand"]);
    }
    if(configs.find("timeAccuracy") != configs.end()){
        congCtrlConfig.timeAccuracy = stoi(configs["timeAccuracy"]);
    }
    if(configs.find("vMax_ITT") != configs.end()){
        congCtrlConfig.maxItt = stoi(configs["vMax_ITT"]);
    }
    if(configs.find("vRescheduleTh") != configs.end()){
        congCtrlConfig.reschedThresh = stoi(configs["vRescheduleTh"]);
    }
    if(configs.find("supraGain") != configs.end()){
        congCtrlConfig.supraGain = stod(configs["supraGain"]);
    }
    if(configs.find("minChanUtil") != configs.end()){
        congCtrlConfig.minChanUtil = stoi(configs["minChanUtil"]);
    }
    if(configs.find("maxChanUtil") != configs.end()){
        congCtrlConfig.maxChanUtil = stoi(configs["maxChanUtil"]);
    }
    if(configs.find("minRadiPwr") != configs.end()){
        congCtrlConfig.minRadiPwr = stoi(configs["minRadiPwr"]);
    }
    if(configs.find("maxRadiPwr") != configs.end()){
        congCtrlConfig.maxRadiPwr = stoi(configs["maxRadiPwr"]);
    }

    if (configs.find("enableSpsEnhancements") != configs.end()) {
        istringstream is(configs["enableSpsEnhancements"]);
        is >> boolalpha >> congCtrlConfig.enableSpsEnhancements;
        if (congCtrlConfig.enableSpsEnhancements) {
            std::cout << "SPS Enhancements Enabled\n";
        }
        if(configs.find("cv2xMaxITTRounding") != configs.end()){
            congCtrlConfig.cv2xMaxITTRounding = stoi(configs["cv2xMaxITTRounding"]);
        }

        if (configs.find("spsEnhIntervalRound") != configs.end()) {
            congCtrlConfig.spsEnhIntervalRound = stoi(configs["spsEnhIntervalRound"]);
        }
        if (configs.find("spsEnhHysterPerc") != configs.end()) {
            congCtrlConfig.spsEnhHysterPerc = stoi(configs["spsEnhHysterPerc"]);
        }
        if (configs.find("spsEnhDelayPerc") != configs.end()) {
            congCtrlConfig.spsEnhDelayPerc = stoi(configs["spsEnhDelayPerc"]);
        }

    }
}

void ApplicationBase::simTxSetup(const string ipv4, const uint16_t port) {
    RadioOpt radioOpt;
    radioOpt.enableUdp = configuration.enableUdp;
    radioOpt.ipv4_src = configuration.ipv4_src;
    simTransmit = std::unique_ptr<RadioTransmit>
            (new RadioTransmit(radioOpt, ipv4, port));
    simTransmit->set_radio_verbosity(this->configuration.codecVerbosity);
    txSimMsg = std::make_shared<msg_contents>();
    abuf_alloc(&txSimMsg->abuf, ABUF_LEN, ABUF_HEADROOM);
}

void ApplicationBase::simRxSetup(const string ipv4, const uint16_t port) {
    if (this->configuration.ldmSize && this->ldm == nullptr) {
        this->ldm = new Ldm(this->configuration.ldmSize);
    }
    RadioOpt radioOpt;
    radioOpt.enableUdp = configuration.enableUdp;
    radioOpt.ipv4_src = configuration.ipv4_src;
    simReceive = std::unique_ptr<RadioReceive>
            (new RadioReceive(radioOpt, ipv4, port));
    simReceive->set_radio_verbosity(this->configuration.codecVerbosity);
    rxSimMsg = std::make_shared<msg_contents>();
    abuf_alloc(&rxSimMsg->abuf, ABUF_LEN, ABUF_HEADROOM);
}

// CV2X supported SPS period {20,50,100,...,900,1000} ms
int ApplicationBase::adjustSpsPeriodicity(int intervalMs) {
    if (intervalMs < 50) {
        return 20;
    } else if (intervalMs < 100) {
        return 50;
    }
    int ret = intervalMs / 100;
    if (ret >= 10) {
        return 1000;
    }
    return (ret * 100);
}

void ApplicationBase::setup(MessageType msgType) {
    uint8_t i = 0;
    // setup ldm
    if(this->configuration.ldmSize){
        this->ldm = new Ldm(this->configuration.ldmSize);
    }
    EventFlowInfo eventInfo;
    SpsFlowInfo spsInfo;

    if (MessageType::WSA == msgType) {
        spsInfo.periodicityMs = this->configuration.wsaInterval;
    } else {
        spsInfo.periodicityMs = this->configuration.transmitRate;
    }
    spsInfo.periodicityMs = adjustSpsPeriodicity(spsInfo.periodicityMs);

    // set sps priority to user specified value
    spsInfo.priority = this->configuration.spsPriority;
    spsInfo.nbytesReserved = this->configuration.spsReservationSize;
    if(appVerbosity > 3) {
        cout << "SPS period set to " << spsInfo.periodicityMs << "ms" << endl;
        cout << "SPS priority set to " << static_cast<uint32_t>(spsInfo.priority) << endl;
        cout << "SPS reservation size set to " << spsInfo.nbytesReserved << endl;
    }

    for (auto port : this->configuration.spsPorts)
    {

        RadioTransmit tx(spsInfo, TrafficCategory::SAFETY_TYPE, TrafficIpType::TRAFFIC_NON_IP,
                         port, this->configuration.spsServiceIDs[i], false, 0);
        // save Tx instance only if create Tx flow succeeded
        if (tx.flow) {
            this->spsTransmits.push_back(std::move(tx));
        } else {
            cerr << "ApplicationBase::setup error in creating Tx SPS flow!" <<
                    " with spsServiceId: " << this->configuration.spsServiceIDs[i]
                    << endl;
            return;
        }

        this->spsTransmits[i].configureIpv6(this->configuration.spsDestPorts[i],
                this->configuration.spsDestAddrs[i].c_str(),
                this->configuration.spsDestNames[i].c_str());
        /* radio debug */
        if (this->configuration.codecVerbosity) {
            this->spsTransmits[i].
                set_radio_verbosity(this->configuration.codecVerbosity);
        }
        std::shared_ptr<msg_contents> mc = std::make_shared<msg_contents>();
        abuf_alloc(&mc->abuf, ABUF_LEN, ABUF_HEADROOM);
        this->spsContents.push_back(mc);
        i += 1;
    }
    i = 0;
    for (auto port : this->configuration.receivePorts)
    {
        printf("Creating new rx subscription with port : %d\n", port);
        if (this->configuration.wildcardRx == true) {
            RadioReceive rx(TrafficCategory::SAFETY_TYPE, TrafficIpType::TRAFFIC_NON_IP, port);
            // save Rx instance only if create Rx flow succeeded
            if (rx.gRxSub) {
                this->radioReceives.push_back(std::move(rx));
            } else {
                cerr << "ApplicationBase::setup error in creating wildcard Rx!"
                        << endl;
                return;
            }
        } else {
            RadioReceive rx(TrafficCategory::SAFETY_TYPE,
                            TrafficIpType::TRAFFIC_NON_IP, port,
                            std::make_shared<std::vector<uint32_t>>
                             (this->configuration.receiveSubIds));
            // save Rx instance only if create Rx flow succeeded
            if (rx.gRxSub) {
                this->radioReceives.push_back(std::move(rx));
            } else {
                cerr << "ApplicationBase::setup error in creating non-wildcard Rx!"
                        << " with receiveSubIds: ";
                for(int j = 0; j < configuration.receiveSubIds.size(); j++){
                    cerr << "" << this->configuration.receiveSubIds[i]<< ", ";
                }
                cerr << "" << endl;
                return;
            }
        }

        /* radio debug */
        if (this->configuration.codecVerbosity) {
            this->radioReceives[i].
                set_radio_verbosity(this->configuration.codecVerbosity);
        }

        std::shared_ptr<msg_contents> mc = std::make_shared<msg_contents>();
        abuf_alloc(&mc->abuf, ABUF_LEN, ABUF_HEADROOM);
        this->receivedContents.push_back(mc);
        i += 1;
    }
    i = 0;
    for (auto port : this->configuration.eventPorts)
    {
        RadioTransmit tx(eventInfo, TrafficCategory::SAFETY_TYPE, TrafficIpType::TRAFFIC_NON_IP,
                         port, this->configuration.eventServiceIDs[i]);
        // save Tx instance only if create Tx flow succeeded
        if (tx.flow) {
            this->eventTransmits.push_back(std::move(tx));
        } else {
            cerr << "ApplicationBase::setup error in creating Tx event flow!"
                    << endl;
            return;
        }
        this->eventTransmits[i].configureIpv6(this->configuration.eventDestPorts[i],
                this->configuration.eventDestAddrs[i].c_str(),
                this->configuration.eventDestNames[i].c_str());
        /* radio debug */
        if (this->configuration.codecVerbosity) {
            this->eventTransmits[i].
                set_radio_verbosity(this->configuration.codecVerbosity);
        }

        std::shared_ptr<msg_contents> mc = std::make_shared<msg_contents>();
        abuf_alloc(&mc->abuf, ABUF_LEN, ABUF_HEADROOM);
        this->eventContents.push_back(mc);
        i += 1;
    }
    lastTxTime = timestamp_now();
}

void ApplicationBase::setupLdm(){
    this->ldm->startGb(this->configuration.ldmGbTime,
            this->configuration.ldmGbTimeThreshold);
    this->ldm->packeLossThresh = this->configuration.packetError;
    this->ldm->distanceThresh = this->configuration.distance3D;
    this->ldm->positionCertaintyThresh = this->configuration.uncertainty3D;
    this->ldm->tuncThresh = this->configuration.tunc;
    this->ldm->ageThresh = this->configuration.age;
    this->ldm->setLdmVerbosity(this->configuration.ldmVerbosity);
}

void ApplicationBase::fillSecurity(ieee1609_2_data *secData) {
    secData->protocolVersion = 3;
    if (this->configuration.enableSecurity == true)
        secData->content = signedData;
    else
        secData->content = unsecuredData;
    secData->tagclass = (ieee1609_2_tagclass)2;
}

// This function maybe overloaded to perform additonal operation before calling
// radio tx function.
int ApplicationBase::transmit(uint8_t index, std::shared_ptr<msg_contents> mc,
    int16_t bufLen, TransmitType txType) {
    std::thread::id tid = std::this_thread::get_id();
    if(MsgType ==  MessageType::BSM) {
        qMon->tData[tid].txBSMs++;
    }
    // If positive, should be the # of bytes sent
    // Else, something went wrong
    int ret = -1;

    // ethernet
    if (this->isTxSim) {
        ret = simTransmit->transmit(mc->abuf.data, bufLen,
                                    this->configuration.eventPriority);
    }else{ // radio
        if (txType == TransmitType::SPS) {
            // SPS priority is set when creating the flow
            ret =  this->spsTransmits[index].transmit(mc->abuf.data, bufLen,
                                                      Priority::PRIORITY_UNKNOWN);
        } else if (txType == TransmitType::EVENT) {
            // event priority is set per packet using traffic class
            ret =this->eventTransmits[index].transmit(mc->abuf.data, bufLen,
                                                      this->configuration.eventPriority);
        }
    }

    return ret;
}

int ApplicationBase::send(uint8_t index, TransmitType txType) {
    const auto i = index;
    auto encLength = 0;
    std::shared_ptr<msg_contents> mc = nullptr;
    bool validMessage = false;

    // if congestion control enabled, only send when congestionControl tells us to:
    if(this->configuration.enableCongCtrl && congCtrlInitialized){
        sem_wait (congestionControlManager->
                    getCongestionControlUserData()->congestionControlSem);
    }

    if (this->isTxSim) {
        mc = txSimMsg;
    } else if (txType == TransmitType::SPS) {
        mc = spsContents[i];
    } else if (txType == TransmitType::EVENT) {
        mc = eventContents[i];
    } else {
        return -1;
    }
    // reserve headroom in abuf if padding is specified
    abuf_reset(&mc->abuf, ABUF_HEADROOM + this->configuration.padding);
    fillMsg(mc);
    encLength = encode_msg(mc.get());
    if (this->configuration.enableSecurity) {
        encLength =
            encodeAndSignMsg(mc,
                             txType == TransmitType::EVENT ?
                             SecurityService::SignType::ST_CERTIFICATE :
                             SecurityService::SignType::ST_AUTO);
    }
    // this will avoid any log writing if the log file was never opened
    // save timestamp before sendto
    uint64_t timestamp = timestamp_now();
    lastTxTime = timestamp;
    if(this->configuration.enableCongCtrl){
        /* Will start it here because to prevent desynchronization
            between the transmit thread and congestion control startup */
        /* Start congestion control threads */
        if(!congCtrlInitialized){
            auto &v2xPropFactory = V2xPropFactory::getInstance();
            congestionControlManager = v2xPropFactory.getCongestionControlManager();
            congCtrlListener = std::make_shared<QitsCongCtrlListener>();
            congestionControlManager->registerListener(congCtrlListener);
            congCtrlInitialized = true;
            congestionControlManager->updateCongestionControlType
                ((CongestionControlType)congCtrlConfig.congCtrlType);
            CongestionControlUtility::setLoggingLevel(congCtrlConfig.enableCongCtrlLogging);
            congestionControlManager->updateCbpConfig(congCtrlConfig.cbpWeightFactor,
                congCtrlConfig.cbpMeasInterval);
            congestionControlManager->enableSpsEnhancements
                (congCtrlConfig.enableSpsEnhancements);
            congestionControlManager->updatePERConfig(congCtrlConfig.perMax,
                congCtrlConfig.perInterval, congCtrlConfig.perSubInterval);
            congestionControlManager->updateDensConfig
                (congCtrlConfig.vDensityCoefficient, congCtrlConfig.vDensityWeightFactor,
                congCtrlConfig.vDensityMinPerRange);
            congestionControlManager->updateTeConfig
                (congCtrlConfig.txCtrlInterval, congCtrlConfig.hvTEMinTimeDiff,
                congCtrlConfig.hvTEMaxTimeDiff, congCtrlConfig.rvTEMinTimeDiff,
                congCtrlConfig.rvTEMaxTimeDiff, congCtrlConfig.teMinThresh,
                congCtrlConfig.teMaxThresh, congCtrlConfig.teErrSensitivity);
            congestionControlManager->updateIttConfig(congCtrlConfig.reschedThresh,
            congCtrlConfig.timeAccuracy,
                congCtrlConfig.minItt, congCtrlConfig.maxItt, congCtrlConfig.txRand);
            if(congCtrlConfig.enableSpsEnhancements){
                congestionControlManager->updateSpsEnhanceConfig
                    (congCtrlConfig.spsEnhIntervalRound, congCtrlConfig.spsEnhDelayPerc,
                    congCtrlConfig.spsEnhHysterPerc);
            }
            if(CCErrorCode::SUCCESS !=
                    congestionControlManager->startCongestionControl()){
                std::cout << "Congestion control manager failed start up\n";
                exit(0);
            }
        }
    }

    int ret = this->transmit(index, mc, encLength, txType);

    // write the log here for this tx now. using tx timestamp made before sendto
    writeLog(mc, index, 0, true, txType, validMessage, timestamp);

    if (encLength > 0 && ret > 0) {
        validMessage = true;
        if(csvfp) {
            uint64_t currTime = timestamp_now();
            txInterval = currTime - lastTxTime;
            lastTxTime = currTime;
        }
    }
    if (kinematicsReceive && appLocListener_) {
        auto locationInfo = appLocListener_->getLocation();
        if (locationInfo) {
            locTimeMs_ = locationInfo->getTimeStamp();
            locPositionDop_ = locationInfo->getPositionDop();
            locNumSvUsed_ = locationInfo->getNumSvUsed();
        }
    }



    return (validMessage ? encLength : 0);
}

int ApplicationBase::encodeAndSignMsg(std::shared_ptr<msg_contents> mc,
                                      SecurityService::SignType type) {
    // The message need to be signed/encrypted after layer 3
    SecurityOpt sopt = {};
    uint8_t signedSpdu[512];
    uint32_t signedSpduLen = 512;
    sopt.psidValue = this->configuration.psid;
    if (this->configuration.sspLength){
         memcpy(sopt.sspValue, this->configuration.ssp,
            this->configuration.sspLength);
         if(this->configuration.enableSspMask && this->configuration.sspMaskLength){
            memcpy(sopt.sspMaskValue, this->configuration.sspMask,
                this->configuration.sspMaskLength);
         }
        sopt.sspLength = this->configuration.sspLength;
        sopt.sspMaskLength = this->configuration.sspMaskLength;
    }
    sopt.enableAsync = this->configuration.enableAsync;
    sopt.secVerbosity = this->configuration.secVerbosity;
    if(kinematicsReceive && appLocListener_){
        shared_ptr<ILocationInfoEx> locationInfo =
                                    appLocListener_->getLocation();
        sopt.hvKine.latitude = (locationInfo->getLatitude() * 10000000);
        sopt.hvKine.longitude = (locationInfo->getLongitude() * 10000000);
        sopt.hvKine.elevation = (locationInfo->getAltitude() * 10);
    }

    std::thread::id tid = std::this_thread::get_id();
    if(configuration.enableSignStatLog){
        if (thrSignLatencies[tid].size() > signStatIdx[tid]) {
            sopt.signStat = &thrSignLatencies[tid].at(signStatIdx[tid]);
        }else{
            signStatIdx[tid] = 0;
            sopt.signStat = &thrSignLatencies[tid].at(signStatIdx[tid]);
        }
    }
    auto encLength = 0;
    if (mc->abuf.tail_bits_left != 8)
        encLength = mc->abuf.tail - mc->abuf.data + 1;
    else
        encLength = mc->abuf.tail - mc->abuf.data;

    // Aerolink handles IEEE1609.2 header insertion, but this requires us to
    // make buffer copy of the header and the payload.
    if (SecService->SignMsg(sopt, (uint8_t*)mc->abuf.data,
                            encLength, signedSpdu, signedSpduLen, type) < 0) {
        return -1;
    }
    if(configuration.enableSignStatLog){
        // successful signing, increment the sign stat idx
        signStatIdx[tid]++;
        signStatIdx[tid]%=thrSignLatencies[tid].size();
    }
    abuf_purge(&mc->abuf, abuf_headroom(&mc->abuf));
    asn_ncat(&mc->abuf, (char *)signedSpdu, signedSpduLen);
    // transmit packet
    return encode_msg_continue(mc.get());
}

int ApplicationBase::receive(const uint8_t index, const uint16_t bufLen) {
    return -1;
}

int ApplicationBase::receive(const uint8_t index, const uint16_t bufLen,
        const uint32_t ldmIndex) {
    return -1;
}

void ApplicationBase::closeAllRadio() {

    for (uint8_t i = 0; i<this->eventTransmits.size(); i++)
    {
        this->eventTransmits[i].closeFlow();
    }
    eventTransmits.erase(eventTransmits.begin(),eventTransmits.end());
    for (uint8_t i = 0; i < this->spsTransmits.size(); i++)
    {
        this->spsTransmits[i].closeFlow();
    }
    spsTransmits.erase(spsTransmits.begin(),spsTransmits.end());

    for (uint8_t i = 0; i < this->radioReceives.size(); i++)
    {
        this->radioReceives[i].closeFlow();
    }
    radioReceives.erase(radioReceives.begin(),radioReceives.end());
    if (this->kinematicsReceive != nullptr)
    {
        this->kinematicsReceive->close();
    }
}

/**
 * Instantiate and initialize any variables associated with
 * security statistics logging
 */
void ApplicationBase::initVerifLogging() {
    std::vector<VerifStats> stats;
    sem_wait(&this->log_sem);
    for(int i = 0 ; i < configuration.verifStatsSize; i++)
        stats.push_back(VerifStats());
    thrVerifLatencies[std::this_thread::get_id()] = stats;
    if(remove(configuration.verifStatLogFile.c_str()) != 0){
        if(appVerbosity > 4)
            cerr << "Error deleting log file" << endl;
    }
    sem_post(&this->log_sem);
}

/**
 * Function to print out - if any - security related statistics
 * gathered from security side
 */
void ApplicationBase::writeVerifLogging() {
    ofstream file;
    sem_wait(&this->log_sem);
    std::stringstream ss;
    ss << std::this_thread::get_id();
    printf("Thread (%08x) is now dumping verification stats to %s\n",
            std::stoi(ss.str()),configuration.verifStatLogFile.c_str());
    file.open(configuration.verifStatLogFile.c_str(),
                std::ofstream::out | std::ofstream::app);
    std::vector<VerifStats> stats = thrVerifLatencies[std::this_thread::get_id()];
    for (auto it = stats.begin(); it != stats.end(); ++it) {
        if (it->timestamp != 0.0 && it->verifLatency != 0.0) {
            file << it->timestamp << ", " <<
                        it->verifLatency << std::endl;
        }
    }
    file.close();
    sem_post(&this->log_sem);
}

/**
 * Instantiate and initialize any variables associated with
 * security statistics logging
 */
void ApplicationBase::initSignLogging() {
    std::vector<SignStats> stats;
    sem_wait(&this->log_sem);
    for(int i = 0 ; i < configuration.signStatsSize; i++)
        stats.push_back(SignStats());
    thrSignLatencies[std::this_thread::get_id()] = stats;
    if(remove(configuration.signStatLogFile.c_str()) != 0){
        if(appVerbosity > 4)
            cerr << "Error deleting log file" << endl;
    }
    sem_post(&this->log_sem);
}

/**
 * Function to print out - if any - security related statistics
 * gathered from security side
 */
void ApplicationBase::writeSignLogging() {
    ofstream file;
    sem_wait(&this->log_sem);
    std::stringstream ss;
    ss << std::this_thread::get_id();
    printf("Thread (%08x) is now dumping signing stats to %s\n",
            std::stoi(ss.str()),configuration.signStatLogFile.c_str());
    file.open(configuration.signStatLogFile.c_str(),
                std::ofstream::out | std::ofstream::app);
    std::vector<SignStats> stats = thrSignLatencies[std::this_thread::get_id()];
    for (auto it = stats.begin(); it != stats.end(); ++it) {
        if (it->timestamp != 0.0 && it->signLatency != 0.0) {
            file << it->timestamp << ", " <<
                        it->signLatency << std::endl;
        }
    }
    file.close();
    sem_post(&this->log_sem);
}

/**
 * Instantiate and initialize any variables associated with
 *  Misbehavior statistics logging
 */
void ApplicationBase::initMisbehaviorLogging() {
    std::vector<MisbehaviorStats> stats;
    sem_wait(&this->log_sem);
    for(int i = 0 ; i < configuration.mbdStatLogListSize; i++)
        stats.push_back(MisbehaviorStats());
    thrMisbehaviorLatencies[std::this_thread::get_id()] = stats;
    if(remove(configuration.mbdStatLogFile.c_str()) != 0){
        if(appVerbosity > 4)
            cerr << "Error deleting log file" << endl;
    }
    sem_post(&this->log_sem);
}

/**
 * Function to print out - if any - Misbehavior related statistics
 * gathered from security side.
 */
void ApplicationBase::writeMisbehaviorLogging() {
    ofstream file;
    sem_wait(&this->log_sem);
    std::stringstream ss;
    ss << std::this_thread::get_id();
    printf("Thread (%08x) is now dumping misbehavior stats to %s\n",
            std::stoi(ss.str()),configuration.mbdStatLogFile.c_str());
    file.open(configuration.mbdStatLogFile.c_str(),
                std::ofstream::out | std::ofstream::app);
    std::vector<MisbehaviorStats> stats = thrMisbehaviorLatencies[std::this_thread::get_id()];
    for (auto it = stats.begin(); it != stats.end(); ++it) {
        if (it->timestamp != 0.0 && it->misbehaviorLatency != 0.0) {
            file << it->timestamp << ", " <<
                        it->misbehaviorLatency << std::endl;
        }
    }
    file.close();
    sem_post(&this->log_sem);
}

int ApplicationBase::getSysV2xIpIfaceAddr(string& ipAddr) {
    int result = -1;
    struct ifaddrs *ifap;
    struct ifaddrs *ifa;
    char addr[INET6_ADDRSTRLEN];
    string v2xIfName;

    //get V2X-IP iface name from the radio instance used for Tx WSA
    if (this->spsTransmits.empty()
        or this->spsTransmits[0].getV2xIfaceName(TrafficIpType::TRAFFIC_IP, v2xIfName)
        or v2xIfName.empty()) {
        cerr << "Failed to get V2X-IP iface name" << endl;
        return -1;
    }

    if (-1 == getifaddrs(&ifap)) {
        cerr << "Failed to get ifaddr!" << endl;
        return -1;
    }

    ifa = ifap;
    while (ifa && ifa->ifa_name) {
        if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET6) {
            string ifaName(ifa->ifa_name);
            if (ifaName == v2xIfName) {
                getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in6), addr, sizeof(addr),
                            NULL, 0, NI_NUMERICHOST);
                ipAddr = string(addr);
                if(appVerbosity > 3) {
                    cout << "Found V2X ifaceName:" << ifaName << " addr:" << ipAddr << endl;
                }
                result = 0;
                break;
            }
        }
        ifa = ifa->ifa_next;
    }
    freeifaddrs(ifap);

    if (result) {
        cerr << "Found no global IPv6 address for V2X IP iface!" << endl;
    }

    return result;
}

int ApplicationBase::updateCachedV2xIpIfaceAddr() {
    // get the old addr
    string oldAddr;
    {
        std::unique_lock<std::mutex> lock(v2xIpAddrMtx_);
        oldAddr = v2xIpAddr_;
    }

    string newAddr;
    if (0 == getSysV2xIpIfaceAddr(newAddr) and !newAddr.empty()) {
        if (oldAddr == newAddr) {
            cout << "V2X IP address not changed!" << endl;
        } else {
            // update local stored address
            std::unique_lock<std::mutex> lock(v2xIpAddrMtx_);
            v2xIpAddr_ = newAddr;
            if (appVerbosity > 3) {
                cout << "V2X IP address is upated to:" << newAddr << endl;
            }
            return 0;
        }
    }

    cerr << "Failed to update V2X IP iface address!" << endl;
    return -1;
}

int ApplicationBase::getV2xIpIfaceAddr(string& addr) {
    {
        std::unique_lock<std::mutex> lock(v2xIpAddrMtx_);
        addr = v2xIpAddr_;
    }

    if (addr.empty()) {
        cout << "Get V2X IP address failed!" << endl;
        return -1;
    }

    if (appVerbosity > 3) {
        cout << "Get V2X IP address " << addr << endl;
    }
    return 0;
}

bool ApplicationBase::openBsmLogFile(const std::string& fullPathName) {
    bool res = false;
    if (not enableCsvLog_) {
        return res;
    }
    {
        lock_guard<std::mutex> lock(csvMutex);
        if ((nullptr == csvfp) && !fullPathName.empty()) {
            csvfp = fopen(fullPathName.c_str(), "w+");
            if (!csvfp) {
                cerr << "Failed to open log file " << fullPathName << std::endl;
            } else {
                std::cout << "Open log " << fullPathName << " success!" << std::endl;
                res = true;
                write_bsm_header(csvfp);
            }
        }
    }
    if (res) {
        for(int index = 0 ; index < spsTransmits.size(); index++) {
            this->spsTransmits[index].enableCsvLog(enableCsvLog_);
        }
        for(int index = 0 ; index < eventTransmits.size(); index++) {
            this->eventTransmits[index].enableCsvLog(enableCsvLog_);
        }
        for(int index = 0 ; index < radioReceives.size(); index++) {
            this->radioReceives[index].enableCsvLog(enableCsvLog_);
        }
    }

    return res;
}

/* Log Format:
 * TimeStamp    TimeStamp_ms    Time_monotonic
 * LogRecType   L2 ID    CBR Percent    CPU_Util
 * TXInterval   msgCnt  TempId  GPGSAMode
 * secMark  lat long    semi_major_dev  speed
 * heading  longAccel   latAccel    Tracking_Error
 * vehicleDensityInRange    ChannelQualityIndication
 * BSMValid max_ITT GPS-Time    Events  DCC random time Hysterisis
 */
void ApplicationBase::writeLogHeader(FILE *fp) {
    // Writes log header to the csv file pointed by fp.
    fprintf(fp, "TimeStamp,TimeStamp_ms,Time_monotonic,");
    fprintf(fp, "LogRecType,L2 ID,CBR Percent,CPU_Util,");
    fprintf(fp, "TXInterval,msgCnt,TempId,GPGSAMode,");
    fprintf(fp, "secMark,lat,long,semi_major_dev,speed,");
    fprintf(fp, "heading,longAccel,latAccel,Tracking_Error,");
    fprintf(fp, "vehicleDensityInRange,ChannelQualityIndication,");
    fprintf(fp, "BSMValid,max_ITT,GPS-Time,Events,DCC random time,Hysterisis");
    // TODO add security headers here too
    fprintf(fp, "\n");
}

bool ApplicationBase::openLogFile(const std::string& fullPathName) {
    bool res = false;
    if (not enableCsvLog_) {
        return res;
    }
    {
        lock_guard<std::mutex> lock(csvMutex);
        if ((nullptr == csvfp) && !fullPathName.empty()) {
            csvfp = fopen(fullPathName.c_str(), "w+");
            if (!csvfp) {
                cerr << "Failed to open log file " << fullPathName << std::endl;
            } else {
                std::cout << "Open log " << fullPathName << " success!" << std::endl;
                res = true;
                writeLogHeader(csvfp);
            }
        }
    }
    if (res) {
        for(int index = 0 ; index < spsTransmits.size(); index++) {
            this->spsTransmits[index].enableCsvLog(enableCsvLog_);
        }
        for(int index = 0 ; index < eventTransmits.size(); index++) {
            this->eventTransmits[index].enableCsvLog(enableCsvLog_);
        }
        for(int index = 0 ; index < radioReceives.size(); index++) {
            this->radioReceives[index].enableCsvLog(enableCsvLog_);
        }
    }

    return res;
}


void ApplicationBase::writeLog(std::weak_ptr<msg_contents> mc, const uint8_t index,
    uint32_t l2SrcAddr, bool isTx, TransmitType txType, bool validPkt, uint64_t timestamp) {
    uint64_t periodicityMs = 0;
    int res = -1;
    uint64_t monotonicTime;
    struct timespec ts;
    uint64_t timestamp_now_ms = 0;
    uint8_t cbr;
    uint32_t RVsInRange;

    if (not csvfp) {
        return;
    }

    auto sp = mc.lock();
    // build a string and then only lock for just that part in writing to the file
    char tmpLogBuf[600] = "";
    char* curChar = tmpLogBuf;
    char* const endChar = tmpLogBuf + sizeof tmpLogBuf;
    char tmpLogStr[200] = "";

    if (sp) {
        // lock the critical section and write to the shared file
        RVsInRange = vehiclesInRange();
        timestamp_now_ms = timestamp_now();
        if (isTx) {
            if (txType == TransmitType::SPS) {
                cbr = spsTransmits[index].getCBRValue();
                monotonicTime = spsTransmits[index].latestTxRxTimeMonotonic();
            } else {
                cbr = eventTransmits[index].getCBRValue();
                monotonicTime = eventTransmits[index].latestTxRxTimeMonotonic();
            }
        } else {
            monotonicTime = radioReceives[index].latestTxRxTimeMonotonic();
            cbr = radioReceives[index].getCBRValue();
        }
        /* Log Format:
         * TimeStamp    TimeStamp_ms    Time_monotonic
         * LogRecType   L2 ID    CBR Percent    CPU_Util
         * TXInterval   msgCnt  TempId  GPGSAMode
         * secMark  lat long    semi_major_dev  speed
         * heading  longAccel   latAccel    Tracking_Error
         * vehicleDensityInRange    ChannelQualityIndication
         * BSMValid max_ITT GPS-Time    Events  DCC random time Hysterisis
         */

        // write general data to log
        // build the string in this function instead of immediately writing
        writeGeneralLog(tmpLogStr, 200, sp.get(), csvfp, isTx, periodicityMs, validPkt,
            RVsInRange, getCurrentTimestamp().c_str(), monotonicTime, timestamp,
            locPositionDop_, locNumSvUsed_, locTimeMs_, cbr, txInterval, l2SrcAddr);
        curChar += snprintf(curChar, endChar-curChar, "%s", tmpLogStr);

        // if congestion control enabled, write cong ctrl data to log
        memset(tmpLogStr, 0, sizeof(tmpLogStr));
        // may need to write these regardless
        if (this->configuration.enableCongCtrl && isTx) {
            // build the string in this function instead of immediately writing to file
            writeCongCtrlLog(tmpLogStr, 200, csvfp,
                congestionControlManager->
                    getCongestionControlUserData()->congestionControlCalculations,
                validPkt);
        }else{
            // make sure to write commas for the empty fields
            snprintf(tmpLogStr, 200, "0.0,0.0,0.0,%d,0.0,0.0,0,0.0,%d",
                validPkt ? 1 : 0, this->congCtrlConfig.spsEnhHysterPerc);
        }
        curChar += snprintf(curChar, endChar-curChar, "%s", tmpLogStr);
        //reset
        memset(tmpLogStr, 0, sizeof(tmpLogStr));
        /* Security related fields */
        // if security enabled, write security stats to log
        if (this->configuration.enableSecurity) {
            writeSecurityLog(tmpLogStr, 200, csvfp);
        }else {
            // todo: make sure to write commas for the empty fields
        }
        curChar += snprintf(curChar, endChar-curChar, "%s", tmpLogStr);

        lock_guard<std::mutex> lock(csvMutex);
        fprintf(csvfp, "%s\n", tmpLogBuf);
    }
}
