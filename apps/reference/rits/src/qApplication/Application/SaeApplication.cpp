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
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted (subject to the limitations in the
 *  disclaimer below) provided that the following conditions are met:
 *
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *
 *      * Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials provided
 *        with the distribution.
 *
 *      * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *        contributors may be used to endorse or promote products derived
 *        from this software without specific prior written permission.
 *
 *  NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 *  GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 *  HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 *  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 *  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 *  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

 /**
  * @file: SaeApplication.cpp
  *
  * @brief: class for ITS stack application - SAE
  */
#include "SaeApplication.hpp"
#include <telux/cv2x/Cv2xRadioTypes.hpp>
#include <fstream>
#include <sstream>
#include "asnbuf.h"
#include "wsmp.h"

// Each thread that is receiving and verifying will use this for logging purposes
thread_local int verifStatIdx = 0;
thread_local int misbehaviorStatIdx = 0;
thread_local int verif_fails = 0;
thread_local std::vector<MisbehaviorStats> misbehaviorStats;
thread_local std::vector<VerifStats> verifStats;
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
thread_local std::shared_ptr<msg_contents> threadMc = nullptr;
thread_local std::shared_ptr<msg_contents> hostMc = nullptr;

SaeApplication::SaeApplication(char *fileConfiguration,  MessageType msgType, bool enableCsvLog):
    ApplicationBase(fileConfiguration, msgType, enableCsvLog) {
    if (not configuration.isValid) {
        return;
    }

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
    if (isRxSim) {
        initMsg(rxSimMsg, true);
    }
    for (auto mc : receivedContents) {
        initMsg(mc, true);
    }

}

SaeApplication::SaeApplication(const string txIpv4, const uint16_t txPort,
        const string rxIpv4, const uint16_t rxPort,
        char* fileConfiguration, MessageType msgType, bool enableCsvLog) :
        ApplicationBase(txIpv4, txPort, rxIpv4, rxPort, fileConfiguration, enableCsvLog) {
    if (not configuration.isValid) {
        return;
    }
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
    if (isRxSim) {
        initMsg(rxSimMsg, true);
    }
    for (auto mc : receivedContents) {
        initMsg(mc, true);
    }
}

SaeApplication::~SaeApplication() {
    printf("Total number of transmitted packets: %d\n",totalTxSuccess);
    printf("Total number of received packets: %d\n",totalRxSuccess);

    // notify the wraThread to exit
    exit_ = true;
    {
        std::unique_lock<std::mutex> lk(wraMutex);
        wraCv.notify_all();
    }

    if (wraThread.joinable() == true) {
        wraThread.join();
    }
    if (isTxSim) {
        freeMsg(txSimMsg);
    }
    for (auto mc : eventContents) {
        freeMsg(mc);
    }
    for (auto mc : spsContents) {
        freeMsg(mc);
    }
    if (isRxSim) {
        freeMsg(rxSimMsg);
    }
    for (auto mc : receivedContents) {
        freeMsg(mc);
    }
    // delete default route in OBU if previously set
    deleteDefaultRouteInObu();
}

void SaeApplication::printRxStats() {
    printf("Printing rx stats\n");
    sem_wait(&this->log_sem);
    std::stringstream ss;
    ss << std::this_thread::get_id();
    int tid = (int)std::stoul(ss.str());
    printf("Thread (%08x) rx fails is: %d\n", tid, rxFail);
    printf("Thread (%08x) decode fails is: %d\n", tid, decFail);
    printf("Thread (%08x) rx successes is: %d\n", tid, rxSuccess);
    if (verifFail)
        printf("Thread (%08x) verif fails is: %d\n", tid, verifFail);
    if (verifSuccess)
        printf("Thread (%08x) verif success is: %d\n", tid, verifSuccess);
    totalRxSuccess+=rxSuccess;
    sem_post(&this->log_sem);
}

void SaeApplication::printTxStats() {
    printf("Printing tx stats\n");
    sem_wait(&this->log_sem);
    std::stringstream ss;
    ss << std::this_thread::get_id();
    int tid = (int)std::stoul(ss.str());
    printf("Thread (%08x) tx fails is: %d\n", tid, txFail);
    printf("Thread (%08x) tx successes is: %d\n", tid, txSuccess);
    if (signFail)
        printf("Thread (%08x) sign fails is: %d\n", tid, signFail);
    if (signSuccess)
        printf("Thread (%08x) sign success is: %d\n", tid, signSuccess);
    totalTxSuccess+=txSuccess;
    sem_post(&this->log_sem);
}

