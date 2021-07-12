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
  * @file: EtsiApplication.cpp
  *
  * @brief: class for ITS stack - ETSI
  */
#include "EtsiApplication.hpp"

EtsiApplication::EtsiApplication(char *fileConfiguration): ApplicationBase(fileConfiguration) {

    GnConfig_t GnCfg;
    GeoNetRouterImpl::InitDefaultConfig(GnCfg);
    memcpy(GnCfg.mid, this->configuration.MacAddr, GN_MID_LEN);
    GnCfg.StationType = static_cast<gn::ITSStationType>(this->configuration.StationType);

    std::unique_ptr<GeoNetRouterImpl> gnp(GeoNetRouterImpl::Instance(
                kinematicsReceive->shared_from_this(), GnCfg));

    //std::unique_ptr<GeoNetRouterImpl> gnp(GeoNetRouterImpl::Instance(nullptr));
    GnRouter = std::move(gnp);
    GnRouter->SetLogLevel(4);

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
        mc->stackId = STACK_ID_ETSI;
    }
}

EtsiApplication::EtsiApplication(const string txIpv4, const uint16_t txPort,
        const string rxIpv4, const uint16_t rxPort, char* fileConfiguration) :
        ApplicationBase(txIpv4, txPort, rxIpv4, rxPort, fileConfiguration) {

    GnConfig_t GnCfg;
    GeoNetRouterImpl::InitDefaultConfig(GnCfg);
    memcpy(GnCfg.mid, this->configuration.MacAddr, GN_MID_LEN);
    GnCfg.StationType = static_cast<gn::ITSStationType>(this->configuration.StationType);

    std::unique_ptr<GeoNetRouterImpl> gnp(GeoNetRouterImpl::Instance(
                kinematicsReceive->shared_from_this(), GnCfg));
    GnRouter = std::move(gnp);

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
        mc->stackId = STACK_ID_ETSI;
    }
}
void EtsiApplication::initMsg(std::shared_ptr<msg_contents> mc) {
    mc->stackId = STACK_ID_ETSI;
    mc->gn = new char[sizeof(GnData_t)];
    mc->btp = new char[sizeof(btp_data_t)];
    mc->cam = new char[sizeof(CAM_t)];
    mc->denm = new char[sizeof(DENM_t)];
}

void EtsiApplication::freeMsg(std::shared_ptr<msg_contents> mc) {
    if (mc->gn)
        delete (char *)mc->gn;
    if (mc->btp)
        delete (char *)mc->btp;
    if (mc->cam)
        delete (char *)mc->cam;
    if (mc->denm)
        delete (char *)mc->denm;
}
void EtsiApplication::fillMsg(std::shared_ptr<msg_contents> mc) {
    fillBtp(static_cast<btp_data_t *>(mc->btp));
    fillCam(static_cast<CAM_t *>(mc->cam));
    mc->etsi_msg_id = ItsPduHeader__messageID_cam;
}

void EtsiApplication::fillBtp(btp_data_t *btp) {
    btp->pkt_type = BTP_PACKET_TYPE_B;
    btp->dp_info = 0;
    btp->d_port = this->configuration.CAMDestinationPort;
}

void EtsiApplication::fillCam(CAM_t *cam) {
    memset(cam, 0, sizeof(CAM_t));
    cam->header.protocolVersion = ItsPduHeader__protocolVersion_currentVersion; // value is 1
    cam->header.messageID = ItsPduHeader__messageID_cam; // value is 2(cam)
    cam->header.stationID = 0;
    fillCamLocation(cam);
    fillCamCan(cam);
}

void EtsiApplication::fillCamLocation(CAM_t *cam) {
    shared_ptr<ILocationInfoEx> locationInfo = kinematicsReceive->getLocation();
    ReferencePosition_t *RefPos = &(cam->cam.camParameters.basicContainer.referencePosition);

    // These unit convertion need to be reviewed
    RefPos->latitude = (locationInfo->getLatitude() * 10000000);
    RefPos->longitude = (locationInfo->getLongitude() * 10000000);
    RefPos->altitude.altitudeValue = (locationInfo->getAltitude() * 10);
    RefPos->altitude.altitudeConfidence = AltitudeConfidence_alt_000_20;
    RefPos->positionConfidenceEllipse.semiMajorConfidence =
        (locationInfo->getHorizontalUncertaintySemiMajor() * 20);

    RefPos->positionConfidenceEllipse.semiMinorConfidence =
        (locationInfo->getHorizontalUncertaintySemiMinor() * 20);

    RefPos->positionConfidenceEllipse.semiMajorOrientation = 0;
    cam->cam.camParameters.highFrequencyContainer.present =
        HighFrequencyContainer_PR_basicVehicleContainerHighFrequency;

    BasicVehicleContainerHighFrequency_t *bvchf = 
        &cam->cam.camParameters.highFrequencyContainer.choice.basicVehicleContainerHighFrequency;
    bvchf->heading.headingValue = 0;
    bvchf->heading.headingConfidence = HeadingConfidence_equalOrWithinZeroPointOneDegree;
    bvchf->speed.speedValue = (50 * locationInfo->getSpeed());
    bvchf->speed.speedConfidence = SpeedConfidence_equalOrWithinOneMeterPerSec;
    bvchf->driveDirection = DriveDirection_forward;
    bvchf->vehicleLength.vehicleLengthValue = 6;
    bvchf->vehicleLength.vehicleLengthConfidenceIndication =
        VehicleLengthConfidenceIndication_trailerPresenceIsUnknown;
    bvchf->vehicleWidth = 30;   //3 meters
    bvchf->longitudinalAcceleration.longitudinalAccelerationValue =
        (100 * locationInfo->getBodyFrameData().longAccel);

    bvchf->longitudinalAcceleration.longitudinalAccelerationConfidence =
        AccelerationConfidence_pointOneMeterPerSecSquared;
    bvchf->curvature.curvatureValue = CurvatureValue_straight;
    bvchf->curvature.curvatureConfidence = CurvatureConfidence_onePerMeter_0_00002;
    bvchf->curvatureCalculationMode = CurvatureCalculationMode_yawRateUsed;
    bvchf->yawRate.yawRateValue = (locationInfo->getBodyFrameData().yawRate * 100);
    bvchf->yawRate.yawRateConfidence = YawRateConfidence_degSec_000_10;
}

