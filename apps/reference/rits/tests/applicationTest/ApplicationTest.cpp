/*
 *  Copyright (c) 2018-2021, The Linux Foundation. All rights reserved.
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
  * @file: ApplicationTest.cpp
  *
  * @brief: Implements several functionalities of qApplication library.
  *
  */
#include <thread>
#include <list>
#include <string>
#include <functional>
#include <thread>
#include <mutex>
#include <unistd.h>
#include <sys/timerfd.h>
#include <sys/time.h>
#include <regex>
#include "SaeApplication.hpp"
#ifdef ETSI
#include "EtsiApplication.hpp"
#endif
#include "safetyapp_util.h"
#include "bsm_utils.h"
#include <telux/common/Version.hpp>

using std::thread;
using std::string;
using std::list;
using std::ref;
using std::lock_guard;
using std::mutex;

// Global variables
ApplicationBase* application = nullptr;
vector<thread> threads;
bool csv = false;
string csvFileName;
sem_t cnt_sem;
auto rxsuccess = 0;
auto rxfail = 0;
bool stopThread = false;
bool dump_raw = false;
bool print_rv = true;
std::condition_variable cv;
bool haltRx = false;
std::mutex cv2xStatusMtx;
bool simMode = false;

void joinThreads() {
    for (int i = 0; i < threads.size(); i++)
    {
        threads[i].join();
    }
}

void signalHandler(int signum) {
    if(signum == SIGSEGV){
        if(application->ldm != nullptr)
            application->ldm->stopGb();
        application->closeAllRadio();
        exit(signum);
    }
    cout << "Interrupt signal (" << signum << ") received.\n";
    cout << "Exiting..." << endl;
    stopThread = true;
    return;
}




/**
 * receiving thread function.
 *
 * @param[in] msgType type of the messsage we are processing.
 */
void receive(MessageType msgType, int index) {
    auto count = 0;
    FILE *fp;
    struct timeval currTime;
    gettimeofday(&currTime, NULL);
    time_t startTime = currTime.tv_sec;

    if (application->configuration.driverVerbosity > 4) {
        cout << "Thread id: " << std::this_thread::get_id()
                << " Wating for message..." << endl;
    }
    if(application->configuration.enableVerifStatLog){
        application->initVerifLogging();
    }
    if(application->configuration.enableMbdStatLog)
        application->initMisbehaviorLogging();

    // will need to make this compatible for multiple rx ports
    int ret;
    while (!stopThread)
    {
        if(!simMode){
            //Check if CV2X is active, if not wait for CV2X Status to be ACTIVE
            sem_wait(&cnt_sem);
            //Check CV2X RX status when only RX is enabled
            if (!application->configuration.enableTxAlways) {
                application->radioReceives[index].waitForCv2xToActivate(haltRx);
                if (application->radioReceives[index].restartFlow) {
                    application->closeAllRadio();
                    application->setup();
                }
            }
            else {// if TX is also enabled check the CV2X status in TX only
                std::unique_lock<std::mutex> lk(cv2xStatusMtx);
                cv.wait(lk, []{return (!haltRx);});
            }
            sem_post(&cnt_sem);
        }

        // call application's receive() function to process the packet across
        // stack layers.
        ret = application->receive(index, MAX_PACKET_LEN);

        sem_wait(&cnt_sem);
        if(ret >= 0){
            rxsuccess++;
            if (application->configuration.driverVerbosity) {
                if (rxsuccess % 50 == 0 && rxsuccess > 0){
                    gettimeofday(&currTime, NULL);
                    cout << "Dur(s): " << (currTime.tv_sec-startTime) <<
                        " Decode/Rx Success #: " << rxsuccess <<
                        " Decode/Rx Fail #: " << rxfail << std::endl;
                }
            }
        } else {
            rxfail++;
        }
        sem_post(&cnt_sem);
    }
    if(application->configuration.enableVerifStatLog){
        application->writeVerifLogging();
    }
    if(application->configuration.enableMbdStatLog){
        application->writeMisbehaviorLogging();
    }
    if(msgType == MessageType::BSM || msgType == MessageType::WSA)
        ((SaeApplication*)application)->printRxStats();
    printf("Total of RX packets is: %d\n", application->totalRxSuccess);

    if(application->ldm != nullptr)
        application->ldm->stopGb();
}
/**
 * receive function for LDM functionality test.
 *
 * @param [in] msgType, so far only BSM is supported.
 */
