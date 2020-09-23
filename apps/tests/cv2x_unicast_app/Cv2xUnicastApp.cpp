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
 * @file: Cv2xUnicastApp.cpp
 *
 * @brief: Simple application that demonstrates Tx/Rx Unicast in Cv2x
 */

#include <arpa/inet.h>
#include <net/if.h>
#include <signal.h>
#include <errno.h>
#include <assert.h>
#include <ifaddrs.h>
#include <cstring>
#include <sys/time.h>
#include <unistd.h>
#include <glib.h>
#include <iostream>
#include <memory>
#include <atomic>
#include <telux/cv2x/Cv2xRadio.hpp>

using std::cerr;
using std::cout;
using std::endl;
using std::future;
using std::promise;
using std::shared_ptr;
using telux::common::ErrorCode;
using telux::common::Status;
using telux::cv2x::Cv2xFactory;
using telux::cv2x::Cv2xStatus;
using telux::cv2x::Cv2xStatusType;
using telux::cv2x::ICv2xRadio;
using telux::cv2x::Periodicity;
using telux::cv2x::Priority;
using telux::cv2x::TrafficCategory;
using telux::cv2x::SpsFlowInfo;
using telux::cv2x::EventFlowInfo;
using telux::cv2x::ICv2xTxFlow;
using telux::cv2x::ICv2xRxSubscription;
using telux::cv2x::TrafficIpType;
using telux::cv2x::ICv2xRadioListener;

static constexpr uint32_t BROADCAST_SERVICE_ID = 1u;
static constexpr uint32_t UNICAST_SERVICE_ID = 10u;  // differ from broadcast SID
static constexpr uint16_t DEFAULT_BROADCAST_PORT = 5000u;
static constexpr uint16_t DEFUALT_UNICAST_PORT = 6000u; // differ from broadcast port
static constexpr uint32_t BROADCAST_PACKET_LEN = 128u;
static constexpr uint32_t UNICAST_PACKET_LEN = 256u;
static constexpr uint32_t DUMMY_PACKET_LEN = UNICAST_PACKET_LEN; //cv2x msg buffer length
static constexpr char TEST_VERNO_MAGIC = 'U'; // magic word for WSA and unicast msgs
static constexpr int PRIORITY = 3; // WSA priority
static constexpr char RSU_ID = 1;
static constexpr char OBU_ID = 2;

static Cv2xStatus gCv2xStatus;
static promise<ErrorCode> gCallbackPromise;
static volatile std::atomic<uint8_t> gTerminate = { 0 };
static uint32_t gTxUnicastCount = 0;
static uint32_t gRxUnicastCount = 0;
static struct sockaddr_in6 gRsuAddr = { 0 };
static bool gWsaReceived = false;

// Two operation modes are supported by this app.
// If in OBU mode, after receiving broadcast WSA msg from RSU, it starts transmitting
// unicast msgs and waits for the unicast echo msgs from RSU.
// If in RSU mode, it transmits broadcast WSA msgs periodically and sends unicast echo
// msg to OBU for each received unicast msg.
enum class OperationMode {
    OBU,
    RSU
};

static OperationMode gOperationMode;

class State {
public:
    shared_ptr<ICv2xTxFlow> txBroadcastFlow = nullptr;
    shared_ptr<ICv2xRxSubscription> rxBroadcastFlow = nullptr;
    uint32_t broadcastServiceId = BROADCAST_SERVICE_ID;
    uint16_t broadcastPort = DEFAULT_BROADCAST_PORT;
    shared_ptr<ICv2xTxFlow> txUnicastFlow = nullptr;
    shared_ptr<ICv2xRxSubscription> rxUnicastFlow = nullptr;
    bool isWsaThreadValid = false;
    future<void> wsaThread;
    shared_ptr<ICv2xRadioListener> radioListener = nullptr;
};

class RadioListener : public ICv2xRadioListener {
public:
    void onL2AddrChanged(uint32_t newL2Address) {
        cout << "Src L2 Addr updated to:" << newL2Address << endl;
    }
};

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

