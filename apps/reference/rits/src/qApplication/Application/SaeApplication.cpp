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


 /**
  * @file: SaeApplication.cpp
  *
  * @brief: class for ITS stack application - SAE
  */
#include "SaeApplication.hpp"
#include <telux/cv2x/Cv2xRadioTypes.hpp>

// Each thread that is receiving and verifying will use this for logging purposes
thread_local int verifStatIdx = 0;
thread_local int verif_fails = 0;
thread_local std::vector<VerifStats> verifStats;
thread_local msg_contents* mc;
thread_local msg_contents msg_cont;
thread_local int rxFail = 0;
thread_local int txFail = 0;
thread_local int decFail = 0;
thread_local int encFail = 0;
thread_local int rxSuccess = 0;
thread_local int txSuccess = 0;
thread_local int verifFail = 0;
thread_local int verifSuccess = 0;
thread_local int signFail = 0;
thread_local int signSuccess = 0;


SaeApplication::SaeApplication(char *fileConfiguration,  MessageType msgType):
    ApplicationBase(fileConfiguration) {
    MsgType = msgType;

    wraInterval = std::chrono::milliseconds::zero();
    //init messages for sending.
    if (isTxSim) {
        initMsg(txSimMsg);
    }
    for (auto mc : eventContents) {
        initMsg(mc);
    }
    for (auto mc : spsContents) {
        initMsg(mc);
    }
    for (auto mc : receivedContents) {
        mc->stackId = STACK_ID_SAE;
    }
    sem_init(&this->rx_sem, 0, 1);
    sem_init(&this->log_sem, 0, 1);
}

SaeApplication::SaeApplication(const string txIpv4, const uint16_t txPort,
        const string rxIpv4, const uint16_t rxPort, 
        char* fileConfiguration, MessageType msgType) :
        ApplicationBase(txIpv4, txPort, rxIpv4, rxPort, fileConfiguration) {

    wraInterval = std::chrono::milliseconds::zero();
    MsgType = msgType;
    //init messages for sending.
    if (isTxSim) {
        initMsg(txSimMsg);
    }
    for (auto mc : eventContents) {
        initMsg(mc);
    }
    for (auto mc : spsContents) {
        initMsg(mc);
    }
    for (auto mc : receivedContents) {
        mc->stackId = STACK_ID_SAE;
    }
    sem_init(&this->rx_sem, 0, 1);
    sem_init(&this->log_sem, 0, 1);
}

SaeApplication::~SaeApplication() {
    printf("Total number of transmitted packets: %d\n",totalTxSuccess);
    printf("Total number of received packets: %d\n",totalRxSuccess);

    if (wraThread.joinable() == true) {
        wraThread.join();
    }
}

void SaeApplication::printRxStats(){
    sem_wait(&this->log_sem);
    std::thread::id tid = std::this_thread::get_id();
    printf("Thread (%04x) rx fails is: %d\n", tid, rxFail);
    printf("Thread (%04x) decode fails is: %d\n", tid, decFail);
    printf("Thread (%04x) rx successes is: %d\n", tid, rxSuccess);
    if(verifFail)
        printf("Thread (%04x) verif fails is: %d\n", tid, verifFail);
    if(verifSuccess)
        printf("Thread (%04x) verif success is: %d\n", tid, verifSuccess);
    totalRxSuccess+=rxSuccess;
    sem_post(&this->log_sem);
}

void SaeApplication::printTxStats(){
    printf("Printing tx stats\n");
    sem_wait(&this->log_sem);
    std::thread::id tid = std::this_thread::get_id();
    printf("Thread (%04x) tx fails is: %d\n", tid, txFail);
    printf("Thread (%04x) tx successes is: %d\n", tid, txSuccess);
    if(signFail)
        printf("Thread (%04x) sign fails is: %d\n", tid, signFail);
    if(signSuccess)
        printf("Thread (%04x) sign success is: %d\n", tid, signSuccess);
    totalTxSuccess+=txSuccess;
    sem_post(&this->log_sem);
}

