/*
 *  Copyright (c) 2020, The Linux Foundation. All rights reserved.
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
 * @file: Cv2xTcpTest.cpp
 *
 * @brief: Simple application that demonstrates Tx/Rx TCP packets in Cv2x
 */

#include <arpa/inet.h>
#include <linux/tcp.h>
#include <net/if.h>
#include <signal.h>
#include <errno.h>
#include <assert.h>
#include <ifaddrs.h>
#include <cstring>
#include <string>
#include <sys/time.h>
#include <unistd.h>
#include <iostream>
#include <memory>
#include <telux/cv2x/Cv2xRadio.hpp>


using std::array;
using std::string;
using std::cerr;
using std::cout;
using std::endl;
using std::promise;
using std::shared_ptr;
using std::atomic;
using std::mutex;
using std::lock_guard;
using std::unique_lock;
using std::condition_variable;
using telux::common::ErrorCode;
using telux::common::Status;
using telux::cv2x::Cv2xFactory;
using telux::cv2x::Cv2xStatus;
using telux::cv2x::Cv2xStatusType;
using telux::cv2x::ICv2xRadio;
using telux::cv2x::ICv2xTxRxSocket;
using telux::cv2x::Periodicity;
using telux::cv2x::Priority;
using telux::cv2x::TrafficCategory;
using telux::cv2x::SocketInfo;
using telux::cv2x::EventFlowInfo;
using telux::cv2x::ICv2xRadio;
using telux::cv2x::ICv2xRadioListener;
using telux::cv2x::ICv2xListener;
using telux::cv2x::ICv2xRadioManager;

static constexpr uint32_t SERVIC_ID = 1u;
static constexpr uint8_t TCP_CLIENT = 0u;
static constexpr uint8_t TCP_SERVER = 1u;
static constexpr uint16_t DEFAULT_PORT = 5000u;
static constexpr int      PRIORITY = 5;
static constexpr uint32_t PACKET_LEN = 128u;
static constexpr uint32_t MAX_DUMMY_PACKET_LEN = 10000;

static constexpr char TEST_VERNO_MAGIC = 'Q';
static constexpr char CLIENT_UEID = 1;
static constexpr char SERVER_UEID = 2;

static shared_ptr<ICv2xRadioManager> gCv2xRadioMgr = nullptr;
static shared_ptr<ICv2xRadio> gCv2xRadio = nullptr;
static shared_ptr<ICv2xRadioListener> gRadioListener = nullptr;
static shared_ptr<ICv2xListener> gStatusListener = nullptr;
static shared_ptr<ICv2xTxRxSocket> gTcpSockInfo = nullptr;
static int32_t gTcpSocket = -1;
static int32_t gAcceptedSock = -1;
static Cv2xStatus gCv2xStatus;
static mutex gCv2xStatusMutex;
static condition_variable gStatusCv;
static promise<ErrorCode> gCallbackPromise;
static array<char, MAX_DUMMY_PACKET_LEN> gBuf;
static uint8_t gTcpMode = TCP_CLIENT;
static uint16_t gSrcPort = DEFAULT_PORT;
static uint16_t gDstPort = DEFAULT_PORT;
static string gDstAddr;
static uint32_t gServiceId = SERVIC_ID;
static uint32_t gPacketLen = PACKET_LEN;
static uint32_t gPacketNum = 0;
static uint32_t gTxCount = 0u;
static uint32_t gRxCount = 0u;
static atomic<bool> gTcpConnected{false};
static atomic<int> gTerminate{0};
static int gTerminatePipe[2];
static mutex gOperationMutex;

class RadioListener : public ICv2xRadioListener {
public:
    void onL2AddrChanged(uint32_t newL2Address) {
        cout << "source L2 address changed to:" << newL2Address << endl;
        // local-link address has changed after TCP connection establishment,
        // the TCP connection cannot be used now, need to exit
        if (newL2Address > 0 and gTcpConnected) {
            cerr << "local-link address has changed, need exit and re-start test!" << endl;
            gTerminate = 1;
            write(gTerminatePipe[1], &gTerminate, sizeof(int));
            gStatusCv.notify_all();
        }
    }
};

