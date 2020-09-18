/*
 *  Copyright (c) 2018-2020, The Linux Foundation. All rights reserved.
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
#include <regex>
#include "SaeApplication.hpp"
#include "EtsiApplication.hpp"
#include "safetyapp_util.h"
#include "bsm_utils.h"

using std::thread;
using std::string;
using std::list;
using std::ref;
using std::lock_guard;
using std::mutex;

// Comment it out if you don't want to dump the raw received packets.
//#define DUMP_RAW    1

// Global variables
static ApplicationBase* application = nullptr;
static vector<thread> threads;
static bool csv = false;
static string csvFileName;

static void joinThreads() {
    for (int i = 0; i < threads.size(); i++)
    {
        threads[i].join();
    }
}

static void signalHandler(int signum) {
    cout << "Interrupt signal (" << signum << ") received.\n";
    application->closeAllRadio();
    exit(signum);
}
/**
 * receiving thread function.
 *
 * @param[in] msgType type of the messsage we are processing.
 */
static void receive(MessageType msgType) {
    std::signal(SIGINT, signalHandler);
    auto count = 0;
    FILE *fp;
    if (csv == true) {
        fp = fopen(csvFileName.c_str(), "w+");
        if (!fp) {
            cerr << "Failed to open file " << csvFileName << " for writting" << endl;
            return;
        }
    }
    while (true)
    {
        const auto mc = application->receivedContents[0];
        abuf_reset(&mc->abuf, 256);
        auto recCount = application->radioReceives[0].receive(mc->abuf.data);
        abuf_put(&mc->abuf, recCount);
        if(recCount <= 0){
            cout << "Nothing received, continuing\n";
            continue;
        }
#ifdef DUMP_RAW
        cout << "Packet # " << count << " with length " << recCount << endl << endl;
        cout << "Hex: ";
        for (uint16_t t = 0; t < recCount; t++) {
            if (t > 0) printf(":");
            printf("%02X", mc->abuf.data[t]);
        }
        cout << endl;
        count += 1;
#endif
        // call application's receive() function to process the packet across
        // stack layers.
        if (application->receive(0, recCount) == 0) {
            if (msgType == MessageType::BSM) {
                print_summary_RV(mc.get());
                if(csv){
                    write_to_csv(mc.get(), fp);
                }
            } else if (msgType == MessageType::CAM) {
                print_cam(mc->cam);
            } else {
                print_denm(mc->denm);
            }
        }
    }
}
/**
 * receive function for LDM functionality test.
 *
 * @param [in] msgType, so far only BSM is supported.
 */
static void ldmRx(MessageType msgType) {
    if (msgType != MessageType::BSM) {
        cerr << "Only BSM is supported in ldmRx" << endl;
        return;
    }
    std::signal(SIGINT, signalHandler);
    while (true)
    {
        const auto mc = application->receivedContents[0];
        const auto recCount = application->radioReceives[0].receive(mc->abuf.data);
        abuf_put(&mc->abuf, recCount);
        const auto ldmIndex = application->ldm->getFreeBsm();
        application->receive(0, recCount, ldmIndex);
    }
}
/**
 * Initialize timer for transmit
 * @param[in] interval_ns timer interval value in nano seconds
 * @return timer's file descriptor if success or -1 on failure.
 */