int SaeApplication::receive(const uint8_t index, const uint16_t bufLen) {

    // Allocate msg_contents struct and copy actual packet into it
    const auto i = index;
    int ret = 0;
    int packet_len = 0;
    wsmp_data_t *wsmpp;
    uint8_t sourceMacAddr[CV2X_MAC_ADDR_LEN];
    int macAddrLen = CV2X_MAC_ADDR_LEN;

    if(&msg_cont.abuf == NULL || msg_cont.abuf.size == 0){
        abuf_alloc(&msg_cont.abuf, ABUF_LEN, ABUF_HEADROOM);
        // for SAE only
        msg_cont.stackId = STACK_ID_SAE;
        if(msg_cont.wsmp == nullptr)
            msg_cont.wsmp = new char[sizeof(wsmp_data_t)];
        if(msg_cont.ieee1609_2data == nullptr)
            msg_cont.ieee1609_2data = new char[sizeof(ieee1609_2_data)];
        if (MsgType == MessageType::BSM) {
            if(msg_cont.j2735_msg == nullptr)
                msg_cont.j2735_msg = new char[sizeof(bsm_value_t)];
            msg_cont.msgId = J2735_MSGID_BASIC_SAFETY;
        } else {
#ifdef WITH_WSA
            if (msg_cont.wsa == nullptr)
                msg_cont.wsa = new char[sizeof(SrvAdvMsg_t)];
            //if (msg_cont.wra == nullptr)
            //    msg_cont.wra = new char[sizeof(RoutingAdvertisement_t)];
            msg_cont.msgId = (int)WSA_MSG_ID;
#endif
        }
    }
    else{
        abuf_reset(&msg_cont.abuf, ABUF_HEADROOM);
    }
    mc = &msg_cont;

    // receive packet
    if (isRxSim)
    {
        sem_wait(&rx_sem);
        ret = simReceive->receive(msg_cont.abuf.data, ABUF_LEN-ABUF_HEADROOM);
        sem_post(&rx_sem);
        packet_len = ret;
    }
    else {
        sem_wait(&rx_sem);
        ret = radioReceives[0].receive(mc->abuf.data, ABUF_LEN-ABUF_HEADROOM,
                            sourceMacAddr, macAddrLen);
        sem_post(&rx_sem);
    }

    // Make sure packet is successfully received
    if(ret < MIN_PACKET_LEN || ret > MAX_PACKET_LEN || mc == nullptr){
        if(appVerbosity > 4){
            if(ret < 0){
                printf("Receive returned with error.\n");
            }else if(ret > 0 && ret < MIN_PACKET_LEN){
                printf(
                "Dropping packet with %d bytes. Needs to be at least %d bytes.\n",
                        ret, MIN_PACKET_LEN); 
            }else if(ret > 0 && ret >= MAX_PACKET_LEN){
                printf(
                "Dropping packet with %d bytes. Needs to be less than %d bytes.\n",
                        ret, MAX_PACKET_LEN);
            }
            // if ret is 0, then polling timed out
        }
        if(ret != 0) rxFail++;
        return -1;
    }

    // needs to be done for data pointer to not override tail pointer
    mc->abuf.tail = mc->abuf.data+ret;

    if(appVerbosity > 7){
       printf("\n 2) Full rx packet with length %d\n", ret);
       print_buffer((uint8_t*)mc->abuf.data, ret);
       printf("\n");
    }

    // Decode packet as WSMP Packet and IEEE 1609.2 Header
    ret = decode_msg(mc);
    // Determine if we are expecting signed packet or not
    if(this->configuration.enableSecurity){
        // check if the message is signed/encrypted IEEE1609.2 content.
        if (ret == 1) { // message is secured
            // Prepare for verification
            SecurityOpt sopt;
            sopt.psidValue = this->configuration.psid;
            if (this->configuration.sspLength)
                memcpy(sopt.sspValue, this->configuration.ssp,
                    this->configuration.sspLength);
            sopt.sspLength = this->configuration.sspLength;
            sopt.enableAsync = this->configuration.enableAsync;
            sopt.enableEnc  = this->configuration.enableEncrypt;
            sopt.secVerbosity = this->configuration.secVerbosity;
            std::thread::id tid = std::this_thread::get_id();
            if (thrVerifLatencies[tid].size() > verifStatIdx[tid]) {
                sopt.verifStat = &thrVerifLatencies[tid].at(verifStatIdx[tid]);
            }else{
                verifStatIdx[tid] = 0;
                sopt.verifStat = &thrVerifLatencies[tid].at(verifStatIdx[tid]);
            }

            uint32_t dot2HdrLen;
            // Verify packet signature
            ret = SecService->VerifyMsg(sopt,
            (uint8_t*)mc->l3_payload,(uint32_t)mc->l3_payload_len,dot2HdrLen);

            if(ret == -1){
                verifFail++;
            }
            else{
                verifSuccess++;
                // successful verification, increment the verif stat idx
                std::thread::id tid = std::this_thread::get_id();
                verifStatIdx[tid]++;
                verifStatIdx[tid]%=thrVerifLatencies[tid].size();
                mc->l3_payload=mc->l3_payload+dot2HdrLen;
                // ieee header is 3 bytes long typically
                abuf_pull(&mc->abuf, dot2HdrLen - IEEE_1609_2_HDR_LEN);
                mc->payload_len=ret;
                if(appVerbosity > 4){
                    printf("Total security header length is: %d bytes\n",
                            dot2HdrLen);
                    printf("payload length is %d bytes\n", ret);
                }
                wsmpp = (wsmp_data_t *)mc->wsmp;
                if (MsgType == MessageType::WSA && wsmpp->psid == PSID_WSA) {
#ifdef WITH_WSA
                    ret = decode_as_wsa(mc);
                    if (!ret && mc->wra) {
                        ret = onReceiveWra(
                                static_cast<RoutingAdvertisement_t*>(mc->wra),
                                sourceMacAddr, macAddrLen);
                    }
#endif
                } else {
                    ret = decode_as_j2735(mc);
                }
            }
        }else if(ret >= 0){
            if(appVerbosity > 3)
                printf("Error in decoding unsigned packet - security enabled.\n");
            ret = -1;
        }else{
            if(appVerbosity > 3)
                printf("Error in decoding packet\n");
            ret = -1;
        }
    }else{
        // determine if unsigned packet decoded properly
        switch(ret){
            case 0:
                if(appVerbosity > 3)
                    printf("Successful decode\n");
                ret = 0;
                wsmpp = (wsmp_data_t *)mc->wsmp;
                if (MsgType == MessageType::WSA && wsmpp->psid == PSID_WSA) {
#ifdef WITH_WSA
                    if (mc->wra) {
                        ret = onReceiveWra(
                                static_cast<RoutingAdvertisement_t*>(mc->wra), 
                                sourceMacAddr, macAddrLen);
                    }
#endif
                }
                break;
            case 1:
                if(appVerbosity > 3)
                    printf("Error in decoding packet. Expecting unsigned packet.\n");
                ret = -1;
                break;
            default:
                if(appVerbosity > 3)
                    printf("Error in decoding unsigned packet\n");
                ret = -1;
                break;
        }
    }
    if(ret >= 0)
        rxSuccess++;
    else
        decFail++;
    return ret;
}

