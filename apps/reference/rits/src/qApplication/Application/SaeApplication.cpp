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
SaeApplication::SaeApplication(char *fileConfiguration):ApplicationBase(fileConfiguration) {
    MsgType = MessageType::BSM;

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
}

SaeApplication::SaeApplication(const string txIpv4, const uint16_t txPort,
        const string rxIpv4, const uint16_t rxPort, char* fileConfiguration) :
        ApplicationBase(txIpv4, txPort, rxIpv4, rxPort, fileConfiguration) {

    MsgType = MessageType::BSM;
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
}
SaeApplication::~SaeApplication() {
}

void SaeApplication::initMsg(std::shared_ptr<msg_contents> mc) {
    mc->stackId = STACK_ID_SAE;
    mc->wsmp = new char[sizeof(wsmp_data_t)];
    mc->j2735_msg = new char[sizeof(bsm_value_t)];
    mc->ieee1609_2data = new char[sizeof(ieee1609_2_data)];
}

void SaeApplication::freeMsg(std::shared_ptr<msg_contents> mc) {
    if (mc->wsmp)
        delete static_cast<char *>(mc->wsmp);
    if (mc->j2735_msg)
        delete static_cast<char *>(mc->j2735_msg);
    if (mc->ieee1609_2data)
        delete static_cast<char *>(mc->ieee1609_2data);
}

void SaeApplication::fillMsg(std::shared_ptr<msg_contents> mc) {
    fillWsmp(static_cast<wsmp_data_t *>(mc->wsmp));
    fillSecurity(static_cast<ieee1609_2_data *>(mc->ieee1609_2data));
    fillBsm(static_cast<bsm_value_t *>(mc->j2735_msg));
    mc->msgId = 20;
    std::cout << "fillMsg SAE" << std::endl;
}

void SaeApplication::fillWsmp(wsmp_data_t *wsmp) {
    memset(wsmp, 0, sizeof(wsmp_data_t));
    wsmp->n_header.data = 3;
    wsmp->tpid.octet = 0;
    wsmp->psid = configuration.psid;
    wsmp->chan_load_ptr = nullptr;
    wsmp->chan_load_len = 0;
}

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
    //ref_app code
    current_dynamic_vehicle_state_t* dp = this->vehicleReceive->vehicleData;
    bsm->TransmissionState = (j2735_transmission_state_e)(int) dp->prndl; //fixing enum problem
    if (dp->events.data != 0) {
        bsm->has_safety_extension = (v2x_bool_t)1;
        bsm->vehsafeopts = (v2x_bool_t)(bsm->vehsafeopts | (1 << 3));
        bsm->events.data = (v2x_bool_t)((dp->events.data) >> 3);
    }
    bsm->SteeringWheelAngle = dp->steering_wheel_angle;
    bsm->brakes.word = dp->brake_status.word;

    if (dp->exterior_lights.data != 0) {
        bsm->has_safety_extension = (v2x_bool_t)1;
        bsm->vehsafeopts = bsm->vehsafeopts | (1 << 0);
        bsm->lights_in_use.data = dp->exterior_lights.data;
    }

    bsm->has_supplemental_extension = (v2x_bool_t)1;
    bsm->suppvehopts |= (SUPPLEMENT_VEH_EXT_OPTION_WEATHER_PROBE);
    bsm->weatheropts = bsm->weatheropts | (1 << 0);
    bsm->statusFront = dp->front_wiper_status;

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
void SaeApplication::transmit(uint8_t index, std::shared_ptr<msg_contents>mc, int16_t bufLen,
        TransmitType txType) {
    // insert family ID of 0x01
    char *p = abuf_push(&mc->abuf, 1);
    *p = 0x01;
    ApplicationBase::transmit(index, mc, bufLen + 1, txType);
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