void ldmRx(void) {
    auto count = 0;
    FILE *fp;
    struct timeval currTime;
    gettimeofday(&currTime, NULL);
    time_t startTime = currTime.tv_sec;

    if (nullptr == application) {
        cerr << "application nullptr" << endl;
        return;
    }
    if(application->configuration.enableVerifStatLog){
        application->initVerifLogging();
    }
    if(application->configuration.enableMbdStatLog)
        application->initMisbehaviorLogging();

    if (application->ldm == nullptr) {
        printf("LDM not initialized properly, exiting\n");
        return;
    }

    int ret = 0;
    uint32_t ldmIndex = 0;
    while (!stopThread)
    {
        // if a new ldm slot index is necessary, retrieve one
        if(ret >= 0){
            ldmIndex = application->ldm->getFreeBsmSlotIdx();
        }
        // else keep the old ldm index and reuse slot
        ret = application->receive(0, MAX_PACKET_LEN, ldmIndex);
        sem_wait(&cnt_sem);
        if(ret >= 0){
            rxsuccess++;
            if (application->configuration.driverVerbosity) {
                if (rxsuccess % 50 == 0 && rxsuccess > 0){
                    gettimeofday(&currTime, NULL);
                    cout << "Dur(s): " << (currTime.tv_sec-startTime) <<
                        " Decode/Rx Success #: " << rxsuccess <<
                        " Decode/Rx Fail #: " << rxfail << std::endl;
                }
            }
        } else {
            rxfail++;
        }
        sem_post(&cnt_sem);
    }
    if(application->configuration.enableVerifStatLog){
        application->writeVerifLogging();
    }
    if(application->configuration.enableMbdStatLog){
        application->writeMisbehaviorLogging();
    }
    ((SaeApplication*)application)->printRxStats();
    printf("Total of RX packets is: %d\n", application->totalRxSuccess);

    if(application->ldm != nullptr)
        application->ldm->stopGb();

}
/**
 * Initialize timer for transmit
 * @param[in] interval_ns timer interval value in nano seconds
 * @return timer's file descriptor if success or -1 on failure.
 */
int start_tx_timer(long long interval_ns) {
    int timerfd;
    struct itimerspec its = {0};

    timerfd = timerfd_create(CLOCK_MONOTONIC, 0);
    if (timerfd < 0) {
        return -1;
    }

    /* Start the timer */
    its.it_value.tv_sec = interval_ns / 1000000000;
    its.it_value.tv_nsec = interval_ns % 1000000000;
    its.it_interval = its.it_value;

    if (timerfd_settime(timerfd, 0, &its, NULL) < 0) {
        close(timerfd);
        return -1;
    }

    return timerfd;

}
/**
 * transmit thread function
 * @param[in] msgType, type of message we are transmitting, so far only BSM and
 * CAM are supported. DENM is not supported
 * @returns none.
 */
