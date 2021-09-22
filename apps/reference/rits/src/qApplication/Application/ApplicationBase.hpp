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
#include <semaphore.h>
#include "v2x_msg.h"
#include "v2x_codec.h"
#include "KinematicsReceive.h"
#include "RadioReceive.h"
#include "RadioTransmit.h"
#include "Ldm.h"
#ifdef AEROLINK
#include "AerolinkSecurity.hpp"
#else
#include "NullSecurity.hpp"
#endif

#define ABUF_LEN            8448
#define ABUF_HEADROOM       256
#define MIN_PACKET_LEN      20
#define MAX_PACKET_LEN      8192

using namespace std;
enum class TransmitType {
    SPS,
    EVENT
};

enum class MessageType {
    BSM,
    CAM,
    DENM,
    SPAT,
    WSA
};

struct Config{
    bool isValid = false;
    int codecVerbosity;
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
    bool wildcardRx = false;
    bool enablePreRecorded = false;
    string preRecordedFile;
    bool enableTxAlways = true;
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
    string ipv4_dest;
    uint16_t tx_port = 0;
    /** GeoNetwork config data */
    uint8_t MacAddr[6];
    int StationType = 0;
    uint16_t CAMDestinationPort = 0;
    /** Security Config Data */
    bool enableSecurity = false;
    string securityContextName;
    uint16_t securityCountryCode;
    uint32_t psid;
    uint8_t ssp[32];
    uint32_t sspLength = 0;
    uint8_t sspMask[32];
    uint32_t sspMaskLength = 0;
    bool enableAsync = false;
    bool enableEncrypt = false;
    uint8_t externalDataHash[32];
    uint32_t hashLength = 0;
    /** Sec Driver Options **/
    uint8_t driverVerbosity = 0;
    uint8_t secVerbosity = 0;
    uint8_t appVerbosity = 0;
    /** Sec Driver Multi Threading Options **/
    uint8_t numRxThreads = 0;
    uint8_t numTxThreads = 0; // TODO
    /** Verification Stats Parameters */
    bool enableVerifStatLog = true;
    uint32_t verifStatsSize = 10000;
    string verifStatLogFile = "/tmp/verif_stats.log";
    /** Signing Stats Parameters */
    bool enableSignStatLog = true;
    uint32_t signStatsSize = 10000;
    string signStatLogFile = "/tmp/sign_stats.log";

    /* config data for Ieee1609.3 Wsa */
    long routerLifetime;
    string ipPrefix;
    int ipPrefixLength;
    string defaultGateway;
    string primaryDns;
    uint32_t wraServiceId = 4;
};

class ApplicationBase
{
public:
    sem_t rx_sem;
    sem_t log_sem;
    int appVerbosity = 0;
    int totalTxSuccess = 0;
    int totalRxSuccess = 0;

    /** For multi-threaded msg verification */
    std::map<std::thread::id, int> verifStatIdx;
    std::map<std::thread::id, int> signStatIdx;
    std::map<std::thread::id, std::vector<VerifStats>> thrVerifLatencies;
    std::map<std::thread::id, std::vector<SignStats>> thrSignLatencies;

    /* Function to permit different levels of verbosity */
    void setAppVerbosity(int value) {
        appVerbosity = value;
    }

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
    virtual int receive(const uint8_t index, const uint16_t bufLen,
                        const uint32_t ldmIndex);

    /**
     * Overloaded function to fill the message with stack specific data.(BSM/CAM/DENM) for transmition
     */
    virtual void fillMsg(std::shared_ptr<msg_contents> mc) = 0;

    /**
    * Closes all tx and rx flows from Snaptel SDK.
    */
    void closeAllRadio();

    /**
    *   Sets up the verification statistics vector based on the exisitng threads
    */
    void initVerifLogging();

    /**
     * Write verification statistics to file
     */
    void writeVerifLogging();

    /**
    *   Sets up the signing statistics vector based on the exisitng threads
    */
    void initSignLogging();

    /**
     * Write signing statistics to file
     */
    void writeSignLogging();

    void printRxStats();
    void printTxStats();
    void setup();

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

    FILE *csvfp;

    bool writeToCsv = false;

protected:
    bool isTx = false;
    bool isRx = false;
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
    virtual int transmit(uint8_t index, std::shared_ptr<msg_contents>mc,
                            int16_t bufLen, TransmitType txType);

    /**
     * Object that holds all data and meta data of the LocationSDK
     * and allows incoming fixes from such service.
     */
    shared_ptr<KinematicsReceive> kinematicsReceive;

    /**
     * Security service object.
     */
    unique_ptr<SecurityService> SecService;


private:
    void simTxSetup(const string ipv4, const uint16_t port);
    void simRxSetup(const string ipv4, const uint16_t port);
    static uint16_t delimiterPos(string line, vector<string> delimiters);
    int loadConfiguration(char* file);
    void saveConfiguration(map<string, string> configs);

};
#endif