class Cv2xStatusListener : public ICv2xListener {
public:
    void onStatusChanged(Cv2xStatus status) override {
        lock_guard<mutex> lock(gCv2xStatusMutex);
        if (status.rxStatus != gCv2xStatus.rxStatus
            or status.txStatus != gCv2xStatus.txStatus) {
            cout << "cv2x status changed, Tx: " << static_cast<int>(status.txStatus);
            cout << ", Rx: " << static_cast<int>(status.rxStatus) << endl;
            gCv2xStatus = status;

            if (status.rxStatus == Cv2xStatusType::ACTIVE and
                status.txStatus == Cv2xStatusType::ACTIVE) {
                gStatusCv.notify_all();
            }
        }
    }
};

static bool isV2xReady() {
    lock_guard<mutex> lock(gCv2xStatusMutex);
    if (Cv2xStatusType::ACTIVE == gCv2xStatus.rxStatus and
        Cv2xStatusType::ACTIVE == gCv2xStatus.txStatus) {
        return true;
    }

    cout << "cv2x Tx/Rx not active!" << endl;
    return false;
}

static void waitV2xStatusActive() {
    std::unique_lock<std::mutex> cvLock(gCv2xStatusMutex);
    while (!gTerminate and
           (Cv2xStatusType::ACTIVE != gCv2xStatus.rxStatus or
            Cv2xStatusType::ACTIVE != gCv2xStatus.txStatus)) {
        cout << "wait for Cv2x status active." << endl;
        gStatusCv.wait(cvLock);
    }
}

// Resets the global callback promise
static inline void resetCallbackPromise(void) {
    gCallbackPromise = promise<ErrorCode>();
}

// Callback function for ICv2xRadioManager->requestCv2xStatus()
static void cv2xStatusCallback(Cv2xStatus status, ErrorCode error) {
    if (ErrorCode::SUCCESS == error) {
        gCv2xStatus = status;
    }
    gCallbackPromise.set_value(error);
}

// Callback function for ICv2xRadio->createCv2xTcpSocket()
static void createTcpSocketCallback(shared_ptr<ICv2xTxRxSocket> sock,
                                    ErrorCode error) {
    if (ErrorCode::SUCCESS == error) {
        gTcpSockInfo = sock;
    }
    gCallbackPromise.set_value(error);
}

// Returns current timestamp
static uint64_t getCurrentTimestamp(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000ull + tv.tv_usec;
}

// Fills buffer with dummy data
static void fillBuffer(void) {
    static uint16_t seq_num = 0u;
    auto timestamp = getCurrentTimestamp();

    // Very first payload is test Magic number, this is  where V2X Family ID would normally be.
    gBuf[0] = TEST_VERNO_MAGIC;

    // Next byte is the UE equipment ID
    if (gTcpMode == TCP_CLIENT) {
        gBuf[1] = CLIENT_UEID;
    } else {
        gBuf[1] = SERVER_UEID;
    }

    // Sequence number
    auto dataPtr = gBuf.data() + 2;
    uint16_t tmp = htons(seq_num++);
    memcpy(dataPtr, reinterpret_cast<char *>(&tmp), sizeof(uint16_t));
    dataPtr += sizeof(uint16_t);

    // Timestamp
    dataPtr += snprintf(dataPtr, gPacketLen - (2 + sizeof(uint16_t)),
                        "<%llu> ", static_cast<long long unsigned>(timestamp));

    // Dummy payload
    constexpr int NUM_LETTERS = 26;
    auto i = 2 + sizeof(uint16_t) + sizeof(long long unsigned);
    for (; i < gPacketLen; ++i) {
        gBuf[i] = 'a' + ((seq_num + i) % NUM_LETTERS);
    }
}