int SaeApplication::receive(const uint8_t index, const uint16_t bufLen,
                     const uint32_t ldmIndex) {
    int ret = receive(index, bufLen);
    if (ret > 0) {
        auto bsm = reinterpret_cast<bsm_value_t *>(mc->j2735_msg);
        this->ldm->setIndex(bsm->id, ldmIndex);
    }
    return ret;
}

void SaeApplication::initMsg(std::shared_ptr<msg_contents> mc) {
    mc->stackId = STACK_ID_SAE;
    mc->wsmp = new char[sizeof(wsmp_data_t)];
    mc->ieee1609_2data = new char[sizeof(ieee1609_2_data)];

    if (MsgType == MessageType::BSM) {
        mc->j2735_msg = new char[sizeof(bsm_value_t)];
        mc->wsa = 0;
        mc->msgId = J2735_MSGID_BASIC_SAFETY;
    } else {
#ifdef WITH_WSA
        mc->wsa = new char[sizeof(SrvAdvMsg_t)];
        mc->wra = new char [sizeof(RoutingAdvertisement_t)];
        mc->j2735_msg = 0;
        mc->msgId = (int)WSA_MSG_ID;
#endif
    }

}

void SaeApplication::freeMsg(std::shared_ptr<msg_contents> mc) {
    if (mc->wsmp)
        delete static_cast<char *>(mc->wsmp);
    if (mc->j2735_msg)
        delete static_cast<char *>(mc->j2735_msg);
    if (mc->ieee1609_2data)
        delete static_cast<char *>(mc->ieee1609_2data);
    if (mc->wsa)
        delete static_cast<char *>(mc->wsa);
    if (mc->wra)
        delete static_cast<char *>(mc->wra);
}

void SaeApplication::fillMsg(std::shared_ptr<msg_contents> mc) {
    fillWsmp(static_cast<wsmp_data_t *>(mc->wsmp));
    fillSecurity(static_cast<ieee1609_2_data *>(mc->ieee1609_2data));

    if (MsgType == MessageType::BSM) {
        fillBsm(static_cast<bsm_value_t *>(mc->j2735_msg));
    }
    if (MsgType == MessageType::WSA) {
#ifdef WITH_WSA
        fillWsa(static_cast<SrvAdvMsg_t *>(mc->wsa),
                static_cast<RoutingAdvertisement_t *>(mc->wra));
        wsmp_data_t *wsmpp = (wsmp_data_t *)mc->wsmp;
        wsmpp->psid = PSID_WSA;
#endif
    }
}