void transmit(MessageType msgType) {
    int tx_timer_fd = -1;
    int timer_misses = 0;
    uint64_t exp;
    ssize_t s;
    tx_timer_fd = start_tx_timer(1000000*application->configuration.transmitRate);
    if (tx_timer_fd == -1) {
        cerr << "Failed to start Tx timer" << endl;
        return;
    }

    int txsuccess = 0;
    int txfail = 0;
    int ret = 0;

    // check if sign stat logging on
    // check off for now
    if(application->configuration.enableSignStatLog)
        application->initSignLogging();

     struct timeval currTime;
    gettimeofday(&currTime, NULL);
    time_t startTime = currTime.tv_sec;

    //Perform message protocol specific setup here
    switch (msgType){
        case MessageType::BSM:
            printf("Sending BSM messages\n");
            break;
        case MessageType::WSA:
            printf("Sending WSA messages\n");
            //sending WSA, transmit only, we are simulating RSU, so set the IrevV6
            if ((dynamic_cast<SaeApplication *>
                    (application))->setGlobalIPv6Prefix() < 0) {
                printf("Failed to set global IP info\n");
                return;
            }
            break;
        case MessageType::CAM:
            break;
        case MessageType::DENM:
            cerr << "DENM transmit is not supported" << endl;
            break;
        default:
            break;
    }

    // main transmitting code
    while (!stopThread){
        if(!simMode){
            //Check if CV2X is active, if not wait for CV2X Status to be ACTIVE
            //Check CV2X TX Status when TX is enabled.
            application->spsTransmits[0].waitForCv2xToActivate(haltRx);
            if (application->spsTransmits[0].restartFlow) {
                application->closeAllRadio();
                application->setup();
                close(tx_timer_fd);
                tx_timer_fd = start_tx_timer(1000000*application->configuration.transmitRate);
                {
                    std::lock_guard<std::mutex> lk(cv2xStatusMtx);
                    haltRx = false;
                }
                cv.notify_all();
            }
        }
        ret = application->send(0, TransmitType::SPS);
        if(ret > 0){
            txsuccess++;
            if (application->configuration.driverVerbosity) {
                if (txsuccess % 50 == 0 && txsuccess > 0){
                    gettimeofday(&currTime, NULL);
                    cout << "Dur(s): " << (currTime.tv_sec-startTime) <<
                        " Encode/Tx Success #: " << txsuccess <<
                        " Encode/Tx Fail #: " << txfail << std::endl;
                }
            }
        } else {
            txfail++;
        }

        s = read(tx_timer_fd, &exp, sizeof(uint64_t));
        if (s == sizeof(uint64_t) && exp > 1) {
            timer_misses += (exp-1);
            cout << "TX timer overruns: Total missed: " << timer_misses << endl;
        }
    }
    printf("Sending thread stopped\n");
    if(msgType == MessageType::WSA)
        (dynamic_cast<SaeApplication *>(application))->clearGlobalIPv6Prefix();

    // dump out any logging information related to signing
    if(application->configuration.enableSignStatLog){
        application->writeSignLogging();
    }
    if(msgType == MessageType::BSM || msgType == MessageType::WSA)
        ((SaeApplication*)application)->printTxStats();
    printf("Total of TX packets is: %d\n", application->totalTxSuccess);

    if(application->ldm != nullptr)
        application->ldm->stopGb();

    if (tx_timer_fd != -1) {
        close(tx_timer_fd);
    }
}
/**
 * transmit BSM from pre-recorded csv file.
 *
 * @param[in] file name of the csv file.
 * @msgType type of the message, so far only BSM is supported for this test.
 * @returns none.
 */
void txRecorded(string file) {
    srand(timestamp_now());
    ifstream configFile(file);
    string line;
    bool go = true;

    if (configFile.is_open())
    {
        auto timer = timestamp_now();
        while (go and !stopThread) {
            if(timer + application->configuration.transmitRate < timestamp_now()){
                if (getline(configFile, line))
                {
                    if (application->configuration.eventPorts.size()) {
                        const auto iEvent = rand() % application->configuration.eventPorts.size();
                        auto mc = application->eventContents[iEvent];
                        auto len = encode_singleline_fromCSV((char *)line.data(), mc->abuf.data,
                                line.length());
                        abuf_put(&mc->abuf, len);
                        application->eventTransmits[iEvent].transmit(mc->abuf.data, len);
                    }
                    if (application->configuration.spsPorts.size()) {
                        if (getline(configFile, line)) {
                            const auto iSps = rand() % application->configuration.spsPorts.size();
                            auto mc = application->spsContents[iSps];
                            auto len = encode_singleline_fromCSV((char*)line.data(),mc->abuf.data,
                                    line.length());
                            abuf_put(&mc->abuf, len);
                            application->spsTransmits[iSps].transmit(mc->abuf.data, len);
                        }
                    }
                }
                else {
                    go = false;
                }
                timer = timestamp_now();
            }
        }
    }
    else {
        cout << "Recorded File doesn't exists.\n";
    }
}
/**
 * transmit pre-recorded BSM message via radio simulation
 *
 * @param [in] msgType , so far only BSM is supported in this mode
 * @returns none.
 */