static int start_tx_timer(long long interval_ns) {
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
static void transmit(MessageType msgType) {
    std::signal(SIGINT, signalHandler);
    int tx_timer_fd = -1;
    int timer_misses = 0;
    uint64_t exp;
    ssize_t s;
    //auto timer = timestamp_now();
    tx_timer_fd = start_tx_timer(1000000*application->configuration.transmitRate);
    if (tx_timer_fd == -1) {
        cerr << "Failed to start Tx timer" << endl;
        return;
    }
    switch (msgType)
    {
    case MessageType::CAM:
    case MessageType::BSM:
        while (true)
        {
            if (application->send(0, TransmitType::SPS) < 0) {
                cerr << "Failed to send message" << endl;
                return;
            }
            s = read(tx_timer_fd, &exp, sizeof(uint64_t));
            if (s == sizeof(uint64_t) && exp > 1) {
                timer_misses += (exp-1);
                cout << "TX timer overruns: Total missed: " << timer_misses << endl;
            }
        }
        break;
    case MessageType::DENM:
        cerr << "DENM transmit is not supported" << endl;
    default:
        break;
    }
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
static void txRecorded(string file, MessageType msgType) {
    srand(timestamp_now());
    ifstream configFile(file);
    string line;
    bool go = true;

    if (msgType != MessageType::CAM) {
        cerr << "Only BSM message is supported in this mode" << endl;
        return;
    }
    if (configFile.is_open())
    {
        auto timer = timestamp_now();
        while (go) {
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
static void simTxRecorded(string file, MessageType msgType) {

    if (msgType != MessageType::BSM) {
        cerr << "Only BSM is supported in this test mode" << endl;
        return;
    }
    ifstream configFile(file);
    string line;
    if (configFile.is_open())
    {
        auto timer = timestamp_now();
        while (true)
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

static void tunnelModeTx(MessageType msgType) {
    if (msgType != MessageType::BSM) {
        cerr << "Only BSM is supported in tunnelModeTx" << endl;
        return;
    }
    std::signal(SIGINT, signalHandler);
    auto timer = timestamp_now();
    while (true)
    {
        if (timer + application->configuration.transmitRate < timestamp_now()) {
            SaeApplication *SaeApp = dynamic_cast<SaeApplication *>(application);
            SaeApp->sendTuncBsm(0, TransmitType::SPS);
            timer = timestamp_now();
        }
    }
}

static void tunnelModeRx(MessageType msgType) {
    while (true)
    {
        SaeApplication *SaeApp = dynamic_cast<SaeApplication *>(application);
        const auto mc = SaeApp->receivedContents[0];
        const auto recCount = SaeApp->radioReceives[0].receive(mc->abuf.data);
        abuf_put(&mc->abuf, recCount);
        const auto ldmIndex = application->ldm->getFreeBsm();
        SaeApp->receiveTuncBsm(0, recCount, ldmIndex);
        if (!SaeApp->ldm->filterBsm(ldmIndex)) {
            const auto bsm = static_cast<bsm_value_t *>(mc->j2735_msg);
            SaeApp->ldm->setIndex(bsm->id, ldmIndex);
        }
    }
}

/**
 * run safety application.
 */
static void runApps(MessageType msgType) {
    if (msgType != MessageType::BSM) {
        cerr << "Only BSM is supported in tunnelModeTx" << endl;
        return;
    }
    auto hostMsg = std::make_shared<msg_contents>();
    rv_specs* rvSpecs = new rv_specs;
    std::signal(SIGINT, signalHandler);
    while (true) {
        for (auto rvMsg : application->ldm->bsmSnapshot()) {
            application->fillMsg(hostMsg);
            fill_RV_specs(hostMsg.get(), &rvMsg, rvSpecs);
            forward_collision_warning(&rvMsg, rvSpecs);
            EEBL_warning(&rvMsg, rvSpecs);
            accident_ahead_warning(&rvMsg, rvSpecs);
            print_rvspecs(rvSpecs);
        }
    }
}

static void simReceive(MessageType msgType) {
    std::signal(SIGINT, signalHandler);
    auto count = 0;
    auto empty = 0;
    FILE *fp;
    /*if (csv == true) {
        fp = fopen(csvFileName.c_str(), "w+");
        if (!fp) {
            cerr << "Failed to open file " << csvFileName << " for writting" << endl;
            return;
        }
    }*/
    while (true)
    {
        // begin waiting and receiving packets
        auto recCount = application->simReceive->receive(application->rxSimMsg->abuf.data);
        abuf_put(&application->rxSimMsg->abuf, recCount);
        if (recCount == 0) {
            cout << "Received empty packet # " << empty << ".\n";
            if (empty > 10) {
                cout << "Received more than 10, empty packets... Closing connections.\n";
                application->closeAllRadio();
            }
            else {
                empty++;
            }
        }
        else {
#ifdef DUMP_RAW
            cout << "Packet # " << count << " with length " << recCount << endl << endl;
            cout << "Hex: ";
            for (uint16_t t = 0; t < recCount; t++) {
                if (t > 0) printf(":");
                printf("%02X", application->rxSimMsg->abuf.data[t]);
            }
            cout << endl;
            count += 1;
#endif

            application->receive(0, recCount);
            auto mc = application->rxSimMsg;
            print_summary_RV(mc.get());
            if (csv) {
                write_to_csv(mc.get(), fp);
            }
        }
    }
}

static void simLdmRx(MessageType msgType) {
    std::signal(SIGINT, signalHandler);
    auto count = 0;
    auto empty=0;
    FILE *fp;
    if (csv == true) {
        fp = fopen(csvFileName.c_str(), "w+");
        if (!fp) {
            cerr << "Failed to open file " << csvFileName << " for writting" << endl;
            return;
        }
    }
    switch (msgType)
    {
    case MessageType::CAM:
    case MessageType::DENM:
        cerr << "CAM and DENM LDM is not supported" << endl;
        break;
    case MessageType::BSM:
        while (true)
        {
            auto recCount = application->simReceive->receive(application->rxSimMsg->abuf.data);
            abuf_put(&application->rxSimMsg->abuf, recCount);
            if (recCount == 0) {
                cout << "Received empty packet # " << empty << ".\n";
                if (empty > 10) {
                    cout << "Received more than 10, empty packets... Closing connections.\n";
                    application->closeAllRadio();
                }
                else {
                    empty++;
                }
            }
            else {
#ifdef DUMP_RAW
            cout << "Packet # " << count << " with length " << recCount << endl << endl;
            cout << "Hex: ";
            for (uint16_t t = 0; t < recCount; t++) {
                if (t > 0) printf(":");
                printf("%02X", application->rxSimMsg->abuf.data[t]);
            }
            cout << endl;
            count += 1;
#endif
                const auto ldmIndex = application->ldm->getFreeBsm();
                application->receive(0, recCount, ldmIndex);
                auto msg = &application->ldm->bsmContents[ldmIndex];
                if (csv) {
                    write_to_csv(msg, fp);
                }
                count += 1;
            }
        }
        break;
    default:
        break;
    }

}

static void simTransmit(MessageType msgType) {
    std::signal(SIGINT, signalHandler);
    auto timer = timestamp_now();
    switch (msgType)
    {
    case MessageType::DENM:
        cerr << "DENM transmit is not supported" << endl;
        break;
    case MessageType::CAM:
    case MessageType::BSM:

        while (true)
        {
            if (timer + application->configuration.transmitRate < timestamp_now()) {
                application->send(0, TransmitType::SPS);
                timer = timestamp_now();
            }
        }
        break;
    default:
        break;
    }
}

void printUse() {
    cout << "Usage: qits [options] <Config File Relative Path>\n\n\n";
    cout << "Example: qits -t -l -s \\home\\root\\ObeConfig.conf\n";
    cout << "Example above will run: transmit mode, ldm receive mode and the safety apps.\n\n";
    cout << "Example: qits -r -b \\home\\root\\ObeConfig.conf\n";
    cout << "Example above will run: receive mode with basic safety messages.\n\n";
    cout << "At least one option is needed and Config File is always required.\n";
    cout << "Options:\n";
    cout << "-h Prints help options.\n";
    cout << "-t Transmits Cv2x data. Runs by default with -b. See -b.\n";
    cout << "-r Receives Cv2x data. Runs by defaullt with -b. See -b.\n";
    cout << "-s Safety Apps Mode; Adds -l if not specified. Runs by default with -b.\n";
    cout << "-p <Pre-Recorded File Path>  Transmists from pre-recorded file.\n";
    cout << "-T Tunnel Transmit.\n";
    cout << "-x Tunnel Receive. It automatically calls -l. See: -l.\n";
    cout << "-l LDM mode; Adds -r if nothing specified. Use it with -r or -j.\n";
    cout << "-b Transmits and Receives BSMS.\n";
    cout << "-c Transmits and Receives CAMs.\n";
    cout << "-d Transmits and Receives DENMs.\n";
    cout << "-i <other_device_ip_address> <port>  Simulating CV2x with kinematics";
    cout << " and can interfaces. Transmit only.\n";
    cout << "           note: You may enable UDP if desired via the config file\n";
    cout << "-j <other_device_ip_address> <port>  Simulates CV2x and sends packets via TCP ";
    cout << "instead of OTA.\n";
    cout << "           note: You may enable UDP if desired via the config file\n";
    cout << "-o <CSV file path> write received BSM into CSV file.\n";
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

void getModes(char mode, int& argc, char** argv, bool& tx, bool& rx,
    bool& ldm, bool& help, bool& safetyApps, bool& bsm,
    bool& cam, bool& denm, bool& preRecorded, string& preRecordedFile,
    bool& txSim, bool& rxSim, bool& tunnelTx, bool& tunnelRx, bool& csv, string& txSimIp,
    string& rxSimIp, uint16_t& txSimPort, uint16_t& rxSimPort) {
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
        break;
    case 'c':
        cam = true;
        bsm = false;
        break;
    case 'd':
        denm = true;
        bsm =false;
        break;
    case 'i':
        txSim = true;
        argc += 1;
        txSimIp = string(argv[argc]);
        argc += 1;
        txSimPort = stoi(string(argv[argc]), nullptr, 10);
        break;
    case 'j':
        rxSim = true;
        argc += 1;
        rxSimIp = string(argv[argc]);
        argc += 1;
        rxSimPort = stoi(string(argv[argc]), nullptr, 10);
        break;
    case 'o':
        csv = true;
        argc+=1;
        csvFileName = string(argv[argc]);
        break;
    default:
        break;
    }
}

void setup(const bool tx, const bool rx,
    const bool ldm, const bool help, const bool safetyApps,
    const bool bsm, const bool cam, const bool denm, const bool preRecorded,
    const string preRecordedFile, const bool txSim, const bool rxSim, const bool tunnelTx,
    const bool tunnelRx, const string txSimIp, const string  rxSimIp, const  uint16_t txSimPort,
    const uint16_t rxSimPort, char* configFile) {
    if (help)
    {
        printUse();
    }

    if (bsm) {
        if (txSim)
            application = new SaeApplication(txSimIp, txSimPort, string(""), 0, configFile);
        else if (rxSim)
            application = new SaeApplication(string(""), 0, rxSimIp, rxSimPort, configFile);
        else
            application = new SaeApplication(configFile);
    } else {
        if (txSim)
            application = new EtsiApplication(txSimIp, txSimPort, string(""), 0, configFile);
        else if (rxSim)
            application = new EtsiApplication(string(""), 0, rxSimIp, rxSimPort, configFile);
        else
            application = new EtsiApplication(configFile);
    }
    if (tx && !txSim)
    {
        if (tunnelTx) {
            if (bsm) {
                threads.push_back(thread(tunnelModeTx, MessageType::CAM));
            } else if (cam) {
                threads.push_back(thread(tunnelModeTx, MessageType::CAM));
            } else {
                threads.push_back(thread(tunnelModeTx, MessageType::DENM));
            }
        } else {
            if (bsm) {
                threads.push_back(thread(transmit, MessageType::BSM));
            } else if(cam) {
                threads.push_back(thread(transmit, MessageType::CAM));
            } else {
                threads.push_back(thread(transmit, MessageType::DENM));
            }
        }
    }

    if (rx && !rxSim)
    {
        if (ldm)
        {
            if (tunnelRx) {
                if (cam) {
                    threads.push_back(thread(tunnelModeRx, MessageType::CAM));
                }
                else if (denm)
                {
                    threads.push_back(thread(tunnelModeRx, MessageType::DENM));
                }
                else {
                    threads.push_back(thread(tunnelModeRx, MessageType::BSM));
                }
            }
            else {
                if (cam) {
                    threads.push_back(thread(ldmRx, MessageType::CAM));
                }
                else if (denm)
                {
                    threads.push_back(thread(ldmRx, MessageType::DENM));
                }
                else {
                    threads.push_back(thread(ldmRx, MessageType::BSM));
                }
            }
        }
        else {
            if (cam) {
                threads.push_back(thread(receive, MessageType::CAM));
            }
            else if (denm)
            {
                threads.push_back(thread(receive, MessageType::DENM));
            }
            else {
                threads.push_back(thread(receive, MessageType::BSM));
            }
        }

    }


    if (txSim && rxSim) {
        cout << "Per building specifications, Simulating tx and rx is not supported.\n";
        return;
    }


    if (txSim)
    {
        if (preRecorded)
        {
            if (cam) {
                threads.push_back(thread(simTxRecorded, string(preRecordedFile), MessageType::CAM));
            }
            else if (denm)
            {
                threads.push_back(thread(simTxRecorded, string(preRecordedFile), MessageType::DENM));
            }
            else {
                threads.push_back(thread(simTxRecorded, string(preRecordedFile), MessageType::BSM));
            }
        }
        else {
            if (cam) {
                threads.push_back(thread(simTransmit, MessageType::CAM));
            }
            else if (denm)
            {
                threads.push_back(thread(simTransmit, MessageType::DENM));
            }
            else {
                threads.push_back(thread(simTransmit, MessageType::BSM));
            }
        }

    }

    if (rxSim)
    {
        if (ldm) {

            if (cam) {
                threads.push_back(thread(simLdmRx, MessageType::CAM));
            }
            else if (denm)
            {
                threads.push_back(thread(simLdmRx, MessageType::DENM));
            }
            else {
                threads.push_back(thread(simLdmRx, MessageType::BSM));
            }
        }
        else {

            if (cam) {
                threads.push_back(thread(simReceive, MessageType::CAM));
            }
            else if (denm)
            {
                threads.push_back(thread(simReceive, MessageType::DENM));
            }
            else {
                threads.push_back(thread(simReceive, MessageType::BSM));
            }
        }
    }

    if (preRecorded && !txSim)
    {
        if (cam) {
            threads.push_back(thread(txRecorded, string(preRecordedFile), MessageType::CAM));
        }
        else if (denm)
        {
            threads.push_back(thread(txRecorded, string(preRecordedFile), MessageType::DENM));
        }
        else {
            threads.push_back(thread(txRecorded, string(preRecordedFile), MessageType::BSM));
        }
    }

    if (safetyApps)
    {
        if (cam) {
            threads.push_back(thread(runApps, MessageType::CAM));
        }
        else if (denm)
        {
            threads.push_back(thread(runApps,MessageType::DENM));
        }
        else {
            threads.push_back(thread(runApps, MessageType::BSM));
        }
    }

}

int main(int argc, char** argv) {
    string txSimIp, rxSimIp;
    uint16_t txSimPort = 0, rxSimPort = 0;
    bool tx, rx, ldm, help, safetyApps, bsm, cam, denm, preRecorded, txSim, rxSim, tunnelRx;
    bool tunnelTx;
    tx = rx = ldm = help = safetyApps = cam = denm = tunnelTx = tunnelRx = false;
    bsm = preRecorded = txSim = rxSim = false;
    if (argc <= 2)
    {
        printUse();
        return 0;
    }
    string configFile = string(argv[argc-1]);
    configFileCheck(configFile);

    string preRecordedFile;
    int count = 1;
    //Get all options and file path
    for (; count < (argc - 1); count++) {
        getModes(argv[count][1], count, argv, tx, rx, ldm, help,
            safetyApps, bsm, cam, denm, preRecorded, preRecordedFile, txSim,
            rxSim, tunnelTx, tunnelRx, csv, txSimIp, rxSimIp, txSimPort, rxSimPort);
    }

    setup(tx, rx, ldm, help, safetyApps, bsm, cam, denm, preRecorded,
        preRecordedFile, txSim, rxSim, tunnelTx, tunnelRx, txSimIp, rxSimIp,
        txSimPort, rxSimPort, (char*)configFile.data());

    joinThreads();
    return 0;
}