void SaeApplication::fillWsmp(wsmp_data_t *wsmp) {
    memset(wsmp, 0, sizeof(wsmp_data_t));
    wsmp->n_header.data = 3;
    wsmp->tpid.octet = 0;
    if(!this->configuration.psid){
        wsmp->psid = this->configuration.psid;
    }else{
        wsmp->psid = PSID_BSM; // default 0x20
    }
    wsmp->chan_load_ptr = nullptr;
    wsmp->chan_load_len = 0;
}

int SaeApplication::parseIPv6Prefix(char *ipPrefix, int& bufLen)
{
    int i = 0;
    auto pos = 0, prev = 0;
    string prefixStr = configuration.ipPrefix + ":"; /* help parsing */
    do {
        if (i >= bufLen) {
            std::cout << "ipPrefix " << i << " too long" << std::endl;
            return -1;
        }
        pos = prefixStr.find(":", prev);
        if (pos != std::string::npos) {
            uint16_t val = stoi(prefixStr.substr(prev, pos), 0, 16);
            ipPrefix[i] = (val >> 8);
            ipPrefix[i + 1] = (val & 0xFF);
        }
        prev = pos + 1;
        i += 2;
    } while(pos != std::string::npos);

    bufLen = i - 2;

    return 0;
}
#ifdef WITH_WSA
void SaeApplication::fillWsa(SrvAdvMsg_t *wsa, RoutingAdvertisement_t *wra) {
    char ipPrefix[16];
    int prefixLen = 16;
    memset(wsa, 0, sizeof(SrvAdvMsg_t));
    wsa->version = 3;  /* 1609.3 2016 */
    wsa->body.routingAdvertisement = wra;
    memset(wra, 0, sizeof(RoutingAdvertisement_t));
    memset(ipPrefix, 0, 16);

    wra->lifetime = configuration.routerLifetime;

    if (parseIPv6Prefix(&ipPrefix[0], prefixLen) < 0) {
        return;
    }
    if (OCTET_STRING_fromBuf(&wra->ipPrefix, ipPrefix, 16) < 0) {
        if(appVerbosity > 3)
            std::cerr << "wra conversion failure for ipPrefix" << std::endl;
    }
    wra->ipPrefixLength = configuration.ipPrefixLength;

    /* Not actually using defaultGateway and primaryDns, but it can not be empty*/
    if (OCTET_STRING_fromBuf(&wra->defaultGateway, configuration.defaultGateway.c_str(),
                configuration.defaultGateway.length()) < 0) {
        if(appVerbosity > 3)
            std::cerr << "wra conversion failure for defaultGateway" << std::endl;
    }
    if (OCTET_STRING_fromBuf(&wra->primaryDns, configuration.primaryDns.c_str(),
                configuration.primaryDns.length()) < 0) {
        if(appVerbosity > 3)
            std::cerr << "wra conversion failure for primaryDns" << std::endl;
    }
}
#endif
void SaeApplication::fillBsm(bsm_value_t *bsm) {
    memset(bsm, 0, sizeof(bsm_value_t));
    srand(timestamp_now());
    fillBsmCan(bsm);
    fillBsmLocation(bsm);
    bsm->timestamp_ms = timestamp_now();

    if (bsm->id == 0) {
        bsm->id = rand();
    }

    bsm->VehicleLength_cm = configuration.vehicleLength;
    bsm->VehicleWidth_cm = configuration.vehicleWidth;
    if(configuration.enableVehicleExt==true){
        bsm->has_safety_extension = v2x_bool_t::V2X_True;
        bsm->has_supplemental_extension = v2x_bool_t::V2X_True;
    }else{
        bsm->has_safety_extension = v2x_bool_t::V2X_False;
        bsm->has_supplemental_extension = v2x_bool_t::V2X_False;
    }

    if (bsm->MsgCount == 0)
    {
        bsm->MsgCount = (rand() % 127) + 1;
    }
    else
    {
        bsm->MsgCount = (bsm->MsgCount + 1) % 127;
    }

    bsm->secMark_ms = bsm->timestamp_ms % 60000;
}