void simTxRecorded(string file)
{
    ifstream configFile(file);
    string line;
    if (configFile.is_open())
    {
        auto timer = timestamp_now();
        while (!stopThread)
        {
            if (timer + application->configuration.transmitRate < timestamp_now()) {
                if (getline(configFile, line))
                {
                    auto mc = application->txSimMsg;
                    auto len = encode_singleline_fromCSV((char*)line.data(), mc->abuf.data,
                            line.length());
                    abuf_put(&mc->abuf, len);
                    application->simTransmit->transmit(mc->abuf.data, len);
                    timer = timestamp_now();
                }else{
                    return;
                }
            }
        }
    }else {
        cout << "Recorded File doesn't exists.\n";
    }
}

void tunnelModeTx(void) {
    auto timer = timestamp_now();
    while (!stopThread)
    {
        if (timer + application->configuration.transmitRate < timestamp_now()) {
            SaeApplication *SaeApp = dynamic_cast<SaeApplication *>(application);
            SaeApp->sendTuncBsm(0, TransmitType::SPS);
            timer = timestamp_now();
        }
    }
}

void tunnelModeRx(void) {
    while (!stopThread)
    {
        SaeApplication *SaeApp = dynamic_cast<SaeApplication *>(application);
        const auto mc = SaeApp->receivedContents[0];
        const auto recCount =
                SaeApp->radioReceives[0].receive(mc->abuf.data,
                                                    MAX_PACKET_LEN-ABUF_HEADROOM);
        abuf_put(&mc->abuf, recCount);
        const auto ldmIndex = application->ldm->getFreeBsmSlotIdx();
        SaeApp->receiveTuncBsm(0, recCount, ldmIndex);
        if (!SaeApp->ldm->filterBsm(ldmIndex)) {
            const auto bsm = static_cast<bsm_value_t *>(mc->j2735_msg);
            SaeApp->ldm->setIndex(bsm->id, ldmIndex, mc);
        }
    }
}

/**
 * run safety application.
 */
void runApps(void) {
    auto hostMsg = std::make_shared<msg_contents>();
    rv_specs* rvSpecs = new rv_specs;
    while (!stopThread) {
        for (auto rvMsg : application->ldm->bsmSnapshot()) {
            application->fillMsg(hostMsg);
            fill_RV_specs(hostMsg.get(), rvMsg.get(), rvSpecs);
            forward_collision_warning(rvMsg.get(), rvSpecs);
            EEBL_warning(rvMsg.get(), rvSpecs);
            accident_ahead_warning(rvMsg.get(), rvSpecs);
            print_rvspecs(rvSpecs);
        }
    }
}