int SaeApplication::receive(const uint8_t index, const uint16_t bufLen) {
    const auto i = index;
    int ret = 0;
    int packet_len = 0;
    wsmp_data_t *wsmpp;
    uint8_t sourceMacAddr[CV2X_MAC_ADDR_LEN];
    int macAddrLen = CV2X_MAC_ADDR_LEN;
    /* Congestion Control Parameters */
    CongestionControlData congestionControlData_;
    bsm_value_t* rvBsm;
    uint32_t l2SrcAddr = 0;
    uint64_t timestamp = 0;
    std::thread::id tid = std::this_thread::get_id();

    // make sure that the threadMc is initialized
    if (threadMc == nullptr) {
        if (ldm == nullptr) {
            // allocate a new one since ldm is not active
            threadMc = std::make_shared<msg_contents>();
        } else {
            // use a ldm-provided msg contents struct for rx and decoding
            uint32_t ldmIndex = this->ldm->getFreeBsmSlotIdx();
            threadMc = this->ldm->bsmContents[ldmIndex];
        }
    }

    if (threadMc->abuf.head == NULL || threadMc->abuf.size == 0) {
        abuf_alloc(&threadMc->abuf, bufLen, ABUF_HEADROOM);
        initMsg(threadMc, true);
    } else {
        abuf_reset(&threadMc->abuf, ABUF_HEADROOM);
    }

    // receive packet
    if (isRxSim)
    {
        sem_wait(&rx_sem);
        ret = simReceive->receive(threadMc->abuf.data, bufLen-ABUF_HEADROOM);
        sem_post(&rx_sem);
        packet_len = ret;
    }
    else {
        sem_wait(&rx_sem);
        ret = radioReceives[index].receive(threadMc->abuf.data, bufLen-ABUF_HEADROOM,
                            sourceMacAddr, macAddrLen);
        sem_post(&rx_sem);
    }
    // actual moment that a packet has been received
    timestamp = timestamp_now();
    // Make sure packet is successfully received
    if (ret < MIN_PACKET_LEN || ret > MAX_PACKET_LEN || threadMc == nullptr) {
        if (appVerbosity > 4) {
            if (ret < 0) {
                printf("Receive returned with error.\n");
            } else if (ret > 0 && ret < MIN_PACKET_LEN) {
                printf(
                "Dropping packet with %d bytes. Needs to be at least %d bytes.\n",
                        ret, MIN_PACKET_LEN);
            } else if (ret > 0 && ret >= MAX_PACKET_LEN) {
                printf(
                "Dropping packet with %d bytes. Needs to be less than %d bytes.\n",
                        ret, MAX_PACKET_LEN);
            }
            // if ret is 0, then polling timed out
        }
        if (ret != 0) {
            rxFail++;
            //qMon->tData[tid].rxFails++;
        }
        return -1;
    }

    // needs to be done for data pointer to not override tail pointer
    threadMc->abuf.tail = threadMc->abuf.data+ret;

    if (appVerbosity > 7) {
       printf("\n 2) Full rx packet with length %d\n", ret);
       print_buffer((uint8_t*)threadMc->abuf.data, ret);
       printf("\n");
    }

    // Decode packet as WSMP Packet and IEEE 1609.2 Header
    ret = decode_msg(threadMc.get());

    if (configuration.enableL2Filtering && !isRxSim) {
        l2SrcAddr = radioReceives[index].msgL2SrcAdrr;
        if (appVerbosity >= 5) {
            std::cout << "L2 ID is " << l2SrcAddr << std::endl;
        }
        auto remote_bsm = reinterpret_cast<bsm_value_t *>(threadMc->j2735_msg);
        if (hostMc == nullptr) {
            try {
                hostMc = std::make_shared<msg_contents>();
            } catch (std::bad_alloc & e) {
                cerr << "Error: Create Host bsm failed!" << endl;
                return -1;
            }
        }
        if (hostMc->abuf.head == NULL || hostMc->abuf.size == 0) {
            abuf_alloc(&hostMc->abuf, ABUF_LEN, ABUF_HEADROOM);
            initMsg(hostMc);
        } else {
            abuf_reset(&hostMc->abuf, ABUF_HEADROOM);
        }

        fillBsm(reinterpret_cast<bsm_value_t *>(hostMc->j2735_msg));
        std::shared_ptr<rv_specs> rvsp;
        try {
            rvsp = std::make_shared<rv_specs>();
        } catch (std::bad_alloc & e) {
            cerr << "Error: Create rv specs failed!" << endl;
            return -1;
        }
        fill_RV_specs(hostMc.get(), threadMc.get(), rvsp.get());
        if (appVerbosity > 5) {
            print_rvspecs(rvsp.get());
        }
        this->updateL2RvMap(l2SrcAddr,rvsp.get());
    }

    // Determine if we are expecting signed packet or not
    if (this->configuration.enableSecurity) {
        // check if the message is signed/encrypted IEEE1609.2 content.
        if (ret == 1) { // message is secured
            ret = decodeAndVerify(threadMc.get());
        } else if (ret >= 0) {
            // here we need to check option for processing both unsigned/signed packets
            if (!configuration.acceptAll) {
                if (appVerbosity > 3)
                    printf("Error in decoding unsigned packet - security enabled.\n");
                ret = -1;
            } else {
                if (appVerbosity > 3)
                    printf("Decoded unsigned packet successfully.\n");
                // process WSA and other WSMP packets
                wsmpp = (wsmp_data_t *)threadMc->wsmp;
                if (MsgType == MessageType::WSA && wsmpp->psid == PSID_WSA) {
#ifdef WITH_WSA
                    ret = decode_as_wsa(threadMc.get());
                    if (!ret && threadMc->wra) {
                        ret = onReceiveWra(
                                static_cast<RoutingAdvertisement_t*>(threadMc->wra),
                                sourceMacAddr, macAddrLen);
                    }
#endif
                } else {
                    ret = decode_as_j2735(threadMc.get());
                }
            }
        } else {
            if (appVerbosity > 3)
                printf("Error in decoding packet\n");
            ret = -1;
        }
    } else {
        // determine if unsigned packet decoded properly
        switch (ret) {
            case 0:
                if (appVerbosity > 3)
                    printf("Successful decode\n");
                ret = 0;
                wsmpp = (wsmp_data_t *)threadMc->wsmp;
                if (MsgType == MessageType::WSA && wsmpp->psid == PSID_WSA) {
#ifdef WITH_WSA
                    if (threadMc->wra) {
                        ret = onReceiveWra(
                                static_cast<RoutingAdvertisement_t*>(threadMc->wra),
                                sourceMacAddr, macAddrLen);
                    }
#endif
                }
                break;
            case 1:
                if (appVerbosity > 3)
                    printf("Error in decoding packet. Expecting unsigned packet.\n");
                ret = -1;
                break;
            default:
                if (appVerbosity > 3)
                    printf("Error in decoding unsigned packet\n");
                ret = -1;
                break;
        }
    }
    wsmp_data_t* wsmpdata;
    uint32_t psid = 0;
    if (ret >= 0) {
        rxSuccess++;
        if (qMon)
        {
            qMon->tData[tid].totalRx++;
        }
        sem_wait(&this->log_sem);
        totalRxSuccessPerSecond++;
        sem_post(&this->log_sem);

        /*  uint64_t timestamp_ms;      // UTC Timestamp in milliseconds when bsm was creatd. computed from secmark_ms
            unsigned int MsgCount;      // Ranges from 0 - 127 in cyclic fashion.
            unsigned int id;            // 32 bit identifier
            unsigned int secMark_ms;    // No of milliseconds in a minute
            signed int   Latitude;      // Degrees * 10^7
            signed int   Longitude;     // Degrees * 10^7
            signed int   Elevation;     // Meters * 10

            unsigned int SemiMajorAxisAccuracy;         // val * 20
            unsigned int SemiMinorAxisAccuracy;         // val * 20
            unsigned int SemiMajorAxisOrientation;      // val/0.0054932479

            j2735_transmission_state_e TransmissionState;   // P,R,N,D,L (park etc..)
            unsigned int Speed;                     // value (in kmph) * 250/18
            unsigned int Heading_degrees;           // value (in degrees) / 0.0125
            signed int   SteeringWheelAngle;        // value (in degree) / 1.5
            signed int   AccelLon_cm_per_sec_squared;       // value (in m/sec2) / 0.01
        */
        if(threadMc.get()->wsmp){
            // check psid for bsm
            wsmpdata = (wsmp_data_t*)(threadMc.get()->wsmp);
            psid = wsmpdata->psid;
        }
        if(this->configuration.enableCongCtrl && this->congCtrlInitialized){
            /* If congestion control is enabled, we will pass the contents
                of the decoded/verified BSM to the cong ctrl library */
            if(psid == PSID_BSM){
                rvBsm = (bsm_value_t*)(threadMc.get()->j2735_msg);

                unsigned int rvTmpId = rvBsm->id;
                congestionControlManager->addCongestionControlData(
                    rvTmpId,rvBsm->Latitude/10000000.0,
                    rvBsm->Longitude / 10000000.0, rvBsm->Heading_degrees,
                    rvBsm->Speed, rvBsm->timestamp_ms,
                    rvBsm->MsgCount);
            }
        }
        if (MsgType == MessageType::BSM) {
            if (qMon)
            {
                qMon->tData[tid].rxBSMs++;
            }
            if (appVerbosity > 2)
            {
                printf("Decoded BSM Summary: \n");
                print_summary_RV(threadMc.get());
            }
        }
    } else {
        decFail++;
        if (qMon)
        {
            qMon->tData[tid].decodeFails++;
        }
    }

    ApplicationBase::writeLog(threadMc, index, l2SrcAddr, false, TransmitType::SPS,
        (ret >= 0) ? true : false, timestamp, psid);

    return ret;
}