void SaeApplication::fillBsmCan(bsm_value_t *bsm)
{
    //fill data with values that may not make sense, these data should come from vehicle
    //CAN network

    bsm->TransmissionState = J2735_TRANNY_FORWARD_GEARS;

    bsm->has_safety_extension = (v2x_bool_t)1;
    bsm->vehsafeopts = (v2x_bool_t)(bsm->vehsafeopts | (1 << 3));
    bsm->events.data = (v2x_bool_t)0;

    bsm->SteeringWheelAngle = 0;
    bsm->brakes.word = 0;
    bsm->has_safety_extension = (v2x_bool_t)1;
    bsm->vehsafeopts = bsm->vehsafeopts | (1 << 0);
    bsm->lights_in_use.data = 0;

    bsm->has_supplemental_extension = (v2x_bool_t)1;
    bsm->suppvehopts |= (SUPPLEMENT_VEH_EXT_OPTION_WEATHER_PROBE);
    bsm->weatheropts = bsm->weatheropts | (1 << 0);
    bsm->statusFront = 0;

    bsm->wiperopts = bsm->wiperopts | (1 << 1);
}

void SaeApplication::fillBsmLocation(bsm_value_t *bsm) {
    shared_ptr<ILocationInfoEx> locationInfo = kinematicsReceive->getLocation();
    //ref_app code with the new telSDK Location
    bsm->Latitude = (locationInfo->getLatitude() * 10000000);
    bsm->Longitude = (locationInfo->getLongitude() * 10000000);
    bsm->Elevation = (locationInfo->getAltitude() * 10);

    bsm->SemiMajorAxisAccuracy = (locationInfo->getHorizontalUncertaintySemiMajor() * 20);

    bsm->SemiMinorAxisAccuracy = (locationInfo->getHorizontalUncertaintySemiMinor() * 20);

    bsm->SemiMajorAxisOrientation = (locationInfo->getHorizontalUncertaintyAzimuth() / 0.0054932479);

    bsm->Heading_degrees = (locationInfo->getHeading() / 0.0125);

    bsm->Speed = (50 * locationInfo->getSpeed());

    bsm->AccelLat_cm_per_sec_squared = (100 * locationInfo->getBodyFrameData().latAccel);

    bsm->AccelLon_cm_per_sec_squared = (100 * locationInfo->getBodyFrameData().longAccel);

    bsm->AccelVert_two_centi_gs = (locationInfo->getBodyFrameData().latAccel * 50); // / 0.1962);

    bsm->AccelYaw_centi_degrees_per_sec = (locationInfo->getBodyFrameData().yawRate * 100);
}
void SaeApplication::initRecordedBsm(bsm_value_t* bsm) {
    bsm->timestamp_ms = (uint64_t ) 0;
    bsm->MsgCount = (unsigned int ) 0;
    bsm->id = (unsigned int ) 0;
    bsm->secMark_ms = (unsigned int) 0;
    bsm->Latitude = (signed int) 0;
    bsm->Longitude = (signed int) 0;
    bsm->Elevation = (signed int) 0;
    bsm->SemiMajorAxisAccuracy = (unsigned int) 0;
    bsm->SemiMinorAxisAccuracy = (unsigned int) 0;
    bsm->SemiMajorAxisOrientation = (unsigned int) 0;
    bsm->TransmissionState = (j2735_transmission_state_e) 0;
    bsm->Speed = (unsigned int) 0;
    bsm->Heading_degrees = (unsigned int) 0;
    bsm->SteeringWheelAngle = (signed int) 0;
    bsm->AccelLon_cm_per_sec_squared = (signed int) 0;
    bsm->AccelLat_cm_per_sec_squared = (signed int) 0;
    bsm->AccelVert_two_centi_gs = (signed int) 0;
    bsm->AccelYaw_centi_degrees_per_sec = (signed int) 0;
    bsm->brakes.word = (uint16_t) 0;
    bsm->brakes.bits.unused_padding = (unsigned) 0;
    bsm->brakes.bits.aux_brake_status = (j2735_AuxBrakeStatus_e) 0;
    bsm->brakes.bits.brake_boost_applied = (j2735_BrakeBoostApplied_e) 0;
    bsm->brakes.bits.stability_control_status = (j2735_StabilityControlStatus_e)0;
    bsm->brakes.bits.antilock_brake_status = (j2735_AntiLockBrakeStatus_e) 0;
    bsm->brakes.bits.traction_control_status = (j2735_TractionControlStatus_e) 0;
    bsm->brakes.bits.rightRear = (unsigned) 0;
    bsm->brakes.bits.rightFront = (unsigned) 0;
    bsm->brakes.bits.leftRear = (unsigned) 0;
    bsm->brakes.bits.leftFront = (unsigned) 0;
    bsm->brakes.bits.unavailable = (unsigned) 0;
    bsm->VehicleWidth_cm = (unsigned int) 0;
    bsm->VehicleLength_cm = (unsigned int) 0;
    bsm->has_partII = (v2x_bool_t) 0;
    bsm->qty_partII_extensions = (int) 0;
    bsm->has_safety_extension = (v2x_bool_t) 0;
    bsm->has_special_extension = (v2x_bool_t) 0;
    bsm->has_supplemental_extension = (v2x_bool_t) 0;
    bsm->vehsafeopts = (int) 0;
    bsm->phopts = (int) 0;
    bsm->ph = (path_history_t) { 0 };
    bsm->pp.is_straight = (v2x_bool_t) 0;
    bsm->pp.radius = (signed int) 0;  // Radius of Curve in unis of 10cm
    bsm->pp.confidence = (uint8_t) 0;
    bsm->events.data = (uint16_t) 0;
    bsm->events.bits.eventAirBagDeployment = (unsigned) 0;
    bsm->events.bits.eventDisabledVehicle = (unsigned) 0;
    bsm->events.bits.eventFlatTire = (unsigned) 0;
    bsm->events.bits.eventWipersChanged = (unsigned) 0;
    bsm->events.bits.eventLightsChanged = (unsigned) 0;
    bsm->events.bits.eventHardBraking = (unsigned) 0;
    bsm->events.bits.eventReserved1 = (unsigned) 0;
    bsm->events.bits.eventHazardousMaterials = (unsigned) 0;
    bsm->events.bits.eventStabilityControlactivated = (unsigned) 0;
    bsm->events.bits.eventTractionControlLoss = (unsigned) 0;
    bsm->events.bits.eventABSactivated = (unsigned) 0;
    bsm->events.bits.eventStopLineViolation = (unsigned) 0;
    bsm->events.bits.eventHazardLights = (unsigned) 0;
    bsm->events.bits.unused = (unsigned) 0;
    bsm->lights_in_use.data = (uint16_t) 0;
    bsm->lights_in_use.bits.parkingLightsOn = (unsigned) 0;
    bsm->lights_in_use.bits.fogLightOn = (unsigned) 0;
    bsm->lights_in_use.bits.daytimeRunningLightsOn = (unsigned) 0;
    bsm->lights_in_use.bits.automaticLightControlOn = (unsigned) 0;
    bsm->lights_in_use.bits.hazardSignalOn = (unsigned) 0;
    bsm->lights_in_use.bits.rightTurnSignalOn = (unsigned) 0;
    bsm->lights_in_use.bits.leftTurnSignalOn = (unsigned) 0;
    bsm->lights_in_use.bits.highBeamHeadlightsOn = (unsigned) 0;
    bsm->lights_in_use.bits.lowBeamHeadlightsOn = (unsigned) 0;
    bsm->lights_in_use.bits.unused = (unsigned) 0;
    bsm->specvehopts = (int) 0;
    bsm->edopts = (uint32_t) 0;
    bsm->eventopts = (uint32_t) 0;
    bsm->vehicleAlerts.sspRights = (int) 0;
    bsm->vehicleAlerts.sirenUse = (int) 0;
    bsm->vehicleAlerts.lightsUse = (int) 0;
    bsm->vehicleAlerts.multi = (int) 0;
    bsm->vehicleAlerts.events.sspRights = (int) 0;
    bsm->vehicleAlerts.events.event = (int) 0;
    bsm->vehicleAlerts.responseType = (int) 0;
    bsm->description.typeEvent = (int) 0;
    bsm->description.size_desc = (int) 0;
    //bsm->description.desc = { 0 };//array of 8 ints so it is already initialized
    bsm->description.priority = (int) 0;
    bsm->description.heading = (int) 0;
    bsm->description.extent = (int) 0;
    bsm->description.size_reg = (int) 0;
    //bsm->description.regional = {0}//array of 4 ints so it is already initialized
    bsm->trailers.sspRights = (int) 0;  // 5 bits
    bsm->trailers.pivotOffset = (int) 0; // 11 bits
    bsm->trailers.pivotAngle = (int) 0;  // 8 bits
    bsm->trailers.pivots = (int) 0; // 1 bit. boolean
    bsm->trailers.size_trailer = (int) 0;
    //bsm->trailers.units = {0}// 8 tr_unit_dest array so it is already initialized
    bsm->suppvehopts = (int) 0;
    bsm->VehicleClass = (int) 0;
    bsm->veh.height_cm = (uint32_t) 0;
    bsm->veh.front_bumper_height_cm = (uint32_t) 0;
    bsm->veh.rear_bumper_height_cm = (uint32_t) 0;
    bsm->veh.mass_kg = (uint32_t) 0;
    bsm->veh.trailer_weight = (uint32_t) 0;
    bsm->veh.supplemental_veh_data_options.word = (uint8_t) 0;
    bsm->veh.supplemental_veh_data_options.bits.has_trailer_weight = (unsigned) 0;
    bsm->veh.supplemental_veh_data_options.bits.has_mass = (unsigned) 0;
    bsm->veh.supplemental_veh_data_options.bits.has_bumpers_heights = (unsigned) 0;
    bsm->veh.supplemental_veh_data_options.bits.has_height = (unsigned) 0;
    bsm->veh.supplemental_veh_data_options.bits.unused_padding = (unsigned) 0;
    bsm->weatheropts = (uint32_t) 0;
    bsm->wiperopts = (uint32_t) 0;
    bsm->airTemp = (int) 0;
    bsm->airPressure = (int) 0;
    bsm->rateFront = (int) 0;
    bsm->rateRear = (int) 0;
    bsm->statusFront = (int) 0;
    bsm->statusRear = (int) 0;
}