// Function for transmitting data
static int sampleTx(int32_t sock) {
    cout << "sampleTx(" << sock << ")" << endl;

    if (sock < 0) {
        return EXIT_FAILURE;
    }

    struct msghdr message = {0};
    struct iovec iov[1] = {0};
    struct cmsghdr * cmsghp = NULL;
    char control[CMSG_SPACE(sizeof(int))];

    // Send data using sendmsg to provide IPV6_TCLASS per packet
    iov[0].iov_base = gBuf.data();
    iov[0].iov_len = gPacketLen;
    message.msg_iov = iov;
    message.msg_iovlen = 1;
    message.msg_control = control;
    message.msg_controllen = sizeof(control);

    // Fill ancillary data
    int priority = PRIORITY;
    cmsghp = CMSG_FIRSTHDR(&message);
    cmsghp->cmsg_level = IPPROTO_IPV6;
    cmsghp->cmsg_type = IPV6_TCLASS;
    cmsghp->cmsg_len = CMSG_LEN(sizeof(int));
    memcpy(CMSG_DATA(cmsghp), &priority, sizeof(int));

    // Send data
    auto sendBytes = sendmsg(sock, &message, 0);

    // Check bytes sent
    if (sendBytes <= 0) {
        cerr << "Error occurred sending to sock:" << sock << " err:" << strerror(errno) << endl;
    } else {
        uint16_t seq = 0;
        memcpy(reinterpret_cast<char *>(&seq), gBuf.data() + 2, sizeof(uint16_t));
        seq = ntohs(seq);
        ++gTxCount;
        cout << "TX count: " << gTxCount << " bytes:" << sendBytes;
        cout << " UEID:" << static_cast<int>(gBuf[1]);
        cout << " SEQ:" << seq << endl;
    }

    return sendBytes;
}

// Function for reading from Rx socket
static int sampleRx(int32_t sock) {
    cout << "sampleRx(" << sock << ")" << endl;

    if (sock < 0) {
        return EXIT_FAILURE;
    }

    // Attempt to read from socket
    int recvBytes = recv(sock, gBuf.data(), gBuf.max_size(), 0);

    if (recvBytes <= 0) {
        cerr << "Error occurred reading from sock:" << sock << " err:" << strerror(errno) << endl;
    } else {
        ++gRxCount;
        uint16_t seq = 0;
        memcpy(reinterpret_cast<char *>(&seq), gBuf.data() + 2, sizeof(uint16_t));
        seq = ntohs(seq);
        cout << "RX count: " << gRxCount << " bytes:" << recvBytes;
        cout << " UEID:" << static_cast<int>(gBuf[1]);
        cout << " SEQ:" << seq << endl;
    }

    return recvBytes;
}

// Callback for ICv2xRadio->closeCv2xTcpSocket()
static void closeTcpSocketCallback(shared_ptr<ICv2xTxRxSocket> chan, ErrorCode error) {
    gCallbackPromise.set_value(error);
}

static void printUsage(const char *Opt) {
    cout << "Usage: " << Opt << endl;
    cout << "-d <dstAddr>       Destination IPV6 address used for connecting" << endl;
    cout << "-m <tcpMode>       0--Client, 1--Server" << endl;
    cout << "-s <srcPort>       Source port used for binding, default is 5000" << endl;
    cout << "-t <dstPort>       Destination port used for connecting, default is 5000" << endl;
    cout << "-p <service ID>    Service ID used for Tx and Rx flows, default is ";
    cout << gServiceId << endl;
    cout << "-l <packet length> Tx Packet length, default is " << gPacketLen <<endl;
    cout << "-n <packet number> Tx Packet number" <<endl;
}