// Returns current timestamp
static uint64_t getCurrentTimestamp(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000ull + tv.tv_usec;
}

struct cv2x_common_message_t {
    char v2x_family_id;
    char ueId;
    bool has_seq_num;
    uint16_t seq_num;
    bool has_timestamp;
    uint64_t timestamp;
};

struct cv2x_message_t {
    union {
        struct cv2x_common_message_t contents;
        char buffer[DUMMY_PACKET_LEN];
    };
    uint32_t length;
};

// Fills buffer
void createBuffer(cv2x_message_t &cv2xMsg) {

    static uint16_t seq_num = 0u;
    auto timestamp = getCurrentTimestamp();

    cv2xMsg = { 0 };

    // Very first payload is test magic number
    cv2xMsg.contents.v2x_family_id = TEST_VERNO_MAGIC;

    // Next byte is the UEID value
    if (gOperationMode == OperationMode::OBU) {
        cv2xMsg.contents.ueId = OBU_ID;
    } else {
        cv2xMsg.contents.ueId = RSU_ID;
    }

    // Sequence number
    cv2xMsg.contents.seq_num = htons(seq_num++);
    cv2xMsg.contents.has_seq_num = true;

    // Timestamp
    cv2xMsg.contents.timestamp = htobe64(timestamp);
    cv2xMsg.contents.has_timestamp = true;

    if (gOperationMode == OperationMode::OBU) {
         //unicast msg length
        cv2xMsg.length = UNICAST_PACKET_LEN;
    } else {
        //WSA msg length
        cv2xMsg.length = BROADCAST_PACKET_LEN;
    }

    // Dummy payload
    constexpr int NUM_LETTERS = 26;
    auto i = sizeof(struct cv2x_common_message_t);
    for (; i < cv2xMsg.length; ++i) {
        cv2xMsg.buffer[i] = 'a' + (i % NUM_LETTERS);
    }
}

// Function for transmitting data
static int sampleTx(int sock,
                    cv2x_message_t &cv2xMsg,
                    bool isUnicast,
                    struct sockaddr_in6 dstAddr) {
    static uint32_t txCount = 0u;

    cout << "sampleTx(" << sock << ")" << endl;

    struct sockaddr_in6 dest_sockaddr = { 0 };
    if (isUnicast) {
        dest_sockaddr = dstAddr;
    }

    struct msghdr message = { 0 };
    struct iovec iov[1] = { 0 };
    struct cmsghdr *cmsghp = NULL;
    char control[CMSG_SPACE(sizeof(int))];

    // Send data using sendmsg to provide IPV6_TCLASS per packet
    iov[0].iov_base = cv2xMsg.buffer;
    iov[0].iov_len = cv2xMsg.length;

    message.msg_name = &dest_sockaddr;
    message.msg_namelen = sizeof(dest_sockaddr);
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
        return -1;
    }

    ++txCount;
    cout << "TX count: " << txCount << " bytes:" << sendBytes << endl;
    return 0;
}

// Function for reading from Rx socket
static int sampleRx(int sock, cv2x_message_t &cv2xMsg, struct sockaddr_in6 &srcAddr) {
    static uint32_t rxCount = 0u;
    char buffer[DUMMY_PACKET_LEN];
    struct sockaddr_in6 from;
    socklen_t fromLen = sizeof(from);

    // Attempt to read from socket
    int recvBytes = recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&from, &fromLen);

    if (recvBytes <= 0) {
        // EAGAIN and EWOULDBLOCK are possible return values when
        // our Rx socket has timed out. This can be an indication that
        // cv2x status has changed and we need to recheck v2x status before
        // continuing to wait for a message over the socket.
        if (errno != EAGAIN || errno != EWOULDBLOCK) {
            cerr << "Error occurred reading from sock:" << sock;
            cerr << " err:" << strerror(errno) << endl;
            return -1;
        }

        return 0;
    }

    cout << "sampleRx(" << sock << ")" << " bytes:" << recvBytes << endl;

    memcpy(cv2xMsg.buffer, buffer, recvBytes);
    cv2xMsg.length = recvBytes;

    // check the magic number for WSA and unicast msgs
    if (cv2xMsg.contents.v2x_family_id != TEST_VERNO_MAGIC) {
        cout << "Ignore msg with mismatched magic character."<< endl;
        return -1;
    }

    srcAddr = from;

    ++rxCount;
    cout << "RX count: " << rxCount << " bytes:" << recvBytes << endl;
    return recvBytes;
}

