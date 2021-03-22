/*
 *  Copyright (c) 2021, The Linux Foundation. All rights reserved.
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
 * @file: Cv2xTxReportTestApp.cpp
 *
 * @brief: This application can be used to transmit CV2X packets and listen to
 *         its own Tx meta data generated in low layers, or listen to all Tx
 *         meta data triggerred by other applications that transmit CV2X packets.
 */

#include <iostream>
#include <future>
#include <cstring>
#include <mutex>
#include <memory>
#include <stdlib.h>
#include <sys/time.h>
#include <csignal>

#include <telux/cv2x/Cv2xRadioTypes.hpp>
#include <telux/cv2x/Cv2xFactory.hpp>
#include "Cv2xTxReportTestApp.hpp"
#include "Report.hpp"
#include "../../common/utils/Utils.hpp"

using std::cout;
using std::cerr;
using std::cin;
using std::endl;
using std::promise;
using std::string;
using std::mutex;
using std::make_shared;
using std::shared_ptr;
using std::lock_guard;
using std::condition_variable;

using telux::common::ErrorCode;
using telux::common::Status;
using telux::cv2x::Cv2xFactory;
using telux::cv2x::Cv2xStatusType;
using telux::cv2x::TrafficCategory;
using telux::cv2x::TrafficIpType;
using telux::cv2x::SpsFlowInfo;
using telux::cv2x::EventFlowInfo;

#define DEFAULT_PORT (5000)
#define DEFAULT_LENGTH (200)
#define DEFAULT_INTERVAL (100)
#define DEFAULT_SERVICE_ID (1)
#define DEFAULT_LOG_FILE ("/var/log/tx_report.csv")


Cv2xStatusListener::Cv2xStatusListener(Cv2xStatus status) {
    cv2xStatus_ = status;
}

bool Cv2xStatusListener::isCv2xActive() {
    lock_guard<mutex> lock(mtx_);
    if (cv2xStatus_.txStatus == Cv2xStatusType::ACTIVE and
        cv2xStatus_.rxStatus == Cv2xStatusType::ACTIVE) {
        return true;
    }
    return false;
}

void Cv2xStatusListener::onStatusChanged(Cv2xStatus status) {
    lock_guard<mutex> lock(mtx_);
    if (status.rxStatus != cv2xStatus_.rxStatus
        or status.txStatus != cv2xStatus_.txStatus) {
        cout << "cv2x status changed, Tx: " << static_cast<int>(status.txStatus);
        cout << ", Rx: " << static_cast<int>(status.rxStatus) << endl;
        cv2xStatus_ = status;

        if (status.rxStatus == Cv2xStatusType::ACTIVE and
            status.txStatus == Cv2xStatusType::ACTIVE) {
            cv_.notify_all();
        }
    }
}

void Cv2xStatusListener::waitCv2xActive() {
    std::unique_lock<mutex> cvLock(mtx_);
    if (Cv2xStatusType::INACTIVE == cv2xStatus_.rxStatus or
        Cv2xStatusType::INACTIVE == cv2xStatus_.txStatus) {
        cerr << "Tx/Rx inactive, exit." << endl;
        Cv2xTxStatusReportApp::getInstance().deinit();
    } else if (Cv2xStatusType::ACTIVE != cv2xStatus_.rxStatus or
               Cv2xStatusType::ACTIVE != cv2xStatus_.txStatus) {
        cout << "wait for Cv2x status active." << endl;
        cv_.wait(cvLock);
    }
}

void Cv2xStatusListener::stopWaitCv2xActive() {
    cv_.notify_all();
}

Cv2xTxStatusReportApp::Cv2xTxStatusReportApp()
    : ConsoleApp("Cv2x Tx Report Test App Menu", "cmd> ") {
}

int Cv2xTxStatusReportApp::init() {
    if (appInit()) {
        return EXIT_FAILURE;
    }

    consoleInit();

    return EXIT_SUCCESS;
}

