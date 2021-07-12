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
  * @file: Application.cpp
  *
  * @brief: Base class for ITS stack application
  */


#include "ApplicationBase.hpp"
using std::cout;
using std::string;
using std::map;
using std::pair;

#define ABUF_LEN            2048
#define ABUF_HEADROOM       256

ApplicationBase::ApplicationBase(char* fileConfiguration){
    this->vehicleReceive = unique_ptr<VehicleReceive>(VehicleReceive::Instance());
    this->loadConfiguration(fileConfiguration);
    this->setup();
    kinematicsReceive = std::make_shared<KinematicsReceive>(this->configuration.locationInterval);
    if (this->configuration.enableSecurity == true) {
#ifdef SECURITY
        SecService = unique_ptr<SecurityService>(SecurityImpl::Instance(
                    configuration.securityContextName, configuration.securityCountryCode));
#else
        SecService = unique_ptr<SecurityService>(NullSecurity::Instance(
                    configuration.securityContextName, configuration.securityCountryCode));
#endif
    }
 }

ApplicationBase::ApplicationBase(const string txIpv4, const uint16_t txPort, const string rxIpv4,
const uint16_t rxPort, char* fileConfiguration) {
    this->vehicleReceive = unique_ptr<VehicleReceive>(VehicleReceive::Instance());
    this->loadConfiguration(fileConfiguration);
    kinematicsReceive = std::make_shared<KinematicsReceive>(this->configuration.locationInterval);

    if (txPort)
    {
        this->simTxSetup(txIpv4, txPort);
        this->isTxSim = true;
    }
    if (rxPort) {
        this->simRxSetup(rxIpv4, rxPort);
        this->isRxSim = true;
    }
    if (this->configuration.enableSecurity == true) {
#ifdef SECURITY
        SecService = unique_ptr<SecurityService>(SecurityImpl::Instance(
                    configuration.securityContextName, configuration.securityCountryCode));
#else
        SecService = unique_ptr<SecurityService>(NullSecurity::Instance(
                    configuration.securityContextName, configuration.securityCountryCode));
#endif
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

void ApplicationBase::loadConfiguration(char* file) {
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
    }
    else {
        cout<<"Error opening config file.\n";
    }
}


void ApplicationBase::saveConfiguration(map<string, string> configs) {
    istringstream is(configs["EnablePreRecorded"]);
    is >> boolalpha >> this->configuration.enablePreRecorded;
    this->configuration.preRecordedFile = configs["PreRecordedFile"];
    this->configuration.transmitRate = stoi(configs["SpsTransmitRate"], nullptr, 10);
    stringstream stream(configs["SpsPorts"]);
    for (uint8_t i = 0; i < stoi(configs["SpsFlows"], nullptr, 10); i++)
    {
        string port;
        getline(stream, port, ',');
        this->configuration.spsPorts.push_back(stoi(port, nullptr, 10));
    }
    stream.str("");
    stream.clear();
    stream.str(configs["SpsDestAddrs"]);
    for (uint8_t i = 0; i < stoi(configs["SpsFlows"], nullptr, 10); i++)
    {
        string spsDestAddrs;
        getline(stream, spsDestAddrs, ',');
        this->configuration.spsDestAddrs.push_back(spsDestAddrs);
    }

    stream.str("");
    stream.clear();
    stream.str(configs["SpsDestNames"]);
    for (uint8_t i = 0; i < stoi(configs["SpsFlows"], nullptr, 10); i++)
    {
        string spsDestNames;
        getline(stream, spsDestNames, ',');
        this->configuration.spsDestNames.push_back(spsDestNames);
    }

    stream.str("");
    stream.clear();
    stream.str(configs["SpsDestPorts"]);
    for (uint8_t i = 0; i < stoi(configs["SpsFlows"], nullptr, 10); i++)
    {
        string port;
        getline(stream, port, ',');
        this->configuration.spsDestPorts.push_back(stoi(port, nullptr, 10));
    }
    stream.str("");
    stream.clear();
    stream.str(configs["SpsServiceIDs"]);
    for (uint8_t i = 0; i < stoi(configs["SpsFlows"], nullptr, 10); i++)
    {
        string port;
        getline(stream, port, ',');
        this->configuration.spsServiceIDs.push_back(stoi(port, nullptr, 10));
    }
    stream.str("");
    stream.clear();
    stream.str(configs["EventPorts"]);
    for (uint8_t i = 0; i < stoi(configs["EventFlows"], nullptr, 10); i++)
    {
        string port;
        getline(stream, port, ',');
        this->configuration.eventPorts.push_back(stoi(port, nullptr, 10));
    }

    stream.str("");
    stream.clear();
    stream.str(configs["EventDestAddrs"]);
    for (uint8_t i = 0; i < stoi(configs["EventFlows"], nullptr, 10); i++)
    {
        string EventDestAddrs;
        getline(stream, EventDestAddrs, ',');
        this->configuration.eventDestAddrs.push_back(EventDestAddrs);
    }

    stream.str("");
    stream.clear();
    stream.str(configs["EventDestNames"]);
    for (uint8_t i = 0; i < stoi(configs["EventFlows"], nullptr, 10); i++)
    {
        string EventDestNames;
        getline(stream, EventDestNames, ',');
        this->configuration.eventDestNames.push_back(EventDestNames);
    }

    stream.str("");
    stream.clear();
    stream.str(configs["EventDestPorts"]);
    for (uint8_t i = 0; i < stoi(configs["EventFlows"], nullptr, 10); i++)
    {
        string port;
        getline(stream, port, ',');
        this->configuration.eventDestPorts.push_back(stoi(port, nullptr, 10));
    }
    stream.str("");
    stream.clear();
    stream.str(configs["EventServiceIDs"]);
    for (uint8_t i = 0; i < stoi(configs["EventFlows"], nullptr, 10); i++)
    {
        string port;
        getline(stream, port, ',');
        this->configuration.eventServiceIDs.push_back(stoi(port, nullptr, 10));
    }

    stream.str("");
    stream.clear();
    stream.str(configs["ReceivePorts"]);
    for (uint8_t i = 0; i < stoi(configs["ReceiveFlows"], nullptr, 10); i++)
    {
        string port;
        getline(stream, port, ',');
        this->configuration.receivePorts.push_back(stoi(port, nullptr, 10));
    }


    this->configuration.locationInterval = stoi(configs["LocationInterval"], nullptr, 10);
    this->configuration.bsmJitter = stoi(configs["BsmJitter"], nullptr, 10);
    istringstream is2(configs["EnableVehicleExt"]);
    is2 >> boolalpha >> this->configuration.enableVehicleExt;
    this->configuration.pathHistoryPoints = stoi(configs["PathHistoryPoints"], nullptr, 10);
    this->configuration.vehicleWidth = stoi(configs["VehicleWidth"], nullptr, 10);
    this->configuration.vehicleLength = stoi(configs["VehicleLength"], nullptr, 10);
    this->configuration.vehicleHeight = stoi(configs["VehicleHeight"], nullptr, 10);
    this->configuration.frontBumperHeight = stoi(configs["FrontBumperHeight"], nullptr, 10);
    this->configuration.rearBumperHeight = stoi(configs["RearBumperHeight"], nullptr, 10);
    this->configuration.vehicleMass = stoi(configs["VehicleMass"], nullptr, 10);
    this->configuration.vehicleClass = stoi(configs["BasicVehicleClass"], nullptr, 10);
    this->configuration.sirenUse = stoi(configs["SirenInUse"], nullptr, 10);
    this->configuration.lightBarUse = stoi(configs["LightBarInUse"], nullptr, 10);
    this->configuration.specialVehicleTypeEvent = stoi(configs["SpecialVehicleTypeEvent"],
            nullptr, 10);
    this->configuration.vehicleType = stoi(configs["VehicleType"], nullptr, 10);
    this->configuration.ldmSize = stoi(configs["LdmSize"], nullptr, 10);
    this->configuration.ldmGbTime = stoi(configs["LdmGbTime"], nullptr, 10);
    this->configuration.ldmGbTimeThreshold = stoi(configs["LdmGbTimeThreshold"], nullptr, 10);
    this->configuration.tunc = stoi(configs["TTunc"], nullptr, 10);
    this->configuration.age = stoi(configs["TAge"], nullptr, 10);
    this->configuration.packetError = stoi(configs["TPacketError"], nullptr, 10);
    this->configuration.uncertainty3D = stoi(configs["TUncertainty3D"], nullptr, 10);
    this->configuration.distance3D = stoi(configs["TDistance"], nullptr, 10);
    this->configuration.ipv4_src = configs["SourceIpv4Address"];
    istringstream is3(configs["EnableUDP"]);
    is3 >> boolalpha >> this->configuration.enableUdp;
    /* ETSI config items */
    if (configs.find("MacAddr") != configs.end()) {
        int i = 0;
        auto pos = 0, prev = 0;
        do {
            pos = configs["MacAddr"].find(":", prev);
            if (pos != std::string::npos) {
                this->configuration.MacAddr[i] = stoi(configs["MacAddr"].substr(prev, pos), 0, 16);
            }
            prev = pos + 1;
            i++;
        } while(pos != std::string::npos);
        pos = configs["MacAddr"].rfind(" ");
        this->configuration.MacAddr[5] = stoi(configs["MacAddr"].substr(pos + 1, std::string::npos),
                0, 16);
    }
    if(configs.find("StationType") != configs.end()) {
        this->configuration.StationType = stoi(configs["StationType"]);
    }
    if (configs.find("CAMDestinationPort") != configs.end()) {
        this->configuration.CAMDestinationPort = (uint16_t)stoi(configs["CAMDestinationPort"]);
    }
    /* Security service */
    if (configs.find("EnableSecurity") != configs.end()) {
        if (configs["EnableSecurity"].find("true") != std::string::npos)
            this->configuration.enableSecurity = true;
        else
            this->configuration.enableSecurity = false;
    }
    if (configuration.enableSecurity == true) {
        if (configs.find("SecurityContextName") != configs.end()) {
            configuration.securityContextName = configs["SecurityContextName"];
        }
        if (configs.find("SecurityCountryCode") != configs.end()) {
            configuration.securityCountryCode = stoi(configs["SecurityCountryCode"], 0, 16);
        }
        if (configs.find("psidValue") != configs.end()) {
            configuration.psid = stoi(configs["psidValue"]);
        }
        if (configs.find("sspValue") != configs.end()) {
        } else {
            configuration.sspLength = 0;
        }
        if (configs.find("SavariWorkaround") != configs.end()) {
            if (configs["SavariWorkaround"].find("true") != std::string::npos)
                set_savari_workaround(1);
            else
                set_savari_workaround(0);
        }
        if(configs.find("enableAsync") != configs.end()) {
            istringstream is4(configs["enableAsync"]);
            is4 >> boolalpha >> configuration.enableAsync;
        }
    }
    /* codec debug */
    if (configs.find("codecVerbosity") != configs.end()) {
        set_codec_verbosity(stoi(configs["codecVerbosity"]));
    }
}

void ApplicationBase::simTxSetup(const string ipv4, const uint16_t port) {
    RadioOpt radioOpt;
    radioOpt.enableUdp = configuration.enableUdp;
    radioOpt.ipv4_src = configuration.ipv4_src;
    simTransmit = std::unique_ptr<RadioTransmit>(new RadioTransmit(radioOpt, ipv4, port));
    txSimMsg = std::make_shared<msg_contents>();
    abuf_alloc(&txSimMsg->abuf, ABUF_LEN, ABUF_HEADROOM);

    if (this->configuration.ldmSize && this->ldm == nullptr) {
        this->ldm = new Ldm(this->configuration.ldmSize);
        this->ldm->startGb(this->configuration.ldmGbTime, this->configuration.ldmGbTimeThreshold);
    }
}

void ApplicationBase::simRxSetup(const string ipv4, const uint16_t port) {
    RadioOpt radioOpt;
    radioOpt.enableUdp = configuration.enableUdp;
    radioOpt.ipv4_src = configuration.ipv4_src;
    simReceive = std::unique_ptr<RadioReceive>(new RadioReceive(radioOpt, ipv4, port));
    rxSimMsg = std::make_shared<msg_contents>();
    abuf_alloc(&rxSimMsg->abuf, ABUF_LEN, ABUF_HEADROOM);
    if (this->configuration.ldmSize && this->ldm ==nullptr) {
        this->ldm = new Ldm(this->configuration.ldmSize);
        this->ldm->startGb(this->configuration.ldmGbTime, this->configuration.ldmGbTimeThreshold);
    }
}

void ApplicationBase::setup() {
    uint8_t i = 0;
    EventFlowInfo eventInfo;
    SpsFlowInfo spsInfo;
    spsInfo.periodicityMs = this->configuration.transmitRate;
    for (auto port : this->configuration.receivePorts)
    {
        this->radioReceives.push_back(RadioReceive(TrafficCategory::SAFETY_TYPE,
                    TrafficIpType::TRAFFIC_NON_IP, port));
        std::shared_ptr<msg_contents> mc = std::make_shared<msg_contents>();
        abuf_alloc(&mc->abuf, ABUF_LEN, ABUF_HEADROOM);
        this->receivedContents.push_back(mc);

    }
    for (auto port : this->configuration.eventPorts)
    {
        this->eventTransmits.push_back(RadioTransmit(eventInfo, TrafficCategory::SAFETY_TYPE,
                                                TrafficIpType::TRAFFIC_NON_IP, port,
                                                this->configuration.eventServiceIDs[i]));
        this->eventTransmits[i].configureIpv6(this->configuration.eventDestPorts[i],
                                                this->configuration.eventDestAddrs[i].data(),
                                                this->configuration.eventDestNames[i].data());
        std::shared_ptr<msg_contents> mc = std::make_shared<msg_contents>();
        abuf_alloc(&mc->abuf, ABUF_LEN, ABUF_HEADROOM);
        this->eventContents.push_back(mc);
        i += 1;
    }

    i = 0;
    for (auto port : this->configuration.spsPorts)
    {

        this->spsTransmits.push_back(RadioTransmit(spsInfo, TrafficCategory::SAFETY_TYPE,
                                            TrafficIpType::TRAFFIC_NON_IP, port,
                                            this->configuration.spsServiceIDs[i], false, 0));
        this->spsTransmits[i].configureIpv6(this->configuration.spsDestPorts[i],
                                            this->configuration.spsDestAddrs[i].data(),
                                            this->configuration.spsDestNames[i].data());
        std::shared_ptr<msg_contents> mc = std::make_shared<msg_contents>();
        abuf_alloc(&mc->abuf, ABUF_LEN, ABUF_HEADROOM);
        this->spsContents.push_back(mc);
        i += 1;
    }

    if (this->configuration.ldmSize) {
        this->ldm = new Ldm(this->configuration.ldmSize);
        this->ldm->startGb(this->configuration.ldmGbTime, this->configuration.ldmGbTimeThreshold);
        this->ldm->packeLossThresh = this->configuration.packetError;
        this->ldm->distanceThresh = this->configuration.distance3D;
        this->ldm->positionCertaintyThresh = this->configuration.uncertainty3D;
        this->ldm->tuncThresh = this->configuration.tunc;
        this->ldm->ageThresh = this->configuration.age;
    }
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
void ApplicationBase::transmit(uint8_t index, std::shared_ptr<msg_contents> mc, int16_t bufLen,
        TransmitType txType) {
    if (this->isTxSim) {
        simTransmit->transmit(mc->abuf.data, bufLen);
        return;
    }
    if (txType == TransmitType::SPS) {
        this->spsTransmits[index].transmit(mc->abuf.data, bufLen);
    } else if (txType == TransmitType::EVENT) {
        this->eventTransmits[index].transmit(mc->abuf.data, bufLen);
    }
}

int ApplicationBase::send(uint8_t index, TransmitType txType) {
    const auto i = index;
    auto encLength = 0;
    std::shared_ptr<msg_contents> mc = nullptr;

    if (this->isTxSim) {
        mc = txSimMsg;
    } else if (txType == TransmitType::SPS) {
        mc = spsContents[i];
    } else if (txType == TransmitType::EVENT) {
        mc = eventContents[i];
    } else {
        return -1;
    }
    abuf_reset(&mc->abuf, ABUF_HEADROOM);
    fillMsg(mc);
    encLength = encode_msg(mc.get());

    if (encLength == 1) {
        // The message need to be signed/encrypted after layer 3
        SecurityOpt sopt;
        uint8_t signedSpdu[512];
        uint32_t signedSpduLen = 512;

        sopt.psidValue = this->configuration.psid;
        if (this->configuration.sspLength)
            memcpy(sopt.sspValue, this->configuration.ssp, this->configuration.sspLength);
        sopt.sspLength = this->configuration.sspLength;
        sopt.enableAsync = this->configuration.enableAsync;

        shared_ptr<ILocationInfoEx> locationInfo = kinematicsReceive->getLocation();
        sopt.latitude = (locationInfo->getLatitude() * 10000000);
        sopt.longitude = (locationInfo->getLongitude() * 10000000);
        sopt.elevation = (locationInfo->getAltitude() * 10);

        if (mc->abuf.tail_bits_left != 8)
            encLength = mc->abuf.tail - mc->abuf.data + 1;
        else
            encLength = mc->abuf.tail - mc->abuf.data;

        // Aerolink handles IEEE1609.2 header insertion, but this requires us to
        // make buffer copy of the header and the payload.
        if (SecService->SignMsg(sopt, (uint8_t *)mc->abuf.data, encLength, signedSpdu,
                    signedSpduLen) < 0) {
            return -1;
        }
        abuf_purge(&mc->abuf, abuf_headroom(&mc->abuf));
        asn_ncat(&mc->abuf, (char *)signedSpdu, signedSpduLen);
        encLength = encode_msg_continue(mc.get());
    }
    if (encLength > 0)
        this->transmit(i, mc, encLength, txType);

    return encLength;
}
void ApplicationBase::closeAllRadio() {
    for (uint8_t i = 0; i<this->eventTransmits.size(); i++)
    {
        this->eventTransmits[i].closeFlow();
    }

    for (uint8_t i = 0; i < this->spsTransmits.size(); i++)
    {
        this->spsTransmits[i].closeFlow();
    }

    for (uint8_t i = 0; i < this->radioReceives.size(); i++)
    {
        this->radioReceives[i].closeFlow();
    }
    if (this->simReceive != nullptr)
    {
        this->simReceive->closeFlow();
    }

}

int ApplicationBase::receive(const uint8_t index, const uint16_t bufLen) {
    const auto i = index;
    int ret;
    msg_contents *mc = nullptr;
    if(((int16_t)bufLen) > 500){
        cout << "Length too long: " << bufLen << "\n";
        return -1;
    }
    if((int16_t)bufLen <= 0){
        cout << "Something went wrong with reception. \n";
        return -1;
    }
    if (isRxSim)
    {
        mc = rxSimMsg.get();
    }
    else {
        mc = receivedContents[index].get();
    }
    // remove 1 byte family ID
    abuf_pull(&mc->abuf, 1);
    ret = decode_msg(mc);
    if (ret == 1) {
        // the message is signed/encrypted IEEE1609.2 content.
        ieee1609_2_data *ie = static_cast<ieee1609_2_data *>(mc->ieee1609_2data);
        if (ie->content == signedData) {
            SecurityOpt sopt;   // options are not used for verification at this time.
            uint32_t dot2HdrLen;
            sopt.enableAsync = this->configuration.enableAsync;
            ret = SecService->VerifyMsg(sopt, (uint8_t *)mc->l3_payload, mc->l3_payload_len,
                    dot2HdrLen);
            if (!ret) {
                /**
                 * decode_msg() assumed 1609.2 is unsecured packet, and advanced
                 * the packet buffer, but in fact, it's secured packet, the header
                 * length is longer then unsecured packet header, so we need to
                 * advance the packet buffer to pass the secured 1609.2 header
                 */
                mc->abuf.data = mc->l3_payload + dot2HdrLen;
                ret = decode_msg_continue(mc);
            }
        }

    }
    return ret;
}

int ApplicationBase::receive(const uint8_t index, const uint16_t bufLen, const uint32_t ldmIndex) {
    int ret;
    msg_contents *mc = nullptr;

    if (isRxSim) {
        mc = rxSimMsg.get();
    }else{
        mc = receivedContents[index].get();
    }
    // remove 1 byte family ID
    abuf_pull(&mc->abuf, 1);
    ret = decode_msg(mc);
    if (ret == 1) {
        // the message is signed/encrypted IEEE1609.2 content.
        ieee1609_2_data *ie = static_cast<ieee1609_2_data *>(mc->ieee1609_2data);
        if (ie->content == signedData) {
            SecurityOpt sopt;   // options are not used for verification at this time.
            uint32_t dot2HdrLen;
            sopt.enableAsync = this->configuration.enableAsync;
            ret = SecService->VerifyMsg(sopt, (uint8_t *)mc->l3_payload, mc->l3_payload_len,
                    dot2HdrLen);
            if (!ret) {
                /**
                 * decode_msg() assumed 1609.2 is unsecured packet, and advanced
                 * the packet buffer, but in fact, it's secured packet, the header
                 * length is longer then unsecured packet header, so we need to
                 * advance the packet buffer to pass the secured 1609.2 header
                 */
                mc->abuf.data = mc->l3_payload + dot2HdrLen;
                ret = decode_msg_continue(mc);
            }
        }
    }
    if (!ret) {
        auto bsm = reinterpret_cast<bsm_value_t *>(mc->j2735_msg);
        this->ldm->setIndex(bsm->id, ldmIndex);
    }
    return ret;
}