static void printUsage(const char *Opt) {
    cout << "Usage: " << Opt << "\n"
         << "-m<Mode>             0--OBU 1--RSU, default to OBU\n"
         << "-s<Broadcast Service Id>  Broadcast service Id, default to 1\n"
         << "-p<Broadcast Port>  Broadcast port used, default to 5000" << endl;
}

// Parse options
static int parseOpts(int argc, char *argv[], State &state) {
    int rc = 0;
    int c;
    while ((c = getopt(argc, argv, "?:m::p::s::")) != -1) {
        switch (c) {
        case 'm':
            if (optarg) {
                gOperationMode = static_cast<OperationMode>(atoi(optarg));
                cout << "mode: " << static_cast<int>(gOperationMode) << endl;
            }
            break;
        case 'p':
            if (optarg) {
                state.broadcastPort = atoi(optarg);
                cout << "broadcast port: " << state.broadcastPort << endl;
            }
            break;
        case 's':
            if (optarg) {
                state.broadcastServiceId = atoi(optarg);
                cout << "broadcast service id: " << state.broadcastServiceId << endl;
            }
            break;
        case '?':
        default:
            rc = -1;
            printUsage(argv[0]);
            return rc;
        }
    }

    return rc;
}

static void terminationHandler(int signum) {
    gTerminate = 1;
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

Status registerBroadcastFlows(shared_ptr<ICv2xRadio> cv2xRadio, State &state) {
    Status status = Status::SUCCESS;

    if (gOperationMode == OperationMode::OBU) {
        // If in OBU mode, register tx/rx flow to listen to WSA
        cout << "Registering broadcast Tx event Flow" << endl;
        auto createTxEventFlowCallback = [&state](shared_ptr<ICv2xTxFlow> txEventFlow,
                                                  ErrorCode error) {
            if (ErrorCode::SUCCESS == error) {
                state.txBroadcastFlow = txEventFlow;
            }
            gCallbackPromise.set_value(error);
        };

        // Reset global callback
        resetCallbackPromise();

        // Register non-Ip Tx event flow with broadcast SID and broadcast port
        EventFlowInfo flowInfo;
        status = cv2xRadio->createTxEventFlow(TrafficIpType::TRAFFIC_NON_IP,
                                              state.broadcastServiceId,
                                              flowInfo,
                                              state.broadcastPort,
                                              createTxEventFlowCallback);
        if (Status::SUCCESS != status or
            ErrorCode::SUCCESS != gCallbackPromise.get_future().get()) {
            cerr << "Failed to create broadcast Tx event flow!" << endl;
            return Status::FAILED;
        }

        cout << "Succeeded in creating broadcast Tx event Flow, sock:";
        cout << state.txBroadcastFlow->getSock();
        cout << " , port:"<< state.broadcastPort << endl;

        cout << "Registering broadcast Rx flow" << endl;

        // Callback function for ICv2xRadio->createRxSubscription()
        auto createRxSubscriptionCallback = [&state](shared_ptr<ICv2xRxSubscription> rxSub,
                                                     ErrorCode error) {
            if (ErrorCode::SUCCESS == error) {
                state.rxBroadcastFlow = rxSub;
            }
            gCallbackPromise.set_value(error);
        };

        // Reset global callback
        resetCallbackPromise();

        // Subscribe to broadcast SID and broadcast port
        auto idList = std::make_shared<std::vector<uint32_t>>(1, state.broadcastServiceId);
        status = cv2xRadio->createRxSubscription(TrafficIpType::TRAFFIC_NON_IP,
                                                 state.broadcastPort,
                                                 createRxSubscriptionCallback,
                                                 idList);
        if (Status::SUCCESS != status or
            ErrorCode::SUCCESS != gCallbackPromise.get_future().get()) {
            cerr << "Failed to create broadcast Rx flow." << endl;
            return Status::FAILED;
        }

        // Adding 100ms timeout to avoid indefinite read wait
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 100000;

        if (setsockopt(state.rxBroadcastFlow->getSock(),
                       SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
            cerr << "Failed to set Rx socket timeout" << endl;
            return Status::FAILED;
        }

        cout << "Succeeded in creating broadcast Rx Flow, sock:";
        cout << state.rxBroadcastFlow->getSock();
        cout << " , port:"<< state.broadcastPort << endl;

    } else {
        // If in RSU mode, register tx sps flow for the sending of WSA
        cout << "Registering broadcast Tx SPS Flow" << endl;

        auto createTxSpsFlowCallback = [&state](shared_ptr<ICv2xTxFlow> txSpsFlow,
                                                shared_ptr<ICv2xTxFlow> txEventFlow,
                                                ErrorCode spsError,
                                                ErrorCode unused) {
            if (ErrorCode::SUCCESS == spsError) {
                state.txBroadcastFlow = txSpsFlow;
            }
            gCallbackPromise.set_value(spsError);
        };

        // Reset global callback
        resetCallbackPromise();

        // Register non-Ip Tx sps flow with broadcast SID and broadcast port
        SpsFlowInfo spsInfo;
        spsInfo.priority = Priority::PRIORITY_2;
        spsInfo.periodicity = Periodicity::PERIODICITY_100MS;
        spsInfo.nbytesReserved = BROADCAST_PACKET_LEN;
        spsInfo.autoRetransEnabledValid = true;
        spsInfo.autoRetransEnabled = true;
        status = cv2xRadio->createTxSpsFlow(TrafficIpType::TRAFFIC_NON_IP,
                                            state.broadcastServiceId,
                                            spsInfo, state.broadcastPort,
                                            true, state.broadcastPort+1,
                                            createTxSpsFlowCallback);
        if (Status::SUCCESS != status or
            ErrorCode::SUCCESS != gCallbackPromise.get_future().get()) {
            cerr << "Failed to create broadcast Tx SPS Flow." << endl;
            return Status::FAILED;
        }

        cout << "Succeeded in creating broadcast Tx SPS Flow, sock:";
        cout << state.txBroadcastFlow->getSock();
        cout << " , port:"<< state.broadcastPort << endl;
    }

    return Status::SUCCESS;
}

Status registerUnicastFlows(shared_ptr<ICv2xRadio> cv2xRadio, State &state) {
    Status status = Status::SUCCESS;

    cout << "Registering Unicast Tx Flow" << endl;

    auto createTxEventFlowCallback = [&state](shared_ptr<ICv2xTxFlow> txEventFlow,
                                              ErrorCode error) {
        if (ErrorCode::SUCCESS == error) {
            state.txUnicastFlow = txEventFlow;
        }
        gCallbackPromise.set_value(error);
    };

    // Reset global callback
    resetCallbackPromise();

    // Register unicast non-Ip Tx flow with unicast port, SID is ignored for unicast flows
    EventFlowInfo flowInfo;
    flowInfo.isUnicast = true;
    status = cv2xRadio->createTxEventFlow(TrafficIpType::TRAFFIC_NON_IP,
                                          UNICAST_SERVICE_ID,
                                          flowInfo,
                                          DEFUALT_UNICAST_PORT,
                                          createTxEventFlowCallback);
    if (Status::SUCCESS != status or
        ErrorCode::SUCCESS != gCallbackPromise.get_future().get()) {
        cerr << "Failed to create unicast Tx Flow." << endl;
        return Status::FAILED;
    }

    cout << "Succeeded in creating unicast Tx event Flow, sock:";
    cout << state.txUnicastFlow->getSock();
    cout << " , port:"<< DEFUALT_UNICAST_PORT << endl;

    cout << "Registering unicast Rx Flow" << endl;

    auto createRxSubCallback = [&state](shared_ptr<ICv2xRxSubscription> rxSub,
                                        ErrorCode error) {
        if (ErrorCode::SUCCESS == error) {
            state.rxUnicastFlow = rxSub;
        }
        gCallbackPromise.set_value(error);
    };

    // Reset global callback
    resetCallbackPromise();

    // Subscribe to unicast port
    // Unicast SID is specified to avoid creating wildcard for broadcast Rx
    auto idList = std::make_shared<std::vector<uint32_t>>(1, UNICAST_SERVICE_ID);
    status = cv2xRadio->createRxSubscription(TrafficIpType::TRAFFIC_NON_IP,
                                             DEFUALT_UNICAST_PORT,
                                             createRxSubCallback,
                                             idList);
    if (Status::SUCCESS != status or
        ErrorCode::SUCCESS != gCallbackPromise.get_future().get()) {
        cerr << "Failed to create unicast Rx Flow!" << endl;
        return Status::FAILED;
    }

    // Adding 100ms timeout to avoid indefinite read wait
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 100000;
    if (setsockopt(state.txUnicastFlow->getSock(),
                   SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        cerr << "Failed to set Tx socket timeout!" << endl;
        return Status::FAILED;
    }

    if (setsockopt(state.rxUnicastFlow->getSock(),
                   SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        cerr << "Failed to set Rx socket timeout!" << endl;
        return Status::FAILED;
    }

    cout << "Succeeded in creating unicast Rx Flow, sock:";
    cout << state.rxUnicastFlow->getSock();
    cout << " , port:"<< DEFUALT_UNICAST_PORT << endl;

    return Status::SUCCESS;
}

void terminationCleanup(shared_ptr<ICv2xRadio> cv2xRadio, State &state) {
    cout << "Terminating, deregistering all flows" << endl;

    auto closeTxFlowCallback = [](shared_ptr<ICv2xTxFlow> txFlow, ErrorCode error) {
        gCallbackPromise.set_value(error);
    };

    auto closeRxSubCallback = [](shared_ptr<ICv2xRxSubscription> rxSub, ErrorCode error) {
        gCallbackPromise.set_value(error);
    };

    if (state.txBroadcastFlow) {
        cout << "closing broadcast tx flow, sock:" << state.txBroadcastFlow->getSock() << endl;

        // Reset global callback
        resetCallbackPromise();

        auto status = cv2xRadio->closeTxFlow(state.txBroadcastFlow, closeTxFlowCallback);
        if (Status::SUCCESS != status or
            ErrorCode::SUCCESS != gCallbackPromise.get_future().get()) {
            cerr << "Failed to close broadcast tx flow!" << endl;
        }
    }

    if (state.rxBroadcastFlow) {
        cout << "closing broadcast rx flow, sock:" << state.rxBroadcastFlow->getSock() << endl;

        // Reset global callback
        resetCallbackPromise();

        auto status = cv2xRadio->closeRxSubscription(state.rxBroadcastFlow, closeRxSubCallback);
        if (Status::SUCCESS != status or
            ErrorCode::SUCCESS != gCallbackPromise.get_future().get()) {
            cerr << "Failed to close broadcast rx flow!" << endl;
        }
    }

    if (state.txUnicastFlow) {
        cout << "closing unicast tx flow, sock:" << state.txUnicastFlow->getSock() << endl;

        // Reset global callback
        resetCallbackPromise();

        auto status = cv2xRadio->closeTxFlow(state.txUnicastFlow, closeTxFlowCallback);
        if (Status::SUCCESS != status or
            ErrorCode::SUCCESS != gCallbackPromise.get_future().get()) {
            cerr << "Failed to close unicast tx flow!" << endl;
        }
    }

    if (state.rxUnicastFlow) {
        cout << "closing unicast rx flow, sock:" << state.rxUnicastFlow->getSock() << endl;

        // Reset global callback
        resetCallbackPromise();

        auto status = cv2xRadio->closeRxSubscription(state.rxUnicastFlow, closeRxSubCallback);
        if (Status::SUCCESS != status or
            ErrorCode::SUCCESS != gCallbackPromise.get_future().get()) {
            cerr << "Failed to close unicast rx flow!" << endl;
        }
    }

    // Deregister listener
    if (state.radioListener) {
        cv2xRadio->deregisterListener(state.radioListener);
    }

    // Wait for wave advertisment thread to end
    if (state.isWsaThreadValid) {
        state.wsaThread.get();
    }
}

int main(int argc, char *argv[]) {
    cout << "Running Sample C-V2X Unicast app" << endl;

    installSignalHandler();

    State state;

    // Parse parameters
    if (parseOpts(argc, argv, state) < 0) {
        return EXIT_FAILURE;
    }

    // Get handle to Cv2xRadioManager
    auto &cv2xFactory = Cv2xFactory::getInstance();
    auto cv2xRadioManager = cv2xFactory.getCv2xRadioManager();

    // Wait for radio manager to complete initialization
    if (not cv2xRadioManager->isReady()) {
        if (cv2xRadioManager->onReady().get()) {
            cout << "C-V2X Radio Manager is ready" << endl;
        } else {
            cerr << "C-V2X Radio Manager initialization failed, exiting" << endl;
            return EXIT_FAILURE;
        }
    }

    // Get C-V2X status and make sure Tx/Rx enabled
    assert(Status::SUCCESS == cv2xRadioManager->requestCv2xStatus(cv2xStatusCallback));
    assert(ErrorCode::SUCCESS == gCallbackPromise.get_future().get());

    if (Cv2xStatusType::ACTIVE == gCv2xStatus.txStatus &&
        Cv2xStatusType::ACTIVE == gCv2xStatus.rxStatus) {
        cout << "C-V2X TX/RX status is active" << endl;
    } else {
        cerr << "C-V2X TX/RX is not active exiting" << endl;
        return EXIT_FAILURE;
    }

    // Get handle to Cv2xRadio
    shared_ptr<ICv2xRadio> cv2xRadio;
    cv2xRadio = cv2xRadioManager->getCv2xRadio(TrafficCategory::SAFETY_TYPE);

    // Wait for radio to complete initialization
    if (not cv2xRadio->isReady()) {
        if (Status::SUCCESS == cv2xRadio->onReady().get()) {
            cout << "C-V2X Radio is ready" << endl;
        } else {
            cerr << "C-V2X Radio initialization failed." << endl;
            return EXIT_FAILURE;
        }
    }

    // Register for Src L2 Id Update callbacks
    state.radioListener = std::make_shared<RadioListener>();
    if (Status::SUCCESS != cv2xRadio->registerListener(state.radioListener)) {
        cerr << "Listener registration failed." << endl;
        return EXIT_FAILURE;
    }

    // Register broadcast flows
    if (Status::SUCCESS != registerBroadcastFlows(cv2xRadio, state)) {
        cerr << "Broadcast flows creation failed." << endl;
        goto bail;
    }

    // Register unicast flows
    if (Status::SUCCESS != registerUnicastFlows(cv2xRadio, state)) {
        cerr << "Unicast flows creation failed." << endl;
        goto bail;
    }

    if (OperationMode::RSU == gOperationMode) {
        // If in RSU mode, create thread for broadcasting WSA with interval 100ms
        cout << "Start sending WSA." << endl;
        int sock = state.txBroadcastFlow->getSock();
        state.wsaThread = std::async(std::launch::async, [sock]() {
            cv2x_message_t cv2xMsg;
            struct sockaddr_in6 dstAddr = {0};
            while (!gTerminate) {
                createBuffer(cv2xMsg);
                sampleTx(sock, cv2xMsg, false, dstAddr);
                usleep(100000u);
            }
        });
        state.isWsaThreadValid = true;
    }

    // Main operation loop
    while (!gTerminate) {

        cv2x_message_t cv2xMsg = { 0 };

        if (OperationMode::OBU == gOperationMode) {

            // wait for WSA to get the original RSU's address
            if (not gWsaReceived) {
                cout << "Waiting for WSA from RSU." << endl;
                if (sampleRx(state.rxBroadcastFlow->getSock(), cv2xMsg, gRsuAddr) <= 0) {
                    continue;
                }

                cout << "Received WSA from RSU L2 ID: ";
                cout << ntohl(gRsuAddr.sin6_addr.s6_addr32[3]) << endl;

                // Modify destination port of unicast msgs to unicast port
                gRsuAddr.sin6_port = htons((uint16_t)DEFUALT_UNICAST_PORT);
                gWsaReceived = true;
            }

            // send unicast msg and wait for the echo from RSU with interval 100ms
            cout << "Sending unicast msg to RSU L2 ID:";
            cout << ntohl(gRsuAddr.sin6_addr.s6_addr32[3]) << endl;

            createBuffer(cv2xMsg);
            if (sampleTx(state.txUnicastFlow->getSock(), cv2xMsg, true, gRsuAddr) < 0) {
                cerr << "Failed to send unicast msg!" << endl;
                continue;
            }
            gTxUnicastCount++;

            cout << "Waiting for unicast echo msg from RSU." << endl;
            cv2x_message_t echoCv2xMsg = { 0 };
            struct sockaddr_in6 tmpRsuAddr = { 0 };
            if (sampleRx(state.rxUnicastFlow->getSock(), echoCv2xMsg, tmpRsuAddr) > 0) {
                if (cv2xMsg.length == echoCv2xMsg.length and
                    0 == memcmp(cv2xMsg.buffer, echoCv2xMsg.buffer, cv2xMsg.length)) {
                    gRxUnicastCount++;
                    cout << "Received echo msg from RSU L2 ID:";
                    cout << ntohl(tmpRsuAddr.sin6_addr.s6_addr32[3]) << endl;

                    // update RSU's address according to echo msg.
                    // echo msg always has the latest RSU's address
                    if (memcmp(&tmpRsuAddr, &gRsuAddr, sizeof(struct sockaddr_in6))) {
                        cout << "RSU L2 address updated." << endl;
                        gRsuAddr = tmpRsuAddr;
                    }
                } else {
                    cerr << "Received mismatched echo msg from RSU L2 ID: ";
                    cerr << ntohl(tmpRsuAddr.sin6_addr.s6_addr32[3]) << endl;
                }
            } else {
                cerr << "Failed to receive echo msg." << endl;
                goto bail;
            }

            usleep(100000u);
        } else {

            cout << "Waiting for unicast msg from OBU." << endl;

            // On error or timeout of socket resume waiting for unicast Tx
            struct sockaddr_in6 obuAddr = { 0 };
            if (sampleRx(state.rxUnicastFlow->getSock(), cv2xMsg, obuAddr) <= 0) {
                continue;
            }
            gRxUnicastCount++;
            cout << "Received unicast msg from OBU L2 ID: ";
            cout << ntohl(obuAddr.sin6_addr.s6_addr32[3]) << endl;

            cout << "Sending unicast echo msg to OBU." << endl;
            if (sampleTx(state.txUnicastFlow->getSock(), cv2xMsg, true, obuAddr) < 0) {
                cerr << "Failed to echo OBU!" << endl;
            } else {
                gTxUnicastCount++;
            }
        }
    }

bail:
    terminationCleanup(cv2xRadio, state);

    cout << "Unicast Tx count:" << gTxUnicastCount << endl;
    cout << "Unicast Rx count:" << gRxUnicastCount << endl;
    cout << "Done." << endl;

    return EXIT_SUCCESS;
}