int Cv2xTxStatusReportApp::appInit() {
    bool cv2xRadioManagerStatusUpdated = false;
    telux::common::ServiceStatus cv2xRadioManagerStatus =
        telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
    std::condition_variable cv;
    std::mutex mtx;
    auto statusCb = [&](telux::common::ServiceStatus status) {
        std::lock_guard<std::mutex> lock(mtx);
        cv2xRadioManagerStatusUpdated = true;
        cv2xRadioManagerStatus = status;
        cv.notify_all();
    };

    // get handle of cv2x radio manager and wait for readiness
    auto & cv2xFactory = Cv2xFactory::getInstance();
    cv2xRadioManager_ = cv2xFactory.getCv2xRadioManager(statusCb);
    if (!cv2xRadioManager_) {
        cerr << "Failed to get Cv2xRadioManager." << endl;
        return EXIT_FAILURE;
    }
    std::unique_lock<std::mutex> lck(mtx);
    cv.wait(lck, [&] { return cv2xRadioManagerStatusUpdated; });
    if (telux::common::ServiceStatus::SERVICE_AVAILABLE !=
        cv2xRadioManagerStatus) {
        cerr << "Cv2x Radio Manager initialization failed!" << endl;
        return EXIT_FAILURE;
    }

    // Wait for cv2x radio to complete initialization
    radio_ = cv2xRadioManager_->getCv2xRadio(TrafficCategory::SAFETY_TYPE);
    if (not radio_->isReady()) {
        if (Status::SUCCESS != radio_->onReady().get()) {
            cerr << "Cv2x Radio initialization failed!" << endl;
            return EXIT_FAILURE;
        }
    }

    // get initial CV2X status
    promise<Cv2xStatus> prom;
    auto res = cv2xRadioManager_->requestCv2xStatus([&prom](Cv2xStatus status, ErrorCode code)
                                                    {
                                                        prom.set_value(status);
                                                    });
    if (Status::SUCCESS != res) {
        cerr << "Request for Cv2x status failed!" << endl;
        return EXIT_FAILURE;
    };
    Cv2xStatus status = prom.get_future().get();

    // register listener for CV2X status change
    cv2xStatusListener_ = make_shared<Cv2xStatusListener>(status);
    if (Status::SUCCESS != radio_->registerListener(cv2xStatusListener_)) {
        cerr << "Register CV2X status listener failed!" << endl;
        return EXIT_FAILURE;
    }

    // ensure cv2x active before running the test
    if (not cv2xStatusListener_->isCv2xActive()) {
        cerr << "CV2X status not Active!" << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

void Cv2xTxStatusReportApp::consoleInit() {
    shared_ptr<ConsoleAppCommand> startTxAndListenToReportCmd
        = make_shared<ConsoleAppCommand>(ConsoleAppCommand(
        "1", "Start_Tx_and_Listen_to_Report", {},
        std::bind(&Cv2xTxStatusReportApp::startTxAndListenToReportCommand, this)));

    shared_ptr<ConsoleAppCommand> stopTxAndListenToReportCmd
        = make_shared<ConsoleAppCommand>(ConsoleAppCommand(
        "2", "Stop_Tx_and_Listen_to_Report", {},
        std::bind(&Cv2xTxStatusReportApp::stopTxAndListenToReportCommand, this)));

    shared_ptr<ConsoleAppCommand> startListenToReportCmd
        = make_shared<ConsoleAppCommand>(ConsoleAppCommand(
        "3", "Start_Listen_to_Report", {},
        std::bind(&Cv2xTxStatusReportApp::startListenToReportCommand, this)));

    shared_ptr<ConsoleAppCommand> stopListenToReportCmd
        = make_shared<ConsoleAppCommand>(ConsoleAppCommand(
        "4", "Stop_Listen_to_Report", {},
        std::bind(&Cv2xTxStatusReportApp::stopListenToReportCommand, this)));

    std::vector<shared_ptr<ConsoleAppCommand>> commandsList
        = {startTxAndListenToReportCmd, stopTxAndListenToReportCmd,
           startListenToReportCmd, stopListenToReportCmd};
    ConsoleApp::addCommands(commandsList);
    ConsoleApp::displayMenu();
}

int Cv2xTxStatusReportApp::deinit() {
    cout << "Exiting..." << endl;

    // stop Tx pkts if started
    if (txThreadValid_) {
        stopTxPkts();
    }

    // deregister listeners
    if (radio_) {
        if (cv2xStatusListener_) {
            radio_->deregisterListener(cv2xStatusListener_);
        }

        deleteTxReportListener();
    }

    exit(0);
}

void Cv2xTxStatusReportApp::printOptions() {
    cout << "Tx flow options:" << endl;
    cout << "-t<flowType>    Set flow type to sps(s) or event(e), default is event" << endl;
    cout << "-p<srcPort>     Source port of Tx flow, default is " << options_.port << endl;
    cout << "-s<serviceID>   Service ID of Tx flow, default is " << options_.serviceId << endl;
    cout << "-l<length>      Tx Packet length, default is " << options_.length << endl;
    cout << "-i<interval>    Tx Packet interval(ms), default is " << options_.interval << endl;
    cout << "-w<logFile>     Tx report log csv file, default is " << options_.file << endl;
}

// init options with default value
void Cv2xTxStatusReportApp::initOptions() {
    options_.isSPS = false;
    options_.port = DEFAULT_PORT;
    options_.length = DEFAULT_LENGTH;
    options_.interval = DEFAULT_INTERVAL;
    options_.serviceId = DEFAULT_SERVICE_ID;
    options_.file = DEFAULT_LOG_FILE;
}

// Parse options for Tx flow
int Cv2xTxStatusReportApp::parseOptions() {
    int ret = EXIT_SUCCESS;

    initOptions();
    printOptions();

    cout << "Enter Tx flow options:";
    string opt;
    getline(cin, opt);
    if (opt.empty()) {
        return EXIT_SUCCESS;
    }

    char *buf = (char *)malloc(opt.size() + 1);
    if (buf == nullptr) {
        cerr << "error allocating options!"<< endl;
        return EXIT_FAILURE;
    }
    memcpy(buf, opt.c_str(), opt.size());
    buf[opt.size()] = '\0';

    char* saveptr = nullptr;
    char* tok = strtok_r(buf, " -", &saveptr);
    while (tok != nullptr && strlen(tok) > 1) {
        switch (tok[0]) {
        case 't':
            if (tok[1] == 's') {
                options_.isSPS = true;
                cout << "set sps flow type" << endl;
            } else if (tok[1] == 'e') {
                options_.isSPS = false;
                cout << "set event flow type" << endl;
            } else {
                cerr << "Invalid flow type!" << endl;
                ret = EXIT_FAILURE;
            }
            break;
        case 'p':
            options_.port = atoi(&tok[1]);
            cout << "set source port: " << options_.port << endl;
            break;
        case 's':
            options_.serviceId = atoi(&tok[1]);
            cout << "set service ID: " << options_.serviceId << endl;
            break;
        case 'l':
            options_.length = atoi(&tok[1]);
            cout << "set packet length: " << options_.length << endl;
            break;
        case 'i':
            options_.interval = atoi(&tok[1]);
            cout << "set Tx interval: " << options_.interval << endl;
            break;
        case 'w':
            options_.file = &tok[1];
            cout << "set Tx log file: " << options_.file << endl;
            break;
        default:
            cerr << "Invalid options!"<< endl;
            ret = EXIT_FAILURE;
        }
        tok = strtok_r(nullptr, " -", &saveptr);
    }

    // validate interval for SPS flow
    if (options_.isSPS) {
        if (Utils::validateV2xSpsInterval(options_.interval)) {
            cerr << "Invalid SPS period!" << endl;
            ret = EXIT_FAILURE;
        }
    }

    free(buf);
    buf = nullptr;
    return ret;
}

int Cv2xTxStatusReportApp::registerTxFlow() {
    Status status = Status::SUCCESS;

    promise<ErrorCode> p;
    shared_ptr<ICv2xTxFlow>txFlow = nullptr;
    if(options_.isSPS) {
        cout << "Registering Tx SPS Flow" << endl;
        auto createTxSpsFlowCallback = [&p, &txFlow](shared_ptr<ICv2xTxFlow> txSpsFlow,
                                          shared_ptr<ICv2xTxFlow> txEventFlow,
                                          ErrorCode spsError,
                                          ErrorCode unused) {
            if (ErrorCode::SUCCESS == spsError) {
                txFlow = txSpsFlow;
            }
            p.set_value(spsError);
        };

        SpsFlowInfo spsInfo;
        spsInfo.periodicityMs = options_.interval;
        spsInfo.nbytesReserved = options_.length;
        status = radio_->createTxSpsFlow(TrafficIpType::TRAFFIC_NON_IP,
                                         options_.serviceId,
                                         spsInfo, options_.port,
                                         false, 0,
                                         createTxSpsFlowCallback);
    } else {
        cout << "Registering Tx event Flow" << endl;
        auto createTxEventFlowCallback = [&p, &txFlow](shared_ptr<ICv2xTxFlow> txEventFlow,
                                            ErrorCode error) {
            if (ErrorCode::SUCCESS == error) {
                txFlow = txEventFlow;
            }
            p.set_value(error);
        };

        EventFlowInfo flowInfo;
        status = radio_->createTxEventFlow(TrafficIpType::TRAFFIC_NON_IP,
                                           options_.serviceId,
                                           flowInfo,
                                           options_.port,
                                           createTxEventFlowCallback);

    }

    if (Status::SUCCESS != status or
        ErrorCode::SUCCESS != p.get_future().get()) {
        cerr << "Failed to create Tx flow!" << endl;
        return EXIT_FAILURE;
    }

    txFlow_ = txFlow;
    txFlowValid_ = true;
    cout << "Succeeded in creating Tx Flow, create sock:" << txFlow_->getSock();
    cout << " , port:"<< options_.port << endl;
    return EXIT_SUCCESS;
}

int Cv2xTxStatusReportApp::deregisterTxFlow() {
    int ret = EXIT_SUCCESS;
    if (txFlowValid_) {
        cout << "Deregistering Tx flow, close sock:" << txFlow_->getSock() << endl;

        promise<ErrorCode> p;
        auto closeTxFlowCallback = [&p](shared_ptr<ICv2xTxFlow> txFlow, ErrorCode error) {
            p.set_value(error);
        };

        auto status = radio_->closeTxFlow(txFlow_, closeTxFlowCallback);
        if (Status::SUCCESS != status or
            ErrorCode::SUCCESS != p.get_future().get()) {
            cerr << "Failed to deregister Tx flow!" << endl;
            ret = EXIT_FAILURE;
        }
        txFlowValid_ = false;
    }
    return ret;
}

// Fills Tx buffer using same sequence as in acme
int Cv2xTxStatusReportApp::fillTxBuffer(char* buf, uint16_t length) {
    if (!buf or length < 6) {
        cerr << "Invalid Tx Buffer!" << endl;
        return EXIT_FAILURE;
    }

    uint16_t len = 0;
    memset(buf, 0, length);

    // Very first payload is test magic number
    buf[0] = 'Q';
    len++;

    // reserve 2 bytes for non-dummy payload data size
    len += sizeof(uint16_t);

    // UEID value
    buf[len] = 1;
    len++;

    // Sequence number
    *(uint16_t *)(&buf[len]) = htons(txCount_);
    len += sizeof(uint16_t);

    // Add timestamp if buffer size allowed
    uint64_t timestamp = Utils::getCurrentTimestamp();
    char format[] = "<%llu> ";
    uint16_t tmp = snprintf(nullptr, 0, format, timestamp);
    if (tmp + len <= length) {
        len += snprintf(buf+len, tmp, format, timestamp);
    }

    // Fill non-dummy message length
    tmp = htons(len);
    memcpy(buf+1, &tmp, sizeof(uint16_t));

    // Dummy payload
    for (int i = len; i < length; ++i) {
        buf[i] = 'a' + (i % 26);
    }

    return EXIT_SUCCESS;
}

// Function for transmitting data
 int Cv2xTxStatusReportApp::sampleTx(int sock, char* buf, uint16_t length) {
    // Send data using sendmsg to provide IPV6_TCLASS per packet
    struct msghdr message = { 0 };
    struct iovec iov[1] = { 0 };
    struct cmsghdr *cmsghp = NULL;
    char control[CMSG_SPACE(sizeof(int))];
    iov[0].iov_base = buf;
    iov[0].iov_len = length;
    message.msg_iov = iov;
    message.msg_iovlen = 1;
    message.msg_control = control;
    message.msg_controllen = sizeof(control);

    // Fill ancillary data
    int priority = 3;
    cmsghp = CMSG_FIRSTHDR(&message);
    cmsghp->cmsg_level = IPPROTO_IPV6;
    cmsghp->cmsg_type = IPV6_TCLASS;
    cmsghp->cmsg_len = CMSG_LEN(sizeof(int));
    memcpy(CMSG_DATA(cmsghp), &priority, sizeof(int));

    // Send data
    auto bytes = sendmsg(sock, &message, 0);

    // Check bytes sent
    if (bytes <= 0) {
        cerr << "Error occurred sending to sock:" << sock << " err:" << strerror(errno) << endl;
        return EXIT_FAILURE;
    }

    ++txCount_;
    cout << "TX count: " << txCount_ << " bytes:" << bytes << endl;
    return EXIT_SUCCESS;
}

void Cv2xTxStatusReportApp::startTxPkts() {
    cout << "Create thread for Tx packets..." << endl;

    //init tx count
    txCount_ = 0;

    // alloc buffer for Tx pkt
    buf_ = (char*)malloc(options_.length * sizeof(char));
    if (!buf_) {
        cerr << "Alloc Tx buffer failed!" << endl;
        return;
    }

    txThread_ = std::async(std::launch::async, [this]() {
        while (1) {
            // check if user has stopped Tx pkts
            if (not txFlowValid_) {
                cout << "Tx flow has been deregistered" << endl;
                break;
            }

            // check CV2X status before Tx
            if (cv2xStatusListener_->isCv2xActive()) {
                if (fillTxBuffer(buf_, options_.length) or
                    sampleTx(txFlow_->getSock(), buf_, options_.length)) {
                    break;
                }
            } else {
                cv2xStatusListener_->waitCv2xActive();
                continue;
            }

            usleep(options_.interval*1000);
        }
    });
    txThreadValid_ = true;
}

void Cv2xTxStatusReportApp::stopTxPkts() {
    cout << "Stop Tx packets..." << endl;

    // deregister Tx flow
    deregisterTxFlow();

    // stop waiting for cv2x status
    cv2xStatusListener_->stopWaitCv2xActive();

    // wait for Tx thread to end
    txThread_.get();
    txThreadValid_ = false;

    // free allocated tx buffer
    if (buf_) {
        free(buf_);
        buf_ = nullptr;
    }
}

int Cv2xTxStatusReportApp::createTxReportListener() {
    promise<ErrorCode> p;
    txReportListener_ = make_shared<Cv2xTxStatusReportListener>(options_.file);
    auto status = radio_->registerTxStatusReportListener(
        options_.port,
        txReportListener_,
        [&p](ErrorCode code)
        {
            p.set_value(code);
        });
    if (Status::SUCCESS != status or ErrorCode::SUCCESS != p.get_future().get()) {
        cerr << "Register CV2X Tx status report listener failed!" << endl;
        return EXIT_FAILURE;
    }
    cout << "Start listening to Tx Status Report..." << endl;

    return EXIT_SUCCESS;
}

int Cv2xTxStatusReportApp::deleteTxReportListener() {
    if (not txReportListener_) {
        cerr << "Tx status report listener not exist" << endl;
        return EXIT_FAILURE;
    }

    cout << "Stop listening to Tx Status Report" << endl;
    promise<ErrorCode> p;
    auto status = radio_->deregisterTxStatusReportListener(
        options_.port,
        [&p](ErrorCode code)
        {
            p.set_value(code);
        });
    if (Status::SUCCESS != status or ErrorCode::SUCCESS != p.get_future().get()) {
        cerr << "Deregister CV2X Tx status report listener failed!" << endl;
        return EXIT_FAILURE;
    }

    txReportListener_ = nullptr;
    return EXIT_SUCCESS;
}

void Cv2xTxStatusReportApp::startTxAndListenToReportCommand() {
    if (txThreadValid_) {
        cerr << "Tx has been started, stop Tx first(cmd 2)!" << endl;
        return;
    }

    if (txReportListener_) {
        cerr << "Listener has been registered, deregister listener first(cmd 4)!" << endl;
        return;
    }

    cout << "Start Tx and listen to status report..." << endl;

    // input options for Tx flow
    if (parseOptions()) {
        return;
    }

    // create listener with same port number as the Tx flow src port
    if (EXIT_SUCCESS != createTxReportListener()) {
        return;
    }

    // register Tx flow
    if (EXIT_SUCCESS != registerTxFlow()) {
        // delete created listener if Tx flow registration failed
        deleteTxReportListener();
        return;
    }

    // start Tx packets in async task
    startTxPkts();

    return;
}

void Cv2xTxStatusReportApp::stopTxAndListenToReportCommand() {
    if (not txThreadValid_) {
        cerr << "Tx not started!" << endl;
        return;
    }

    cout << "Stop Tx and listen to status report..." << endl;

    // stop Tx packets
    stopTxPkts();

    // wait 100ms in case the reports of the last pkt not received
    usleep(100*1000);

    // deregister Tx status report listener
    deleteTxReportListener();

    return;
}

void Cv2xTxStatusReportApp::startListenToReportCommand() {
    if (txThreadValid_) {
        cerr << "Tx has been started, stop Tx first(cmd 2)!" << endl;
        return;
    }

    if (txReportListener_) {
        cerr << "Listener has been registered, deregister listener first(cmd 4)!" << endl;
        return;
    }

    string file;
    cout << "Enter report csv file path with file name(default is " << DEFAULT_LOG_FILE << "):";
    getline(cin, file);
    if (file.empty()) {
        file = DEFAULT_LOG_FILE;
    }

    // register listener for CV2X Tx status report with port number 0, which means listen to
    // reports associated with all port number
    options_.file = file;
    options_.port = 0;
    createTxReportListener();

    return;
}

void Cv2xTxStatusReportApp::stopListenToReportCommand() {
    deleteTxReportListener();
    return;
}

Cv2xTxStatusReportApp & Cv2xTxStatusReportApp::getInstance() {
    static Cv2xTxStatusReportApp instance;
    return instance;
}

static void signalHandler(int signum) {
    std::cout << " Interrupt signal (" << signum << ") received.." << std::endl;
    Cv2xTxStatusReportApp::getInstance().deinit();
}

int main(int argc, char *argv[]) {
    std::vector<std::string> groups{"radio"};
    if (-1 == Utils::setSupplementaryGroups(groups)){
        cout << "Adding supplementary group failed!" << std::endl;
    }

    auto & app = Cv2xTxStatusReportApp::getInstance();
    if (EXIT_SUCCESS != app.init()){
        cout << "Error: Initialization failed!" << endl;
        return EXIT_FAILURE;
    }

    signal(SIGINT, signalHandler);

    // continuously read and execute commands
    app.mainLoop();

    // release radio resources when exit from mainloop
    app.deinit();
}