int SaeApplication::receive(const uint8_t index, const uint16_t bufLen,
                     const uint32_t ldmIndex) {
    // use the ldm-provide message contents struct for rx and decoding
    threadMc = this->ldm->bsmContents[ldmIndex];
    int ret = receive(index, bufLen);
    if (ret >= 0) {
        if (threadMc->j2735_msg != nullptr) {
            auto bsm = reinterpret_cast<bsm_value_t *>(threadMc->j2735_msg);
            this->ldm->setIndex((uint32_t)bsm->id, ldmIndex, nullptr);
        }
    }
    return ret;
}

int SaeApplication::decodeAndVerify(msg_contents* mc) {
    int ret = -1;
    std::thread::id tid = std::this_thread::get_id();
    wsmp_data_t *wsmpp;
    uint8_t sourceMacAddr[CV2X_MAC_ADDR_LEN];
    int macAddrLen = CV2X_MAC_ADDR_LEN;
    // Prepare for verification
    SecurityOpt sopt;
    sopt.psidValue = this->configuration.psid;
    // sopt.psidValue = wsmpp->psid;
    if (this->configuration.sspLength)
        memcpy(sopt.sspValue, this->configuration.ssp,
            this->configuration.sspLength);
    sopt.sspLength = this->configuration.sspLength;
    sopt.enableAsync = this->configuration.enableAsync;
    sopt.enableConsistency = this->configuration.enableConsistency;
    sopt.enableRelevance = this->configuration.enableRelevance;
    sopt.enableEnc  = this->configuration.enableEncrypt;
    sopt.secVerbosity = this->configuration.secVerbosity;
    uint32_t dot2HdrLen;
    uint8_t const *payload = NULL;
    uint32_t       payloadLen = 0;
    // extract the PDU from the secured packet
    ret = SecService->ExtractMsg(sopt,
        (uint8_t*)mc->l3_payload,mc->l3_payload_len,
        payload, payloadLen,
        dot2HdrLen);
    if (ret == -1) {
        printf("Error in extracting security header from signed packet.\n");
        verifFail++;
        if (qMon)
        {
            qMon->tData[tid].secFails++;
        }
        return -1;
    }

    // ieee header is 3 bytes long typically
    abuf_pull(&mc->abuf, dot2HdrLen - IEEE_1609_2_HDR_LEN);
    mc->l3_payload=mc->l3_payload+dot2HdrLen;
    mc->payload_len=payloadLen;
    if (appVerbosity > 4) {
        printf("Total security header length is: %d bytes\n",
                dot2HdrLen);
        printf("payload length is %d bytes\n", ret);
    }
    wsmpp = (wsmp_data_t *)mc->wsmp;
    if (MsgType == MessageType::BSM && wsmpp->psid == PSID_BSM) {
        ret = decode_as_j2735(mc);
        // here the secure header was extracted properly, packet decoded incorrectly
        if (ret == -1) {
            if (appVerbosity > 3)
                printf("Error in decoding unsigned packet - security enabled.\n");
            decFail++;
            if (qMon)
            {
                qMon->tData[tid].decodeFails++;
            }
            return -1;
        }
    }
    // if a bsm is decoded properly, need to extract the lat/lon from the packet (if bsm)
    if (mc->j2735_msg != nullptr) {
        bsm_value_t* bsm = (bsm_value_t*)mc->j2735_msg;
        sopt.rvKine.latitude = bsm->Latitude;
        sopt.rvKine.longitude = bsm->Longitude;
        sopt.rvKine.elevation = bsm->Elevation;
        if (configuration.enableMbd) {
            sopt.enableMbd = configuration.enableMbd;
            sopt.rvKine.id = bsm->id;
            sopt.rvKine.dataType = this->configuration.psid;
            sopt.rvKine.msgCount = bsm->MsgCount;
            sopt.rvKine.speed = bsm->Speed;
            sopt.rvKine.heading = bsm->Heading_degrees;
            sopt.rvKine.longitudeAcceleration = bsm->AccelLon_cm_per_sec_squared;
            sopt.rvKine.latitudeAcceleration = bsm-> AccelLat_cm_per_sec_squared;
            sopt.rvKine.yawAcceleration = bsm->AccelYaw_centi_degrees_per_sec;
            sopt.rvKine.brakes = (uint16_t)bsm->brakes.word;
        }
    }
    // set the hv kinematics
    shared_ptr<ILocationInfoEx> locationInfo;
    if(configuration.enableLocationFixes && kinematicsReceive && appLocListener_){
        auto locationInfo = appLocListener_->getLocation();
        if (locationInfo) {
            sopt.hvKine.latitude = (locationInfo->getLatitude() * 10000000);
            sopt.hvKine.longitude = (locationInfo->getLongitude() * 10000000);
            sopt.hvKine.elevation = (locationInfo->getAltitude() * 10);
        }
    }
    // prepare verification statistics logging
    if (configuration.enableVerifStatLog) {
        std::thread::id tid = std::this_thread::get_id();
        if (thrVerifLatencies[tid].size() >= verifStatIdx[tid]) {
            sopt.verifStat = &thrVerifLatencies[tid].at(verifStatIdx[tid]);
        } else {
            verifStatIdx[tid] = 0;
            sopt.verifStat = &thrVerifLatencies[tid].at(verifStatIdx[tid]);
        }
        verifStatIdx[tid]++;
        verifStatIdx[tid]%=thrVerifLatencies[tid].size();
    } else {
        sopt.verifStat = nullptr;
    }

    if (configuration.enableMbdStatLog) {
        std::thread::id tid = std::this_thread::get_id();
        if (thrMisbehaviorLatencies[tid].size() >= misbehaviorStatIdx[tid]) {
            sopt.misbehaviorStat =
                &thrMisbehaviorLatencies[tid].at(misbehaviorStatIdx[tid]);
        } else {
            misbehaviorStatIdx[tid] = 0;
            sopt.misbehaviorStat =
                &thrMisbehaviorLatencies[tid].at(misbehaviorStatIdx[tid]);
        }
        misbehaviorStatIdx[tid]++;
        misbehaviorStatIdx[tid]%=thrMisbehaviorLatencies[tid].size();
    } else {
        sopt.misbehaviorStat = nullptr;
    }
    // Verify packet signature ; providing lat/lon from the rx message
    ret = SecService->VerifyMsg(sopt);
    if (ret == -1) {
        verifFail++;
        if (qMon)
        {
            qMon->tData[tid].secFails++;
        }
        if (appVerbosity > 3)
            printf("Error in verifying secured packet.\n");
        ret = -1;
    } else {
        verifSuccess++;
        // process WSA and other WSMP packets after verification
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
        }
    }
    return ret;
}


