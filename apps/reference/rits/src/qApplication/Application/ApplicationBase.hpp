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
  * @file: ApplicationBase.hpp
  *
  * @brief: Base class for ITS stack
  */
#ifndef __APPLICATION_BASE_HPP__
#define __APPLICATION_BASE_HPP__
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <memory>
#include <map>
#include <csignal>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "v2x_msg.h"
#include "v2x_codec.h"
#include "KinematicsReceive.h"
#include "RadioReceive.h"
#include "RadioTransmit.h"
#include "Ldm.h"
#include "VehicleReceive.h"
#ifdef SECURITY
#include "SecurityImpl.hpp"
#else
#include "NullSecurity.hpp"
#endif

using namespace std;
enum class TransmitType {
    SPS,
    EVENT
};

enum class MessageType {
    BSM,
    CAM,
    DENM,
    SPAT
};

struct Config{
    vector<uint16_t> receivePorts;
    vector<uint16_t> eventPorts;
    vector<uint16_t> spsPorts;
    vector<uint32_t> spsServiceIDs;
    vector<uint32_t> eventServiceIDs;
    vector<string> spsDestAddrs;
    vector<string> spsDestNames;
    vector<uint16_t> spsDestPorts;
    vector<uint16_t> eventDestPorts;
    vector<string> eventDestAddrs;
    vector<string> eventDestNames;
    bool enablePreRecorded = false;
    string preRecordedFile;
    uint16_t ldmGbTime = 3;
    uint8_t ldmGbTimeThreshold= 5;
    uint16_t ldmSize = 1;
    uint16_t transmitRate = 100;
    uint16_t locationInterval = 100;
    uint16_t bsmJitter = 0;
    bool enableVehicleExt = false;
    uint8_t pathHistoryPoints = 15;
    uint8_t vehicleWidth = 0;
    uint8_t vehicleLength = 0;
    uint8_t vehicleHeight = 0;
    uint8_t frontBumperHeight = 0;
    uint8_t rearBumperHeight = 0;
    uint8_t vehicleMass = 0;
    uint8_t vehicleClass = 0;
    uint8_t sirenUse = 0;
    uint8_t lightBarUse = 0;
    uint16_t specialVehicleTypeEvent = 0;
    uint8_t vehicleType = 0;
    uint32_t tunc = 0;
    uint32_t age = 0;
    uint32_t uncertainty3D = 0;
    uint32_t distance3D = 0;
    uint32_t packetError = 0;
    /** Simulation config */
    bool enableUdp = false;
    string ipv4_src;
    /** GeoNetwork config data */
    uint8_t MacAddr[6];
    int StationType = 0;
    uint16_t CAMDestinationPort = 0;
    /** Security config data */
    bool enableSecurity = false;
    string securityContextName;
    uint16_t securityCountryCode;
    uint32_t psid;
    uint8_t ssp[32];
    int sspLength;
    bool enableAsync = false;
    uint8_t externalDataHash[32];
};

class ApplicationBase
{
public:

    /**
    * Constructor of  Application instance with all the
    * specifications of a configuration file.
    * @param fileConfiguration a char* that contains the file path of the
    * configuration file.
    */
    ApplicationBase(char* fileConfiguration);

    /**
    * Constructs Application with all the specifications of a
    * configuration file and transmits in simulated TCP/IP
    * instead of Snaptel SDK radio. Ports can't be zero or won't be taken in
    * account.
    * @param txIpv4 a const string object that holds the transmit IP address.
    * @param txPort a const uint16_t that contains the transmit port.
    * @param rxIpv4 a const string object that holds the receive IP address.
    * @param rxPort a const uint16_t that contains the receive port.
    * @param fileConfiguration a char* that contains the file path of the
    */
    ApplicationBase(const string txIpv4, const uint16_t txPort,
        const string rxIpv4, const uint16_t rxPort, char* fileConfiguration);

    /**
    * send  send V2X message.
    * @param index - message content index.
    * @param type - The type of flow (event, sps) in which bsm will transmit.
    */

    int send(uint8_t index, TransmitType txType);
    /**
     * receive process received contents.
     * @param index message content index.
     * @param bufLen received buffer length.
     */
    virtual int receive(const uint8_t index, const uint16_t bufLen);