// Parse options
static int parseOpts(int argc, char *argv[]) {
    int rc = 0;
    int c;
    while ((c = getopt(argc, argv, "?d:m:s:t:p:l:n:")) != -1) {
        switch (c) {
        case 'd':
            if (optarg) {
                gDstAddr = optarg;
                cout << "dstAddr: " << gDstAddr << endl;
            }
            break;
        case 'm':
            if (optarg) {
                gTcpMode = atoi(optarg);
                cout << "tcpMode: " << gTcpMode << endl;
            }
            break;
        case 's':
            if (optarg) {
                gSrcPort = atoi(optarg);
                cout << "srcPort: " << gSrcPort << endl;
            }
            break;
        case 't':
            if (optarg) {
                gDstPort = atoi(optarg);
                cout << "dstPort: " << gDstPort << endl;
            }
            break;
        case 'p':
            if (optarg) {
                gServiceId = atoi(optarg);
                cout << "service ID: " << gServiceId << endl;
            }
            break;
        case 'l':
            if (optarg) {
                gPacketLen = atoi(optarg);
                cout << "packet length: " << gPacketLen << endl;
            }
            break;
        case 'n':
            if (optarg) {
                gPacketNum = atoi(optarg);
                cout << "packet number: " << gPacketNum << endl;
            }
            break;
        case '?':
        default:
            rc = -1;
            printUsage(argv[0]);
            return rc;
        }
    }

    if (gTcpMode == TCP_CLIENT && gDstAddr.empty()) {
        cout << "error Destination IP Addr." << endl;
        rc = -1;
    }

    return rc;
}