void SaeApplication::initMsg(std::shared_ptr<msg_contents> mc, bool isRx) {
    mc->stackId = STACK_ID_SAE;

    if (isRx) {
        // not allocate memory for Rx
        mc->wsmp = nullptr;
        mc->ieee1609_2data = nullptr;
        mc->j2735_msg = nullptr;
        mc->wsa = nullptr;
        if (MsgType == MessageType::BSM) {
            mc->msgId = J2735_MSGID_BASIC_SAFETY;
        } else {
            mc->msgId = (int)WSA_MSG_ID;
        }
    } else {
        mc->wsmp = (wsmp_data_t*) calloc(1, sizeof(wsmp_data_t));
        if (!mc->wsmp) {
            std::cerr << "calloc wsmp failed" << endl;
            return;
        }

        ((wsmp_data_t*)mc->wsmp)->abp = (abuf_t*) calloc(1, sizeof(abuf_t));

        if (!((wsmp_data_t*)mc->wsmp)->abp)
        {
            std::cerr << "calloc wsmp asnbuf structure failed" << endl;
            return;
        }

        const auto abuf_ret = abuf_alloc(((wsmp_data_t*)mc->wsmp)->abp,
                        WSMP_ABUF_DEFAULT_SIZE, WSMP_ABUF_DEFAULT_HEADROOM);

        if (abuf_ret != WSMP_ABUF_DEFAULT_SIZE)
        {
            std::cerr << "alloc wsmp asn buffer failed" << endl;
            return;
        }

        mc->ieee1609_2data = malloc(sizeof(ieee1609_2_data));
        if (!mc->ieee1609_2data) {
            std::cerr << "alloc ieee1609_2data failed" << endl;
            return;
        }
        memset(mc->ieee1609_2data, 0, sizeof(ieee1609_2_data));

        if (MsgType == MessageType::BSM) {
            mc->j2735_msg = malloc(sizeof(bsm_value_t));
            if (!mc->j2735_msg) {
                std::cerr << "alloc j2735_msg failed" << endl;
                return;
            }
            memset(mc->j2735_msg, 0, sizeof(bsm_value_t));
            mc->wsa = 0;
            mc->msgId = J2735_MSGID_BASIC_SAFETY;
        } else {
#ifdef WITH_WSA
            mc->wsa = malloc(sizeof(SrvAdvMsg_t));
            if (!mc->wsa) {
                std::cerr << "alloc wsa failed" << endl;
                return;
            }
            memset(mc->wsa, 0, sizeof(SrvAdvMsg_t));

            mc->wra = malloc(sizeof(RoutingAdvertisement_t));
            if (!mc->wra) {
                std::cerr << "alloc wra failed" << endl;
                return;
            }
            memset(mc->wra, 0, sizeof(RoutingAdvertisement_t));

            // set wra into wsa struct, wra be freed along with wsa
            SrvAdvMsg_t* wsa = (SrvAdvMsg_t*)(mc->wsa);
            wsa->body.routingAdvertisement = (RoutingAdvertisement_t*)(mc->wra);
            mc->j2735_msg = 0;
            mc->msgId = (int)WSA_MSG_ID;
#endif
        }
    }
}