int SaeApplication::transmit(uint8_t index, std::shared_ptr<msg_contents>mc_,
                                int16_t bufLen, TransmitType txType) {
    int encLength = bufLen;
    int ret = -1;
    // let the transmit function handle the actual transmission
    if (encLength){
        // insert family ID of 0x01
        char *p = abuf_push(&mc_->abuf, 1);
        *p = 0x01;
        ret = ApplicationBase::transmit(index, mc_, encLength+1, txType);
    }
    if(ret > 0)
        txSuccess++;
    else
        txFail++;
    return ret;
}

void SaeApplication::receiveTuncBsm(const uint8_t index, const uint16_t bufLen, const uint32_t ldmIndex) {
    const auto i = index;
    float tunc = -1;
    // Still using raw pointer of msg in Ldm, will change to smart pointer
    // later.
    msg_contents *msg = &this->ldm->bsmContents[ldmIndex];
    if (isRxSim) {
        decode_msg(rxSimMsg.get());
    }
    else {
        decode_msg(msg);
    }

    const bsm_value_t *bsm = reinterpret_cast<bsm_value_t *>(msg->j2735_msg);
    if (this->ldm->tuncs.find(bsm->id) == this->ldm->tuncs.end()) {
        this->ldm->tuncs.insert(pair<uint32_t, float>(bsm->id, tunc));
    }
    else {
        this->ldm->tuncs[bsm->id] += tunc;
    }

}