void EtsiApplication::fillCamCan(CAM_t *cam) {
    BasicVehicleContainerHighFrequency_t &bvchf =
        cam->cam.camParameters.highFrequencyContainer.choice.basicVehicleContainerHighFrequency;
    current_dynamic_vehicle_state_t* dp = this->vehicleReceive->vehicleData;
    // vehicle hight/width/steerin wheel angle, etc.
}

void EtsiApplication::transmit(uint8_t index, std::shared_ptr<msg_contents> mc, int16_t bufLen,
        TransmitType txType) {
    GnData_t gd;

    // gd is used to set GeoNetwork parameters.
    GnRouter->InitDefaultGnData(gd);

    // upper_proto should match the setting in fillBtp
    gd.upper_prot = gn::UpperProtocol::GN_UPPER_PROTO_BTP_B;

    // packet type SHB (Single Hop Broadcast), will test more type later
    gd.pkt_type = gn::PacketType::GN_PACKET_TYPE_SHB;
    gd.is_shb = true;
    gd.payload_len = static_cast<int>(bufLen);
    gd.tc = 2;

    // if the packet type is GBC(GeoNetwork Broadcast) or GAC (GeoNetwork Any
    // Cast), we also need to set the destination geographic area. But since we
    // are tesitng SHB here, we don't need to do that.

    // We create a transmit callback function before sending the packet to
    // GeoNet router, the router use it to send the packet to radio transmiter.
    // NOTE: the GnRouter->Transmit() will insert GN header and 1 byte cv2x
    // family ID
    if (this->isTxSim) {
        auto cb = std::bind(&RadioTransmit::transmit, simTransmit.get(),
                       std::placeholders::_1, std::placeholders::_2);
        GnRouter->Transmit(mc, static_cast<size_t>(bufLen), gd, cb);
        return;
    }
    if (txType == TransmitType::SPS) {
        auto cb = std::bind(&RadioTransmit::transmit, &this->spsTransmits[index],
                std::placeholders::_1, std::placeholders::_2);
        GnRouter->Transmit(mc, static_cast<size_t>(bufLen), gd, cb);
    } else if (txType == TransmitType::EVENT) {
        auto cb = std::bind(&RadioTransmit::transmit, &this->eventTransmits[index],
                std::placeholders::_1, std::placeholders::_2);
        GnRouter->Transmit(mc, static_cast<size_t>(bufLen), gd, cb);
    }
}

int EtsiApplication::receive(const uint8_t index, const uint16_t bufLen) {
    std::shared_ptr<msg_contents> mc = nullptr;
    int ret;

    if (isRxSim) {
        mc = rxSimMsg;
    } else {
        mc = receivedContents[index];
    }

    mc->decoded = false;
    if (mc->gn == nullptr) {
        mc->gn = new char[sizeof(GnData_t)];
    }
    GnData_t &gd = *(static_cast<GnData_t *>(mc->gn));

    // skip one byte cv2x family ID
    abuf_pull(&mc->abuf, 1);
    auto len = bufLen;
    len -= 1;
    // process received packet in GeoNetRouter, packet may be dropped by router.
    ret = GnRouter->Receive(reinterpret_cast<uint8_t *>(mc->abuf.data), len, gd);
    if (ret == 0) {
        // remove GN header and decode it.
        abuf_pull(&mc->abuf, len - gd.payload_len);
        if (gd.upper_prot == UpperProtocol::GN_UPPER_PROTO_BTP_A)
            mc->btp_pkt_type = BTP_PACKET_TYPE_A;
        else if (gd.upper_prot == UpperProtocol::GN_UPPER_PROTO_BTP_B)
            mc->btp_pkt_type = BTP_PACKET_TYPE_B;
        else {
            std::cerr << "Unsupported transport type" << std::endl;
            return -1;
        }
        decode_msg(mc.get());
        mc->decoded = true;
    }
    return ret;
}