void SaeApplication::freeMsg(std::shared_ptr<msg_contents> mc) {
    if (mc->wsmp) {
        auto wsmp = (wsmp_data_t*)mc->wsmp;

        if (wsmp->chan_load_ptr) {
            free(wsmp->chan_load_ptr);
            wsmp->chan_load_ptr = nullptr;
        }

        abuf_free(wsmp->abp);
        free(mc->wsmp);
        mc->wsmp = nullptr;
    }

    if (mc->j2735_msg) {
        free(mc->j2735_msg);
        mc->j2735_msg = nullptr;
    }

    if (mc->ieee1609_2data) {
        free(mc->ieee1609_2data);
        mc->ieee1609_2data = nullptr;
    }
#ifdef WITH_WSA
    // free the wsa struct, wra will be freed if it exists
    if (mc->wsa) {
        free_wsa(mc->wsa);
        mc->wsa = nullptr;
    }
#endif
    abuf_free(&(mc->abuf));
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
    wsmp->n_header.data = 3;
    wsmp->tpid.octet = 0;
    if (this->configuration.psid) {
        wsmp->psid = this->configuration.psid;
    } else {
        wsmp->psid = PSID_BSM; // default 0x20
    }

    // The content of channel load is not standardized yet, use this IE for padding
    if (MsgType == MessageType::BSM and this->configuration.padding > 0) {
        if (!wsmp->chan_load_ptr) {
            wsmp->chan_load_ptr = (uint8_t *)malloc(this->configuration.padding);
            if (!wsmp->chan_load_ptr) {
                cerr << "alloc padding failed!" << endl;
            } else {
                wsmp->weid_opts.inc_load_ext = 1;
                wsmp->chan_load_len = this->configuration.padding;
                // fill 0xFF for padding
                memset(wsmp->chan_load_ptr, 0xFF, wsmp->chan_load_len);
            }
        }
    } else {
        wsmp->chan_load_ptr = nullptr;
        wsmp->chan_load_len = 0;
    }
}

int SaeApplication::parseIPv6Addr(const string& str, char *buf, int& bufLen) {
    int i = 0;
    auto pos = 0, prev = 0;

    if (str.empty() or bufLen < 0) {
        cerr << "Input error for parseIPv6Addr!" << endl;
        return -1;
    }

    string ipAddr = str + ":"; /* help parsing */
    do {
        if (i >= bufLen) {
            cerr << "Input IPv6 Address too long!" << endl;
            return -1;
        }
        pos = ipAddr.find(":", prev);
        if (pos == std::string::npos or pos == prev) {
            break;
        }
        string sub = ipAddr.substr(prev, pos - prev);
        if (sub.size() > 4) {
            cerr << "sub string " << sub << " too long!" << endl;
            return -1;
        }
        uint16_t val = stoi(sub, 0, 16);
        buf[i] = (val >> 8);
        buf[i + 1] = (val & 0xFF);
        prev = pos + 1;
        i += 2;
    } while (prev < ipAddr.size());

    bufLen = i;

    return 0;
}

int SaeApplication::getDefaultGWAddrInRsu(char *buf, int& len) {
    if (!buf or len <= 0 or len > CV2X_IPV6_ADDR_ARRAY_LEN) {
        cerr << "Input error for getDefaultGWAddrInRsu!"<< endl;
        return -1;
    }

    string strAddr;
    if (configuration.defaultGateway.empty()) {
        // If the static defaultGateway is not configured, get V2X IP rmnet addr
        if (0 != getV2xIpIfaceAddr(strAddr)) {
            std::cerr << "retrieve V2X IP addr error!" << endl;
            return -1;
        }
    } else {
        strAddr = configuration.defaultGateway;
    }

    if (appVerbosity > 3) {
        cout << "GW:" << strAddr << endl;
    }

    // parse the addr
    return parseIPv6Addr(strAddr, buf, len);
}