void printUse() {
    cout << "Usage: qits [options] <Config File Path>\n";
    cout << "  At least one option is needed and Config File Path is always required.\n";
    cout << "Options:\n";
    cout << "  General options: \n";
    cout << "  -h Prints help options.\n";
    cout << "  -D Dump raw received packet.\n";
    cout << "  -v Don't print received remote vehicle summary(for performance measurement).\n";
    cout << "  -o <CSV file path> write received BSM into CSV file.\n\n";
    cout << "  Modes:\n";
    cout << "  -t Transmits Cv2x data. Runs by default with -b. See -b.\n";
    cout << "  -r Receives Cv2x data. Runs by defaullt with -b. See -b.\n";
    cout << "  -i <other_device_ip_address> <port> Simulates CV2X and sends packets via TCP ";
    cout << "instead of OTA.\n";
    cout << "           note: You may enable UDP if desired via the config file\n";
    cout << "  -j <other_device_ip_address> <port> Simulates CV2X and receives packets via TCP ";
    cout << "instead of OTA.\n";
    cout << "           note: You may enable UDP if desired via the config file\n";
    cout << "  -b SAE WSMP BSMS.\n";
    cout << "  -w SAE WSMP WRA(in WSA).\n";
#ifdef ETSI
    cout << "  -c ETSI CAMs.\n";
    cout << "  -d ETSI DENMs.\n";
#endif
    cout << "  Incomplete Modes\n";
    cout << "  -l LDM mode; Adds -r if nothing specified. Use it with -r or -j.\n";
    cout << "  -s Safety Apps Mode; Adds -l if not specified. Runs by default with -b.\n";
    cout << "  -p <Pre-Recorded File Path> Transmits from pre-recorded file.\n";
    cout << "  -T Tunnel Transmit.\n";
    cout << "  -x Tunnel Receive. It automatically calls -l. See: -l.\n";
    cout << "Examples (assuming path to ObeConfig.conf is /etc/ObeConfig.conf):\n";
    cout << "  Example: qits -t /etc/ObeConfig.conf\n";
    cout << "  Example above will transmit BSMs (the default packet type)\n\n";
    cout << "  Example: qits -r -b /etc/ObeConfig.conf\n";
    cout << "  Example above will run: receive mode with basic safety messages.\n\n";
    cout << "  Example: qits -t -r -b /etc/ObeConfig.conf\n";
    cout << "  Example above will run: transmit and receive mode with basic safety messages.\n\n";
    cout << "  Example: qits -t -l -s /etc/ObeConfig.conf\n";
    cout << "  Example above will run: transmit mode, ldm receive mode and the safety apps.\n\n";
    cout << "  Example: qits -i 127.0.0.1 9000 /etc/ObeConfig.conf\n";
    cout << "  Example above will run: simulation transmit mode (TCP/UDP),\n";
    cout << "    sending BSMs over port 9000 to ip address 127.0.0.1\n\n";
    cout << "  Note: options -i and -j require SourceIpv4Address to be set\n";
}

void configFileCheck(string& configFile)
{
    if (configFile[0] == '-')
    {
        cout << "No config file specified.\n";
        cout << "Setting config file to default .\\ObeConfig.conf\n";
        configFile = string("ObeConfig.conf");
    }
    if (configFile.find(".conf") == std::string::npos) {
        cout << "Config file doesn't have .conf extension...\n";
        cout << "Are you sure you added or you added the right config file?\n\n\n\n";
        cout << "Setting config file to default .\\ObeConfig.conf\n";
        configFile = string("ObeConfig.conf");
    }
}

/* Sets parameters according to runtime arguments */
void getModes(char mode, int& idx, int& argc, char** argv, bool& tx, bool& rx,
    bool& ldm, bool& help, bool& safetyApps, bool& bsm, bool& wsa,
    bool& cam, bool& denm, bool& preRecorded, string& preRecordedFile,
    bool& txSim, bool& rxSim, bool& tunnelTx, bool& tunnelRx,
    bool& csv, string& txSimIp, string& rxSimIp,
    uint16_t& txSimPort, uint16_t& rxSimPort) {

    bsm = true; //default
    switch (mode)
    {
    case 'h':
        help = true;
        break;
    case 't':
        tx = true;
        break;
    case 'r':
        rx = true;
        break;
    case 's':
        safetyApps = true;
        ldm = true;
        rx = true;
        break;
    case 'p':
        preRecorded = true;
        argc += 1;
        preRecordedFile = string(argv[argc]);
        break;
    case 'x':
        tunnelRx = true;
        ldm = true;
        rx = true;
        break;
    case 'T':
        tunnelTx = true;
        tx = true;
        break;
    case 'l':
        ldm = true;
        rx = true;
        break;
    case 'b':
        bsm = true;
        wsa = false;
        break;
    case 'w':
        wsa = true;
        bsm = false;
        break;
#ifdef ETSI
    case 'c':
        cam = true;
        bsm = false;
        break;
    case 'd':
        denm = true;
        bsm = false;
        break;
#endif
    case 'i':
        txSim = true;
        simMode = true;
        if(idx+2 > argc-1){
           printUse();
           fprintf(stderr, "\nInvalid usage of -i option\n");
           exit(0);
        }
        // need to test for valid num of arguments here
        idx += 1;
        try {
            txSimIp = string(argv[idx]);
        }catch (std::invalid_argument& e){
            fprintf(stderr, "Invalid IP Address argument\n");
            exit(0);
        }
        idx += 1;
        try {
            txSimPort = stoi(string(argv[idx]), nullptr, 10);
        }catch (std::invalid_argument& e){
            fprintf(stderr, "Invalid port argument\n");
            fprintf(stdout, "Changing to 9000 by default\n");
            txSimPort = 9000;
        }
        break;
    case 'j':
        rxSim = true;
        simMode = true;
        if(idx+2 > argc-1){
           printUse();
           fprintf(stderr, "\nInvalid usage of -j option\n");
           exit(0);
        }
        idx += 1;
        try {
            rxSimIp = string(argv[idx]);
        }catch (std::invalid_argument& e){
            fprintf(stderr, "Invalid IP Address argument\n");
            fprintf(stdout, "Changing to accept all\n");
            rxSimIp = stoi(string("0.0.0.0"), nullptr, 10);
        }
        idx += 1;
        try {
            rxSimPort = stoi(string(argv[idx]), nullptr, 10);
        }catch (std::invalid_argument& e){
            fprintf(stderr, "Invalid port argument\n");
            fprintf(stdout, "Changing to 9000 by default\n");
            rxSimPort = 9000;
        }
        break;
    case 'o':
        csv = true;
        idx++;
        csvFileName = string(argv[idx]);
        break;
    case 'D':
        dump_raw = true;
        break;
    case 'v':
        print_rv = false;
        break;
    default:
        break;
    }
}