void SaeApplication::sendTuncBsm(uint8_t index, TransmitType txType) {
    const auto i = index;
    auto encLength = 0;
    msg_contents* msg = NULL;
    float tunc = -1.0;
    auto debugVar = 0;
    std::shared_ptr<msg_contents> mc;
    switch (txType)
    {
    case TransmitType::SPS:
        mc = this->spsContents[i];
        fillMsg(mc);
        encLength = encode_msg(mc.get());
        this->spsTransmits[i].statusCheck(RadioType::TX); //This will give us the most up to date time uncertainty
        debugVar = this->spsTransmits[i].gCv2xStatus.timeUncertainty;
        tunc = ntohl(this->spsTransmits[i].gCv2xStatus.timeUncertainty);
        memcpy(mc->abuf.tail, &tunc, sizeof(float));
        abuf_put(&mc->abuf, sizeof(float));
        encLength += sizeof(float);
        this->spsTransmits[i].transmit(mc->abuf.data, encLength);
        break;
    case TransmitType::EVENT:
        mc = this->eventContents[i];
        fillMsg(mc);
        encLength = encode_msg(mc.get());
        this->spsTransmits[i].statusCheck(RadioType::TX); //This will give us the most up to date time uncertainty
        debugVar = this->spsTransmits[i].gCv2xStatus.timeUncertainty;
        tunc = ntohl(this->spsTransmits[i].gCv2xStatus.timeUncertainty);
        memcpy(mc->abuf.tail, &tunc, sizeof(float));
        abuf_put(&mc->abuf, sizeof(float));
        encLength += sizeof(float);
        this->eventTransmits[i].transmit(mc->abuf.data, encLength);
        break;
    default:
        break;
    }
}