int SaeApplication::convertIpv6Addr2Str(char* buf, int bufLen, string& addr) {
    if (not buf or bufLen <= 0 or bufLen > CV2X_IPV6_ADDR_ARRAY_LEN) {
        cerr << "Input error for convertIpv6Addr2Str!" << endl;
        return -1;
    }

    std::ostringstream ss;
    for (int i = 0; i + 1 < bufLen; i += 2) {
        // add colon in front if it's not the first element
        if (0 != i) {
            ss << ':';
        }

        uint16_t val = buf[i] << 8 | buf[i + 1];
        ss << std::hex << val;
    }
    addr = ss.str();
    return 0;
}

int SaeApplication::setDefaultRouteInObu(string addr) {
    // delelte default route if already exist
    deleteDefaultRouteInObu();

    // set default route of OBU to the defaultGateway received from RSU
    string cmd = "ip -6 route add default via " + addr;
    FILE *fp;
    fp = popen(cmd.c_str(), "r");
    if (not fp) {
        cerr << "popen failed when set default route!" << endl;
        return -1;
    }
    if (pclose(fp) < 0) {
        cerr << "Set default route failed!" << endl;
        return -1;
    }
    obuRouteSet_ = true;
    if (appVerbosity > 3) {
        cout << "Set default route " << addr << endl;
    }
    return 0;
}

int SaeApplication::deleteDefaultRouteInObu() {
    if (not obuRouteSet_) {
        return 0;
    }

    FILE *fp;
    fp = popen("ip -6 route del default", "r");
    if (not fp) {
        cerr << "popen failed when delete default route!" << endl;
        return -1;
    }
    if (pclose(fp) < 0) {
        cerr << "Delete default route failed!" << endl;
        return -1;
    }

    if (appVerbosity > 3) {
        cout << "Delete default route" << endl;
    }
    return 0;
}

#ifdef WITH_WSA
int SaeApplication::storeWraInfoInObu(RoutingAdvertisement_t* wra) {
    if (not wra) {
        cerr << "Input error for storeWraInfoInObu" << endl;
        return -1;
    }

    // convert defaultGateway and primaryDns to readable format
    string addr1, addr2;
    if (convertIpv6Addr2Str((char *)wra->defaultGateway.buf, wra->defaultGateway.size, addr1)
        or convertIpv6Addr2Str((char *)wra->primaryDns.buf, wra->primaryDns.size, addr2)) {
        cerr << "convert gateway or DNS error" << endl;
        return -1;
    }

    if (appVerbosity > 3) {
        cout << "defaultGateway = " << addr1 << " primaryDns = " << addr2 << endl;
    }

    // check if the RSU gateway or primary DNS addr has changed
    if (not rsuGateway_.empty() and 0 == addr1.compare(rsuGateway_)
        and not rsuPrimaryDns_.empty() and 0 == addr2.compare(rsuPrimaryDns_)) {
        if (appVerbosity > 3) {
            cout << "RSU address not changed." << endl;
        }
        return 0;
    }

    // store the updated addr to the configured file
    ofstream file(configuration.wsaInfoFile);
    if (!file) {
        cerr << "Failed to create wsa file!" << endl;
        return -1;
    }

    file << "defaultGateway = " << addr1 << endl;
    file << "primaryDns = " << addr2;
    rsuGateway_ = addr1;
    rsuPrimaryDns_ = addr2;

    // set default route to the RSU rmnet addr
    // The DNS address is not actually used for now
    return setDefaultRouteInObu(rsuGateway_);
}