    /**
     * receive process received contents and store in LDM.
     * @param index message content index.
     * @param bufLen received buffer length.
     * @param ldmIndex the LDM index
     */
    virtual int receive(const uint8_t index, const uint16_t bufLen, const uint32_t ldmIndex);

    /**
     * Overloaded function to fill the message with stack specific data.(BSM/CAM/DENM) for transmition
     */
    virtual void fillMsg(std::shared_ptr<msg_contents> mc) = 0;

    /**
    * Closes all tx and rx flows from Snaptel SDK.
    */
    void closeAllRadio();

    /*********************************************************************************
     * data members.
     ********************************************************************************/
    Config configuration;
    /**
    * Instance of RadioReceive for simulations. Only allocated for simulation options.
    */
    unique_ptr<RadioReceive> simReceive;
    /**
    * Instance of RadioTransmit for simulations. Only allocated for simulation options.
    */
    unique_ptr<RadioTransmit> simTransmit;

    /**
     * Message used for simulation
     */
    shared_ptr<msg_contents> rxSimMsg;
    shared_ptr<msg_contents> txSimMsg;

    /**
    * BSMs STL vector of messages that have been decoded. This method gets replaced
    * with the LDM so the acpplication is no longer memoryless.
    */
    vector<shared_ptr<msg_contents>> receivedContents;

    /**
    * BSMs STL vector of host vehicle where data gets populated and
    * sent in an event flow. Each index matches the index of the event flow.
    * Meaning that there is one BSM already allocated for flows that have been opened.
    */
    vector<shared_ptr<msg_contents>> eventContents;

    /**
    * BSMs STL vector of host vehicle where data gets populated and
    * sent in a sps flow. Each index matches the index of the sps flow.
    * Meaning that there is one BSM already allocated for flows that
    * have been opened.
    */
    vector<shared_ptr<msg_contents>> spsContents;

    /**
    * Vector of RadioReceive instances that holds all data and meta
    * data of a Rx Subscription from the Snaptel SDK Radio Interface.
    */
    vector<RadioReceive> radioReceives;
    /**
    * Vector of RadioTransmit instances that holds all data and meta
    * data of an Event Flow from the Snaptel SDK Radio Interface.
    */
    vector<RadioTransmit> eventTransmits;
    /**
    * Vector of RadioTransmit instances that holds all data and meta
    * data of a Sps Flow from the Snaptel SDK Radio Interface.
    */
    vector<RadioTransmit> spsTransmits;
    /**
    * Object that represents the LDM were BSMs get stored.
    */
    Ldm* ldm = nullptr;

protected:
    //const uint16_t bufLength = 3000;
    bool isTxSim = false;
    bool isRxSim = false;
    MessageType MsgType;

    void fillSecurity(ieee1609_2_data* secData);
    /**
     * Overloaded function to initialize the message content for transmition.
     */
    virtual void initMsg(std::shared_ptr<msg_contents> mc) = 0;
    /**
     * Overloaded function to free the message content, counter-part of initMsg.
     */
    virtual void freeMsg(std::shared_ptr<msg_contents> mc) = 0;

    /**
     * Call radio transmit function, maybe overloaded by child class to perform
     * additional operation before calling radio tx.
     */
    virtual void transmit(uint8_t index, std::shared_ptr<msg_contents>mc, int16_t bufLen,
            TransmitType txType);

    /**
    * Object that holds all data and meta data of the LocationSDK
    * and allows incoming fixes from such service.
    */
    shared_ptr<KinematicsReceive> kinematicsReceive;

    /**
    * Object that listens on changes to vehicular CAN data
    * and holds most up to date data in vehicleData member
    * to serve the Application.
    */
    unique_ptr<VehicleReceive> vehicleReceive;

    /**
     * Security service object.
     */
    unique_ptr<SecurityService> SecService;


private:
    /**
    * Configuration Data structure to save all parsed information of configuration file.
    */

    void setup();
    void simTxSetup(const string ipv4, const uint16_t port);
    void simRxSetup(const string ipv4, const uint16_t port);
    static uint16_t delimiterPos(string line, vector<string> delimiters);
    void loadConfiguration(char* file);
    void saveConfiguration(map<string, string> configs);

};
#endif