#ifdef WITH_WSA
int SaeApplication::onReceiveWra(RoutingAdvertisement_t *wra, uint8_t *sourceMacAddr,
        int& macAdrLen) {
    std::lock_guard<std::mutex> lock(wramutex);
    telux::cv2x::IPv6AddrType IpPrefix;
    telux::cv2x::GlobalIPUnicastRoutingInfo RoutingInfo;
    int ret = 0;
    auto func = [&](int routerLifetime) {
        wraThreadFunc(routerLifetime);
    };

    if (GlobalIpSessionActive == true) {
        if (wraInterval == std::chrono::milliseconds::zero()) {
            //received the second WRA message, need to determine the period of the WRA, 
            //so that if within expected internal we didn't receive next WRA, we deem the 
            //OBU went out of range of the associated RSU.
            auto diff = std::chrono::high_resolution_clock::now() - now;
            wraInterval = std::chrono::duration_cast<std::chrono::milliseconds>(diff);
            if(appVerbosity > 3)
                cout << "wraInterval=" << wraInterval.count() << endl;
        }
        wraCv.notify_all();
        if (memcmp(sourceMacAddr, prevSourceMac, CV2X_MAC_ADDR_LEN)) {
            memcpy(RoutingInfo.destMacAddr, sourceMacAddr, CV2X_MAC_ADDR_LEN);
            memcpy(prevSourceMac, sourceMacAddr, CV2X_MAC_ADDR_LEN);
            if(appVerbosity > 3)
                std::cout << "Updating routing info" << endl;
            ret = radioReceives[0].setRoutingInfo(RoutingInfo);
        }
        return ret;
    }
    if (wra->ipPrefix.size > CV2X_IPV6_ADDR_ARRAY_LEN) {
        if(appVerbosity > 3)
            std::cerr << "Invalid ip prefix length received: " << 
                    wra->ipPrefix.size << endl;
        ret = -1;
    } else {
        //Received first valid WRA.
        now = std::chrono::high_resolution_clock::now();
        memcpy(IpPrefix.ipv6Addr, wra->ipPrefix.buf, wra->ipPrefix.size);
        IpPrefix.prefixLen = wra->ipPrefixLength;
        if(appVerbosity > 3)
            cout << "Setting Global IP address" << endl;
        memcpy(prevSourceMac, sourceMacAddr, CV2X_MAC_ADDR_LEN);
        ret = radioReceives[0].setGlobalIPInfo(IpPrefix);
        if (!ret) {
            memcpy(RoutingInfo.destMacAddr, sourceMacAddr, CV2X_MAC_ADDR_LEN);
            ret = radioReceives[0].setRoutingInfo(RoutingInfo);
            if (ret) {
                return ret;
            }
            //Launch Wra thread to monitor WRA timeout.
            if (wraThread.joinable() == false) {
                wraThread = std::thread(func, wra->lifetime);
                GlobalIpSessionActive = true;
            } else {
                if (GlobalIpSessionActive == false) {
                    wraThread.join();
                    wraThread = std::thread(func, wra->lifetime);
                    GlobalIpSessionActive = true;
                } else {
                    //Notify Wra thread we got new WRA message
                    wraCv.notify_all();
                }
            }
        }
    }

    return ret;
}
#endif
void SaeApplication::wraThreadFunc(int routerLifetime)
{
    std::unique_lock<std::mutex> lk(wraMutex);
    std::cv_status status;

    lk.unlock();
    while(true) {
        lk.lock();
        if (wraInterval == std::chrono::milliseconds::zero()) {
            // we didn't receive 2nd WRA yet, no idea about the WRA interval.
            // Use routerlifetime as wait time.
            std::chrono::milliseconds wt(routerLifetime*1000);
            status = wraCv.wait_for(lk, wt);
        } else {
            //if no WRA within 3*interval, deem it as out of range of RSU
            status = wraCv.wait_for(lk, 3*wraInterval);
        }
        lk.unlock();
        if (status == std::cv_status::timeout) {
            radioReceives[0].onWraTimedout();
            GlobalIpSessionActive = false;
            if(appVerbosity > 3)
                std::cout << "WRA timeout, global IP session stopped" << endl;
            return;
        }
    }
}

/* For RSU use case only */
int SaeApplication::setGlobalIPv6Prefix(void)
{
    char ipPrefix[16];
    int prefixLen = 16;
    telux::cv2x::IPv6AddrType IpPrefix;
    int ret = 0;

    if (GlobalIpSessionActive == false) {
        memset(ipPrefix, 0, 16);
        if (!parseIPv6Prefix(ipPrefix, prefixLen)) {
            IpPrefix.prefixLen = configuration.ipPrefixLength;
            memcpy(IpPrefix.ipv6Addr, ipPrefix, prefixLen);
            ret = radioReceives[0].setGlobalIPInfo(IpPrefix);
        }
        GlobalIpSessionActive = true;
    }

    return ret;
}

int SaeApplication::clearGlobalIPv6Prefix(void)
{
    GlobalIpSessionActive = false;
    return radioReceives[0].clearGlobalIPInfo();
}