int setup(const bool tx, const bool rx,
    const bool ldm, const bool help, const bool safetyApps,
    const bool bsm, const bool wsa, const bool cam, const bool denm, const bool preRecorded,
    const string preRecordedFile, const bool txSim, const bool rxSim,
    const bool tunnelTx,const bool tunnelRx, const string txSimIp,
    const string  rxSimIp, const  uint16_t txSimPort,
    const uint16_t rxSimPort, char* configFile)
{
    std::signal(SIGHUP, signalHandler);
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    std::signal(SIGSEGV, signalHandler);
    if (help)
    {
        printUse();
        return 0;
    }

    auto sdkVersion = telux::common::Version::getSdkVersion();
    std::cout << "Telematics SDK v" << std::to_string(sdkVersion.major) << "."
                          << std::to_string(sdkVersion.minor) << "."
                          << std::to_string(sdkVersion.patch) << std::endl;

    MessageType msgType;
    if (bsm || wsa) {
        msgType = bsm? MessageType::BSM : MessageType::WSA;
        printf("Will be creating application for: ");
        if(msgType == MessageType::BSM)
            printf("BSMs\n");
        else
            printf("WSAs\n");
        // wsa not compatible with simulation mode
        if((txSim || rxSim) && wsa){
           fprintf(stderr, "WSA requires radio mode.\n");
           return -1;
        }
        if (txSim)
            application =
                new SaeApplication(txSimIp, txSimPort, string(""), 0, configFile,
                        msgType);
        else if (rxSim)
            application =
                new SaeApplication(string(""), 0, rxSimIp, rxSimPort, configFile,
                        msgType);
        else
            application = new SaeApplication(configFile, msgType);

    } else {
#ifdef ETSI
        msgType = MessageType::CAM;
        printf("Will be creating application for: ");
        if (cam) {
            msgType = MessageType::CAM;
            printf("CAMs\n");
        } else if (denm) {
            msgType = MessageType::DENM;
            printf("DENM\n");
        } else {
            printf("Unknown\n");
            return -1;
        }
        if (txSim)
            application =
                new EtsiApplication(txSimIp, txSimPort, string(""), 0, configFile);
        else if (rxSim)
            application =
                new EtsiApplication(string(""), 0, rxSimIp, rxSimPort, configFile);
        else
            application = new EtsiApplication(configFile);
#endif
    }

    if (not application
        or not application->configuration.isValid) {
        cout << "Invalid configuration" << endl;
        return -1;
    }

    // check if we want to have tx on at same time as rx (either ethernet or radio)
    if(application->configuration.enableTxAlways &&
        (rx || rxSim) && application->configuration.driverVerbosity){
            printf("Device will be sending AND receiving packets\n");
    }else if(!tx && !txSim && application->configuration.driverVerbosity){
            printf("Device will be in receive-only mode\n");
    }

    if ((tx || application->configuration.enableTxAlways) && !txSim && !rxSim)
    {
        if (application->spsTransmits.empty()) {
            cerr << "Tx flow not created, please check configuration" << endl;
            return -1;
        }

        if (tunnelTx) {
            if (cam || denm) {
                cout << "Tunnel Mode only supports BSM" << endl;
                return -1;
            }
            threads.push_back(thread(tunnelModeTx));
        } else {
            threads.push_back(thread(transmit, msgType));
        }
    }

    if(application->configuration.driverVerbosity > 4)
        printf("Number of threads after tx is: %d\n", (int)threads.size());

    if (rx && !rxSim)
    {
        if (application->radioReceives.empty()) {
            cerr << "Rx flow not created, please check configuration" << endl;
            return -1;
        }

        if (csv) {
            application->writeToCsvFile = true;
            application->csvfp = fopen(csvFileName.c_str(), "w+");
            if (!application->csvfp) {
                cerr << "Failed to open file " << csvFileName << " for writing" << endl;
                application->writeToCsvFile = false;
            } else {
                std::cout << "Writing BSM to csv: " << csvFileName << std::endl;
            }
        }

        sem_init(&cnt_sem, 0, 1);
        if (ldm)
        {
            if (cam || denm) {
                cout << "LDM Mode only supports BSM" << endl;
                return -1;
            }
            // setup full ldm
            if(application->configuration.ldmSize) {
                application->setupLdm();
            }
            if (tunnelRx) {
                threads.push_back(thread(tunnelModeRx));
            }
            else {
                // TODO: Implement for CAM, DENM as well
                if (application->configuration.driverVerbosity) {
                    cout << "Number of Radio LDM RX Threads: " <<
                            (int)application->configuration.numRxThreadsRadio << endl;
                }
                for (int i = 0; i < application->configuration.numRxThreadsRadio; i++) {
                    threads.push_back(thread(ldmRx));
                }
            }
        }
        else {
            if (cam) {
                threads.push_back(thread(receive, MessageType::CAM, 0));
            }
            else if (denm)
            {
                threads.push_back(thread(receive, MessageType::DENM, 0));
            }
            else {
                // TODO: Implement for CAM, DENM as well
                if (application->configuration.driverVerbosity) {
                    cout << "Number of Radio RX Threads: " <<
                            (int)application->configuration.numRxThreadsRadio << endl;
                }
                for (int i = 0; i < application->configuration.numRxThreadsRadio; i++) {
                    threads.push_back(thread(receive, msgType, 0));
                }
            }
        }
    }

    if(application->configuration.driverVerbosity > 4)
        printf("Number of threads after rx is: %d\n", (int)threads.size());

    if (txSim && rxSim) {
        cout <<
            "Per building specifications, Simulating tx and rx is not supported.\n";
        return -1;
    }

    if (txSim || (application->configuration.enableTxAlways && !tx && !rx))
    {
        if(application->configuration.driverVerbosity)
            cout << "Starting sim transmit thread\n";
        if (preRecorded)
        {
            if (cam || denm) {
                    cout <<
                        "Transmit from pre-recorded file only supports BSM" << endl;
                return -1;
            }
            threads.push_back(thread(simTxRecorded, string(preRecordedFile)));
        }
        else {
            threads.push_back(thread(transmit, msgType));
        }

    }
    if(application->configuration.driverVerbosity > 4)
        printf("Number of threads after simtransmit is: %d\n", (int)threads.size());

    if (rxSim)
    {
        if (ldm) {
            if (cam || denm) {
                    cout << "LDM Mode only supports BSM" << endl;
                return -1;
            }
            // setup full ldm
            if(application->configuration.ldmSize) {
                application->setupLdm();
            }
            // TODO: Implement for CAM, DENM as well
            if (application->configuration.driverVerbosity) {
                cout << "Number of Ethernet LDM RX Threads: " <<
                        (int)application->configuration.numRxThreadsEth << endl;
            }
            for (int i = 0; i < application->configuration.numRxThreadsEth; i++) {
                threads.push_back(thread(ldmRx));
            }
        }
        else {

            if (cam) {
                threads.push_back(thread(receive, MessageType::CAM, 0));
            }
            else if (denm)
            {
                threads.push_back(thread(receive, MessageType::DENM, 0));
            }
            else {
                sem_init(&cnt_sem, 0, 1);
                // Multi-Threading Capability for RxSim
                if (application->configuration.driverVerbosity) {
                    cout << "Number of Ethernet RX Threads: " <<
                            (int)application->configuration.numRxThreadsEth << endl;
                }
                for(int i = 0; i < application->configuration.numRxThreadsEth; i++){
                    threads.push_back(thread(receive, msgType, 0));
                }
            }
        }
    }

    if(application->configuration.driverVerbosity > 4)
        printf("Number of threads after simreceive is %d\n", (int)threads.size());

    if (preRecorded && !txSim)
    {
        if (cam || denm) {
            cout << "Only BSM is supported for pre-recorded transmit" << endl;
            return -1;
        }
        threads.push_back(thread(txRecorded, string(preRecordedFile)));
    }

    if (safetyApps)
    {
        if (cam || denm) {
            cout << "Only BSM is supported for safetyApp demo" << endl;
            return -1;
        }
        threads.push_back(thread(runApps));
    }
    return 0;
}