static int init() {
    lock_guard<mutex> lock(gOperationMutex);

    // Get handle to Cv2xRadioManager
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

    auto & cv2xFactory = Cv2xFactory::getInstance();
    auto gCv2xRadioMgr = cv2xFactory.getCv2xRadioManager(statusCb);
    if (!gCv2xRadioMgr) {
        cout << "Error: failed to get Cv2xRadioManager." << endl;
        return EXIT_FAILURE;
    }
    std::unique_lock<std::mutex> lck(mtx);
    cv.wait(lck, [&] { return cv2xRadioManagerStatusUpdated; });
    if (telux::common::ServiceStatus::SERVICE_AVAILABLE !=
        cv2xRadioManagerStatus) {
        cerr << "C-V2X Radio Manager initialization failed, exiting" << endl;
        return EXIT_FAILURE;
    }

    // Wait for radio manager to complete initialization
    if (not gCv2xRadioMgr->isReady()) {
        if (gCv2xRadioMgr->onReady().get()) {
            cout << "C-V2X Radio Manager is ready" << endl;
        }
        else {
            cerr << "C-V2X Radio Manager initialization failed, exiting" << endl;
            return EXIT_FAILURE;
        }
    }

    // Get C-V2X status and make sure Tx/Rx is active
    assert(Status::SUCCESS == gCv2xRadioMgr->requestCv2xStatus(cv2xStatusCallback));
    assert(ErrorCode::SUCCESS == gCallbackPromise.get_future().get());

    if (Cv2xStatusType::ACTIVE == gCv2xStatus.txStatus) {
        cout << "C-V2X TX/RX status is active" << endl;
    } else {
        cerr << "C-V2X TX/RX is inactive" << endl;
        return EXIT_FAILURE;
    }

    // Get handle to Cv2xRadio
    gCv2xRadio = gCv2xRadioMgr->getCv2xRadio(TrafficCategory::SAFETY_TYPE);

    // Wait for radio to complete initialization
    if (not gCv2xRadio->isReady()) {
        if (Status::SUCCESS == gCv2xRadio->onReady().get()) {
            cout << "C-V2X Radio is ready" << endl;
        }
        else {
            cerr << "C-V2X Radio initialization failed." << endl;
            return EXIT_FAILURE;
        }
    }

    // Register for Src L2 Id Update callbacks
    gRadioListener = std::make_shared<RadioListener>();
    if (Status::SUCCESS != gCv2xRadio->registerListener(gRadioListener)) {
        cerr << "Radio listener registration failed." << endl;
        return EXIT_FAILURE;
    }

    // Register for cv2x status update
    gStatusListener = std::make_shared<Cv2xStatusListener>();
    if (Status::SUCCESS != gCv2xRadioMgr->registerListener(gStatusListener)) {
        cerr << "Status listener registration failed." << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

static int connectTcpSocketClient(int sock) {
    // For TCP client, establish connection with the created sock
    struct sockaddr_in6 dstSockAddr = {0}; //must reset the sockaddr
    dstSockAddr.sin6_port = htons((uint16_t)gDstPort);
    inet_pton(AF_INET6, gDstAddr.c_str(), (void *)&dstSockAddr.sin6_addr);
    dstSockAddr.sin6_family = AF_INET6;

    cout << "connecting sock:" << sock << endl;
    if (connect(sock, (struct sockaddr *)&dstSockAddr, sizeof(struct sockaddr_in6))) {
        cout << "connect err:" << strerror(errno) << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

static int acceptTcpSocketServer(int sock) {
    // mark the created socket as listening sock
    cout << "listening sock" << sock << endl;
    if (listen(sock, 5) < 0) {
        cout << "connect err:" << strerror(errno) << endl;
        return EXIT_FAILURE;
    }

    // accept connection request
    cout << "accepting connection..." << endl;
    struct sockaddr_in6 tmpAddr = {0};
    socklen_t socklen = sizeof(tmpAddr);
    gAcceptedSock = accept(sock, (struct sockaddr *)&tmpAddr, &socklen);
    if (gAcceptedSock < 0) {
        cout << "accept err:" << strerror(errno) << endl;
        return EXIT_FAILURE;
    }

    cout << "accepted client sock:" << gAcceptedSock << endl;
    return EXIT_SUCCESS;
}

static int createTcpSocket() {
    lock_guard<mutex> lock(gOperationMutex);

    cout << "creating TCP socket" << endl;
    SocketInfo tcpInfo;
    tcpInfo.serviceId = gServiceId;
    tcpInfo.localPort = gSrcPort;
    EventFlowInfo eventInfo;
    resetCallbackPromise();
    if (Status::SUCCESS != gCv2xRadio->createCv2xTcpSocket(eventInfo, tcpInfo,
                                                           createTcpSocketCallback) ||
        ErrorCode::SUCCESS != gCallbackPromise.get_future().get()) {
        cout << "Tcp Socket creation failed." << endl;
        return EXIT_FAILURE;
    }

    //get created TCP socket
    gTcpSocket = gTcpSockInfo->getSocket();

    // add 500ms Tx/Rx timeout to remove the possibility for indefinite wait
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 500000;
    if (setsockopt(gTcpSocket, SOL_SOCKET, SO_RCVTIMEO|SO_SNDTIMEO, &tv, sizeof(tv)) < 0) {
        cout << "set sock timeout err:" << strerror(errno) << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

static int setupTcpConnection() {
    // create TCP socket
    if (createTcpSocket()) {
        return EXIT_FAILURE;
    }

    if (gTcpMode == TCP_CLIENT) {
        // For TCP client, connect to the configured dst addr
        if (connectTcpSocketClient(gTcpSocket)) {
            return EXIT_FAILURE;
        }
    } else {
        // For TCP server, accept incoming connection request
        if (acceptTcpSocketServer(gTcpSocket)) {
            return EXIT_FAILURE;
        }
    }

    gTcpConnected = true;

    return EXIT_SUCCESS;
}

// close accepted socket in server mode
static void closeAcceptedSocket() {
    if (gAcceptedSock < 0) {
        return;
    }
    cout << "closing client socket:"<< gAcceptedSock << endl;
    //call shutdown to send out FIN
    shutdown(gAcceptedSock, SHUT_WR|SHUT_RD);
    close(gAcceptedSock);
    gAcceptedSock = -1;
    usleep(500000);
}

static void closeTcpSocket() {
    if (!gTcpSockInfo) {
        return;
    }

    cout << "closing Tcp socket, fd:" << gTcpSockInfo->getSocket() << endl;
    resetCallbackPromise();
    if(Status::SUCCESS != gCv2xRadio->closeCv2xTcpSocket(gTcpSockInfo, closeTcpSocketCallback) ||
       ErrorCode::SUCCESS != gCallbackPromise.get_future().get()) {
        cout << "close Tcp socket err" << endl;
    }
    gTcpSocket = -1;
    gTcpSockInfo = nullptr;
}

static void releaseTcpConnection() {
    gTcpConnected = false;

    // For TCP server, close the accepted socket before closing the listening socket
    closeAcceptedSocket();

    // close TCP socket and deregister flows
    closeTcpSocket();
}

static void terminationCleanup() {
    lock_guard<mutex> lock(gOperationMutex);

    cout << "Terminating" << endl;

    // Release resources of TCP connection
    releaseTcpConnection();

    if (gCv2xRadio and gRadioListener) {
        gCv2xRadio->deregisterListener(gRadioListener);
    }

    if (gCv2xRadioMgr and gStatusListener) {
        gCv2xRadioMgr->deregisterListener(gStatusListener);
    }

    cout << "TCP Tx count:" << gTxCount << endl;
    cout << "TCP Rx count:" << gRxCount << endl;

    exit(0);
}

static void terminationHandler(int signum) {
    gTerminate = 1;
    write(gTerminatePipe[1], &gTerminate, sizeof(int));

    // notify threads waiting for active status
    gStatusCv.notify_all();
}

static void installSignalHandler() {
    struct sigaction sig_action;

    sig_action.sa_handler = terminationHandler;
    sigemptyset(&sig_action.sa_mask);
    sig_action.sa_flags = 0;

    sigaction(SIGINT, &sig_action, NULL);
    sigaction(SIGHUP, &sig_action, NULL);
    sigaction(SIGTERM, &sig_action, NULL);
}

static int startTcpClientMode() {
    // send out pkt reaches configured number
    if (gPacketNum > 0 and gTxCount >= gPacketNum) {
        return EXIT_FAILURE;
    }

    if (gTcpSocket < 0) {
        return EXIT_FAILURE;
    }

    // used the created socket to send pkt
    fillBuffer();
    if (sampleTx(gTcpSocket) <= 0) {
        return EXIT_FAILURE;
    }

    // wait for echo
    if (sampleRx(gTcpSocket) <= 0) {
        // EAGAIN and EWOULDBLOCK are possible error when
        // socket read timeout, not bail out in this case
        if (errno != EAGAIN and errno != EWOULDBLOCK) {
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}

static int startTcpServerMode() {
    if (gAcceptedSock < 0) {
        return EXIT_FAILURE;
    }

    // use the accepted socket to recv pkt
    if ((gPacketLen = sampleRx(gAcceptedSock)) <= 0) {
        // EAGAIN and EWOULDBLOCK are possible error when
        // socket read timeout, not bail out in this case
        if (errno != EAGAIN and errno != EWOULDBLOCK) {
            return EXIT_FAILURE;
        }
    } else {
        // send echo
        if (sampleTx(gAcceptedSock) <= 0) {
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
    cout << "Running C-V2X TCP Test" << endl;

    if (pipe(gTerminatePipe) == -1) {
        cout << "Pipe error" << endl;
        return EXIT_FAILURE;
    }

    installSignalHandler();

    auto f = std::async(std::launch::async, []() {
        int terminate = 0;
        read(gTerminatePipe[0], &terminate, sizeof(int));
        cout << "Read terminate:" << terminate << endl;
        terminationCleanup();
    });

    // Parse parameters, get cv2x handles, create TCP flow and establish the connection
    if (parseOpts(argc, argv) or
        init() or
        setupTcpConnection()) {
        goto bail;
    }

    // main operation loop
    while (!gTerminate) {
        // wait for V2X active status before Tx/Rx
        waitV2xStatusActive();
        if (!isV2xReady()) {
            continue;
        }

        if (gTcpMode == TCP_CLIENT) {
            // send msg to server and wait for echo
            if (startTcpClientMode()) {
                goto bail;
            } else {
                // wait 100ms to send the next pkt
                usleep(100000u);
            }
        } else {
            // echo each msg received from client
            if (startTcpServerMode()) {
                goto bail;
            }
        }
    }

bail:
    // teminate
    gTerminate = 1;
    write(gTerminatePipe[1], &gTerminate, sizeof(int));
    f.get();
    cout << "Done." << endl;

    return EXIT_SUCCESS;
}