void SaeApplication::fillWsa(SrvAdvMsg_t *wsa, RoutingAdvertisement_t *wra) {
    memset(wsa, 0, sizeof(SrvAdvMsg_t));
    wsa->version = 3;  /* 1609.3 2016 */
    wsa->body.routingAdvertisement = wra;
    memset(wra, 0, sizeof(RoutingAdvertisement_t));

    wra->lifetime = configuration.routerLifetime;

    /* Fill in the IPv6 prefix */
    char ipAddr[CV2X_IPV6_ADDR_ARRAY_LEN] = {0};
    int addrLen = CV2X_IPV6_ADDR_ARRAY_LEN;
    if (parseIPv6Addr(configuration.ipPrefix, ipAddr, addrLen) < 0) {
        cerr << " Parse IPv6 prefix error" << endl;
        return;
    }
    if (OCTET_STRING_fromBuf(&wra->ipPrefix, ipAddr, CV2X_IPV6_ADDR_ARRAY_LEN) < 0) {
        if (appVerbosity > 3)
            std::cerr << "wra conversion failure for ipPrefix" << std::endl;
    }
    wra->ipPrefixLength = configuration.ipPrefixLength;

    /* Fill in the defaultGateway, it's either from the configuration or the dynamic rmnet addr */
    addrLen = CV2X_IPV6_ADDR_ARRAY_LEN;
    memset(ipAddr, 0, CV2X_IPV6_ADDR_ARRAY_LEN);
    if (0 == getDefaultGWAddrInRsu(ipAddr, addrLen)) {
        if (OCTET_STRING_fromBuf(&wra->defaultGateway, ipAddr, addrLen) < 0) {
            if (appVerbosity > 3)
                std::cerr << "wra conversion failure for defaultGateway" << std::endl;
        }
    }

    /* Fill in the primary DNS */
    addrLen = CV2X_IPV6_ADDR_ARRAY_LEN;
    memset(ipAddr, 0, CV2X_IPV6_ADDR_ARRAY_LEN);
    if (parseIPv6Addr(configuration.primaryDns, ipAddr, addrLen) < 0) {
        cerr << " Parse primary DNS error" << endl;
        return;
    }
    if (OCTET_STRING_fromBuf(&wra->primaryDns, ipAddr, CV2X_IPV6_ADDR_ARRAY_LEN) < 0) {
        if (appVerbosity > 3)
            std::cerr << "wra conversion failure for primaryDns" << std::endl;
    }
}
#endif
void SaeApplication::fillBsm(bsm_value_t *bsm) {
    bool idChangeEnabled = false;
    memset(bsm, 0, sizeof(bsm_value_t));
    srand(timestamp_now());
    fillBsmCan(bsm);
    fillBsmLocation(bsm);

    bsm->timestamp_ms = timestamp_now();
    bsm->VehicleLength_cm = configuration.vehicleLength;
    bsm->VehicleWidth_cm = configuration.vehicleWidth;
    if (configuration.enableVehicleExt==true) {
        bsm->has_safety_extension = v2x_bool_t::V2X_True;
        bsm->has_supplemental_extension = v2x_bool_t::V2X_True;
    } else {
        bsm->has_safety_extension = v2x_bool_t::V2X_False;
        bsm->has_supplemental_extension = v2x_bool_t::V2X_False;
    }
    bsm->secMark_ms = bsm->timestamp_ms % 60000;
    // needs to be randomized along with l2 address and msg id and pseudonym cert

    // check if msg count has been randomized and we haven't updated this yet
    // if so, keep adding and modding 128
    if (!this->configuration.lcmName.empty() &&
        this->configuration.idChangeInterval) {
        idChangeEnabled = true;
    }

    if (idChangeEnabled) {
        sem_wait(&idChangeData.idSem);
    }
        // for synchronization between Application and Aerolink sides
    if (!initialized) {
        bsm->MsgCount = (rand() % 128);
        bsm->id = rand();
        initialized = true;

        if (appVerbosity > 1) {
            printf("Msg count: %d, id: %u\n", bsm->MsgCount, bsm->id);
        }
    }
    else if (idChangeData.idChanged) {
        // randomize msg count
        bsm->MsgCount = (rand() % 128);
        // update the temp id
        bsm->id = (uint32_t)idChangeData.tempId[0] << 24 |
        (uint32_t)idChangeData.tempId[1] << 16 |
        (uint32_t)idChangeData.tempId[2] << 8  |
        (uint32_t)idChangeData.tempId[3];
        idChangeData.idChanged = false;
        if (appVerbosity > 1)
            printf("Id changed, new msgcount is: %d, and new temp id is: %u\n",
                                    bsm->MsgCount, bsm->id);
        this->tempId = bsm->id;
    }
    else{
        bsm->MsgCount = (msgCount + 1) % 128;
        bsm->id = tempId;
    }
    if (idChangeEnabled) {
        sem_post(&idChangeData.idSem);
    }

    msgCount = bsm->MsgCount;
    tempId = bsm->id;
}


void SaeApplication::fillBsmCan(bsm_value_t *bsm)
{
    //fill data with values that may not make sense, these data should come from vehicle
    //CAN network

    // NEED TO FIX THIS!!
    bsm->TransmissionState = J2735_TRANNY_FORWARD_GEARS;

    if(criticalState){
        bsm->has_partII = (v2x_bool_t)1;
    }

    bsm->has_safety_extension = (v2x_bool_t)1;
    bsm->vehsafeopts = (v2x_bool_t)(bsm->vehsafeopts | (1 << 3));
    bsm->events.data = 0;

    if(this->currVehState != NULL){
        // for each flag, set the corresponding one in the bsm
        bsm->events.bits.eventAirBagDeployment =
            this->currVehState->events.bits.eventAirBagDeployment;
        bsm->events.bits.eventDisabledVehicle =
            this->currVehState->events.bits.eventDisabledVehicle;
        bsm->events.bits.eventFlatTire =
            this->currVehState->events.bits.eventFlatTire;
        bsm->events.bits.eventWipersChanged =
            this->currVehState->events.bits.eventWipersChanged;
        bsm->events.bits.eventLightsChanged =
            this->currVehState->events.bits.eventLightsChanged;
        bsm->events.bits.eventHardBraking =
            this->currVehState->events.bits.eventHardBraking;
        bsm->events.bits.eventHazardousMaterials =
            this->currVehState->events.bits.eventHazardousMaterials;
        bsm->events.bits.eventStabilityControlactivated =
            this->currVehState->events.bits.eventStabilityControlactivated;
        bsm->events.bits.eventTractionControlLoss =
            this->currVehState->events.bits.eventTractionControlLoss;
        bsm->events.bits.eventABSactivated =
            this->currVehState->events.bits.eventABSactivated;
        bsm->events.bits.eventStopLineViolation =
            this->currVehState->events.bits.eventStopLineViolation;
        bsm->events.bits.eventHazardLights =
            this->currVehState->events.bits.eventHazardLights;
        bsm->events.bits.unused = 0;
        bsm->events.bits.eventReserved1 = 0;
    }
}