int main(int argc, char** argv) {
    string txSimIp, rxSimIp;
    uint16_t txSimPort = 0, rxSimPort = 0;
    bool tx, rx, ldm, help, safetyApps, bsm, wsa, cam, denm, preRecorded, txSim, rxSim;
    bool tunnelTx, tunnelRx;
    tx = rx = ldm = help = safetyApps = wsa = cam = denm = tunnelTx = tunnelRx = false;
    preRecorded = txSim = rxSim = false;
    // by default bsm is true
    bsm = true;
    if (argc <= 2)
    {
        printUse();
        return 0;
    }
    string configFile = string(argv[argc-1]);
    configFileCheck(configFile);
    string preRecordedFile;
    int idx = 1;
    //Get all options and file path
    for (; idx < (argc - 1); idx++) {
        getModes(argv[idx][1], idx, argc, argv, tx, rx, ldm, help,
            safetyApps, bsm, wsa, cam, denm, preRecorded, preRecordedFile, txSim,
            rxSim, tunnelTx, tunnelRx, csv, txSimIp, rxSimIp, txSimPort, rxSimPort);
    }

    // Let user know how QITS will be running
    printf("Enabled Settings Are: ");
    // communication settings
    if(txSim)
        printf("TX SIM ON; ");
    if(tx)
        printf("TX RADIO ON; ");
    if(rxSim)
        printf("RX SIM ON; ");
    if(rx)
        printf("RX RADIO ON; ");
    if(tunnelTx)
        printf("TUNNEL TX ON; ");
    if(tunnelRx)
        printf("TUNNEL RX ON; ");
    if(ldm)
        printf("LDM ON; ");
    if(wsa) {
        bsm = false;
        printf("WSA ON; ");
    }

    // Packet/protocol type information
    if(bsm)
        printf("BSM; ");
    if(cam)
        printf("CAM; ");
    if(denm)
        printf("DENM; ");
    std::cout << "CONFIG_FILE: " <<  configFile << std::endl;

    if (setup(tx, rx, ldm, help, safetyApps, bsm, wsa, cam, denm, preRecorded,
        preRecordedFile, txSim, rxSim, tunnelTx, tunnelRx, txSimIp, rxSimIp,
        txSimPort, rxSimPort, (char*)configFile.data()) < 0) {
        cout << "Failed to launch program" << endl;
    }

    joinThreads();
    printf("Deleting application\n");
    printf("Attempting to close all flows\n");
    if(!rxSim && !txSim)
        application->closeAllRadio();
    return 0;
}
