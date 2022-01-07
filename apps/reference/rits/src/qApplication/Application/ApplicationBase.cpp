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
// then periodically call "idChange" and check return value and updates in user data (idchangedata)
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

ApplicationBase::ApplicationBase(char* fileConfiguration){
    // set parameters according to config file
    if (this->loadConfiguration(fileConfiguration)) {
        return;
    }

    // set up kinematics listener
    kinematicsReceive = std::make_shared<KinematicsReceive>
                 (this->configuration.locationInterval);

    uint8_t keyGenMethod = NO_KEY_GEN;
    // setup radio flows
    this->setup();
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
              fprintf(stdout, "Performing ID Changes at time interval of: %f seconds\n",
                  this->configuration.idChangeInterval/1000.0);
              changeIdTimer(this->configuration.idChangeInterval);

          }else{
              // Non-LCM Constructor for Aerolink
              SecService = unique_ptr<SecurityService>(AerolinkSecurity::Instance(
                      configuration.securityContextName,
                      configuration.securityCountryCode));
          }
        }catch(const std::runtime_error& error){
            fprintf(stderr, "Aerolink initialization failed. Please check security settings\n");
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
 }

ApplicationBase::ApplicationBase(const string txIpv4, const uint16_t txPort,
            const string rxIpv4, const uint16_t rxPort,
            char* fileConfiguration) {
    if (this->loadConfiguration(fileConfiguration)) {
        return;
    }

    kinematicsReceive = std::make_shared<KinematicsReceive>
                        (this->configuration.locationInterval);
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
        if(this->configuration.enableTxAlways && this->configuration.tx_port &&
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
                this->configuration.eventServiceIDs.push_back(stoi(eventServiceIDs, nullptr, 10));
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

    if (configs.end() != configs.find("LocationInterval")) {
        this->configuration.locationInterval = stoi(configs["LocationInterval"], nullptr, 10);
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
        this->configuration.ldmGbTimeThreshold = stoi(configs["LdmGbTimeThreshold"], nullptr, 10);
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
    if (configs.find("psidValue") != configs.end()) {
        configuration.psid = stoi(configs["psidValue"],0,16);
    }
    if (configuration.enableSecurity == true) {
        if (configs.find("SecurityContextName") != configs.end()) {
            configuration.securityContextName = configs["SecurityContextName"];
        }
        if (configs.find("SecurityCountryCode") != configs.end()) {
            configuration.securityCountryCode = stoi(configs["SecurityCountryCode"], 0, 16);
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
            this->configuration.idChangeInterval = (unsigned int)stoi(configs["idChangeInterval"]);
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
                    std::cout << "Statistics for last " << configuration.mbdStatLogListSize <<
                        " misbehavior will be reported by each thread" << std::endl;
                    std::cout << "Upon closure, statistics will be dumped to logfile: " <<
                        configuration.mbdStatLogFile << std::endl;
                } else{
                    std::cout << "Misbehavior statistic logging is off" << std::endl;
                }
            }
        }
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
    if(configs.find("wildcardRx") != configs.end()){
       istringstream is8(configs["wildcardRx"]);
       is8 >> boolalpha >> configuration.wildcardRx;
    }
    this->configuration.isValid = true;

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
    if (this->configuration.ldmSize && this->ldm == nullptr) {
        this->ldm = new Ldm(this->configuration.ldmSize);
        this->ldm->startGb(this->configuration.ldmGbTime,
            this->configuration.ldmGbTimeThreshold);
        this->ldm->setVerbosity(this->configuration.appVerbosity);
    }
}

void ApplicationBase::simRxSetup(const string ipv4, const uint16_t port) {
    RadioOpt radioOpt;
    radioOpt.enableUdp = configuration.enableUdp;
    radioOpt.ipv4_src = configuration.ipv4_src;
    simReceive = std::unique_ptr<RadioReceive>
            (new RadioReceive(radioOpt, ipv4, port));
    simReceive->set_radio_verbosity(this->configuration.codecVerbosity);
    rxSimMsg = std::make_shared<msg_contents>();
    abuf_alloc(&rxSimMsg->abuf, ABUF_LEN, ABUF_HEADROOM);
    if (this->configuration.ldmSize && this->ldm ==nullptr) {
        this->ldm = new Ldm(this->configuration.ldmSize);
        this->ldm->startGb(this->configuration.ldmGbTime,
            this->configuration.ldmGbTimeThreshold);
        this->ldm->setVerbosity(this->configuration.appVerbosity);
    }
}

void ApplicationBase::setup() {
    uint8_t i = 0;
    EventFlowInfo eventInfo;
    SpsFlowInfo spsInfo;
    spsInfo.periodicityMs = this->configuration.transmitRate;
    for (auto port : this->configuration.spsPorts)
    {
        RadioTransmit tx(spsInfo, TrafficCategory::SAFETY_TYPE, TrafficIpType::TRAFFIC_NON_IP,
                         port, this->configuration.spsServiceIDs[i], false, 0);
        // save Tx instance only if create Tx flow succeeded
        if (tx.flow) {
            this->spsTransmits.push_back(std::move(tx));
        } else {
            cerr << "ApplicationBase::setup error in creating Tx SPS flow!" << endl;
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
        if (this->configuration.wildcardRx == true) {
            RadioReceive rx(TrafficCategory::SAFETY_TYPE, TrafficIpType::TRAFFIC_NON_IP, port);
            // save Rx instance only if create Rx flow succeeded
            if (rx.gRxSub) {
                this->radioReceives.push_back(std::move(rx));
            } else {
                cerr << "ApplicationBase::setup error in creating wildcard Rx!" << endl;
                return;
            }
        } else {
            RadioReceive rx(TrafficCategory::SAFETY_TYPE,
                            TrafficIpType::TRAFFIC_NON_IP, port,
                            std::make_shared<std::vector<uint32_t>>
                             (this->configuration.spsServiceIDs));
            // save Rx instance only if create Rx flow succeeded
            if (rx.gRxSub) {
                this->radioReceives.push_back(std::move(rx));
            } else {
                cerr << "ApplicationBase::setup error in creating non-wildcard Rx!" << endl;
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
            cerr << "ApplicationBase::setup error in creating Tx event flow!" << endl;
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


    if (this->configuration.ldmSize) {
        this->ldm = new Ldm(this->configuration.ldmSize);
        this->ldm->startGb(this->configuration.ldmGbTime,
                this->configuration.ldmGbTimeThreshold);
        this->ldm->packeLossThresh = this->configuration.packetError;
        this->ldm->distanceThresh = this->configuration.distance3D;
        this->ldm->positionCertaintyThresh = this->configuration.uncertainty3D;
        this->ldm->tuncThresh = this->configuration.tunc;
        this->ldm->ageThresh = this->configuration.age;
        this->ldm->setVerbosity(this->configuration.appVerbosity);
    }
    cout << "ApplicationBase::setup complete." << endl;
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
    // If positive, should be the # of bytes sent
    // Else, something went wrong
    int ret;
    // ethernet
    if (this->isTxSim) {
        ret = simTransmit->transmit(mc->abuf.data, bufLen);
    }else{ // radio
        if (txType == TransmitType::SPS) {
            ret =  this->spsTransmits[index].transmit(mc->abuf.data, bufLen);
        } else if (txType == TransmitType::EVENT) {
            ret =this->eventTransmits[index].transmit(mc->abuf.data, bufLen);
        }
    }
    return ret;
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
        encLength = encodeAndSignMsg(mc);
    }
    int ret = this->transmit(index, mc, encLength, txType);
    return encLength;
}

int ApplicationBase::encodeAndSignMsg(std::shared_ptr<msg_contents> mc){
        // The message need to be signed/encrypted after layer 3
        SecurityOpt sopt;
        uint8_t signedSpdu[512];
        uint32_t signedSpduLen = 512;
        sopt.psidValue = this->configuration.psid;
        if (this->configuration.sspLength)
            memcpy(sopt.sspValue, this->configuration.ssp,
            this->configuration.sspLength);
        sopt.sspLength = this->configuration.sspLength;
        sopt.sspMaskLength = this->configuration.sspMaskLength;
        sopt.enableAsync = this->configuration.enableAsync;
        sopt.secVerbosity = this->configuration.secVerbosity;
        if(kinematicsReceive){
            shared_ptr<ILocationInfoEx> locationInfo =
                                        kinematicsReceive->getLocation();
            sopt.hvKine.latitude = (locationInfo->getLatitude() * 10000000);
            sopt.hvKine.longitude = (locationInfo->getLongitude() * 10000000);
            sopt.hvKine.elevation = (locationInfo->getAltitude() * 10);
        }
        std::thread::id tid = std::this_thread::get_id();
        if (thrSignLatencies[tid].size() > signStatIdx[tid]) {
            sopt.signStat = &thrSignLatencies[tid].at(signStatIdx[tid]);
        }else{
            signStatIdx[tid] = 0;
            sopt.signStat = &thrSignLatencies[tid].at(signStatIdx[tid]);
        }
        auto encLength = 0;
        if (mc->abuf.tail_bits_left != 8)
            encLength = mc->abuf.tail - mc->abuf.data + 1;
        else
            encLength = mc->abuf.tail - mc->abuf.data;

        // Aerolink handles IEEE1609.2 header insertion, but this requires us to
        // make buffer copy of the header and the payload.
        if (SecService->SignMsg(sopt, (uint8_t*)mc->abuf.data,
                    encLength, signedSpdu, signedSpduLen) < 0) {
            return -1;
        }
        // successful verification, increment the sign stat idx
        signStatIdx[tid]++;
        signStatIdx[tid]%=thrSignLatencies[tid].size();

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
    std::thread::id thrId = std::this_thread::get_id();
    printf("Thread (%08x) is now dumping verification stats to %s\n",
            thrId,configuration.verifStatLogFile.c_str());
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
    std::thread::id thrId = std::this_thread::get_id();
    printf("Thread (%08x) is now dumping signing stats to %s\n",
            thrId,configuration.signStatLogFile.c_str());
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
    std::thread::id thrId = std::this_thread::get_id();
    printf("Thread (%08x) is now dumping misbehavior stats to %s\n",
            thrId,configuration.mbdStatLogFile.c_str());
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