void SaeApplication::fillBsmLocation(bsm_value_t *bsm) {
    if(!configuration.enableLocationFixes || !kinematicsReceive ||
        !appLocListener_){
        return;
    }
    shared_ptr<ILocationInfoEx> locationInfo = appLocListener_->getLocation();
    if(!locationInfo){
        std::cout << "Invalid location info\n";
        return;
    }
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
    std::thread::id tid = std::this_thread::get_id();
    int encLength = bufLen;
    int ret = -1;
    // let the transmit function handle the actual transmission
    if (encLength) {
        // insert family ID of 0x01
        char *p = abuf_push(&mc_->abuf, 1);
        if (p != NULL) {
            *p = 0x01;
            ret = ApplicationBase::transmit(index, mc_, encLength+1, txType);
        }
    }
    if (ret > 0){
        txSuccess++;
        if (qMon)
        {
            qMon->tData[tid].totalTx++;
        }
    }
    else{
        txFail++;
    }
    return ret;
}

void SaeApplication::receiveTuncBsm(const uint8_t index, const uint16_t bufLen, const uint32_t ldmIndex) {
    const auto i = index;
    float tunc = -1;
    threadMc = this->ldm->bsmContents[ldmIndex];
    if (isRxSim) {
        threadMc = rxSimMsg;
    }
    decode_msg(threadMc.get());

    const bsm_value_t *bsm = reinterpret_cast<bsm_value_t *>(threadMc->j2735_msg);
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
        // SPS priority is set when creating the flow
        this->spsTransmits[i].transmit(mc->abuf.data, encLength, Priority::PRIORITY_UNKNOWN);
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
        // event priority is set per packet using traffic class
        this->eventTransmits[i].transmit(mc->abuf.data, encLength, configuration.eventPriority);
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
    auto func = [this](int routerLifetime) {
        wraThreadFunc(routerLifetime);
    };

    if (GlobalIpSessionActive == true) {
        if (wraInterval == std::chrono::milliseconds::zero()) {
            //received the second WRA message, need to determine the period of the WRA,
            //so that if within expected internal we didn't receive next WRA, we deem the
            //OBU went out of range of the associated RSU.
            auto diff = std::chrono::high_resolution_clock::now() - now;
            wraInterval = std::chrono::duration_cast<std::chrono::milliseconds>(diff);
            if (appVerbosity > 3)
                cout << "wraInterval=" << wraInterval.count() << endl;
        }
        wraCv.notify_all();
        if (memcmp(sourceMacAddr, prevSourceMac, CV2X_MAC_ADDR_LEN)) {
            memcpy(RoutingInfo.destMacAddr, sourceMacAddr, CV2X_MAC_ADDR_LEN);
            memcpy(prevSourceMac, sourceMacAddr, CV2X_MAC_ADDR_LEN);
            if (appVerbosity > 3)
                std::cout << "Updating routing info" << endl;
            ret = radioReceives[0].setRoutingInfo(RoutingInfo);
        }
    } else {
        if (wra->ipPrefix.size > CV2X_IPV6_ADDR_ARRAY_LEN) {
            if (appVerbosity > 3)
                std::cerr << "Invalid ip prefix length received: " <<
                        wra->ipPrefix.size << endl;
            ret = -1;
        } else {
            //Received first valid WRA.
            now = std::chrono::high_resolution_clock::now();
            memcpy(IpPrefix.ipv6Addr, wra->ipPrefix.buf, wra->ipPrefix.size);
            IpPrefix.prefixLen = wra->ipPrefixLength;
            if (appVerbosity > 3)
                cout << "Setting Global IP address" << endl;
            memcpy(prevSourceMac, sourceMacAddr, CV2X_MAC_ADDR_LEN);
            ret = radioReceives[0].setGlobalIPInfo(IpPrefix, configuration.wraServiceId);
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
    }
    // store the wra info from RSU if wsaInfoFile is configured in ObeConfig.conf
    if (0 == ret and not configuration.wsaInfoFile.empty()) {
        return storeWraInfoInObu(wra);
    }
    return ret;
}
#endif
void SaeApplication::wraThreadFunc(int routerLifetime)
{
    std::unique_lock<std::mutex> lk(wraMutex);
    std::cv_status status;

    lk.unlock();
    while (not exit_) {
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
            if (appVerbosity > 3)
                std::cout << "WRA timeout, global IP session stopped" << endl;
            return;
        }
    }
}

/* For RSU use case only */
int SaeApplication::setGlobalIPv6Prefix(void)
{
    char ipPrefix[CV2X_IPV6_ADDR_ARRAY_LEN];
    int prefixLen = CV2X_IPV6_ADDR_ARRAY_LEN;
    telux::cv2x::IPv6AddrType IpPrefix;
    int ret = 0;

    if (GlobalIpSessionActive == false) {
        memset(ipPrefix, 0, CV2X_IPV6_ADDR_ARRAY_LEN);
        if (!parseIPv6Addr(configuration.ipPrefix, ipPrefix, prefixLen)) {
            IpPrefix.prefixLen = configuration.ipPrefixLength;
            memcpy(IpPrefix.ipv6Addr, ipPrefix, prefixLen);
            if(radioReceives.size()){
                ret = radioReceives[0].setGlobalIPInfo(IpPrefix, configuration.wraServiceId);
            }else if(this->spsTransmits.size()){
                ret = this->spsTransmits[0].setGlobalIPInfo(IpPrefix, configuration.wraServiceId);
            }
        }
        GlobalIpSessionActive = true;
    }

    return ret;
}

int SaeApplication::clearGlobalIPv6Prefix(void)
{
    int ret = 0;
    GlobalIpSessionActive = false;
    if(radioReceives.size()){
        ret = radioReceives[0].clearGlobalIPInfo();
    }else if(this->spsTransmits.size()){
        ret = this->spsTransmits[0].clearGlobalIPInfo();
    }
    return ret;
}
