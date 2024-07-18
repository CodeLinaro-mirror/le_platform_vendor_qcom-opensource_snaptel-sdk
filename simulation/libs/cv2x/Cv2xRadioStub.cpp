/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <linux/sockios.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <sys/ioctl.h>

#include "Cv2xRadioStub.hpp"
#include "Cv2xRxSubscriptionStub.hpp"
#include "Cv2xTxFlowStub.hpp"
#include "Cv2xTxRxSocketStub.hpp"

#define RPC_FAIL_SUFFIX " RPC Request failed - "

#define FLOW_RPC_CALL(func, ipType, spsPort, evtPort, flowId, serviceId, status, ec, delay) \
    {                                                                                       \
        ::cv2xStub::FlowInfo request;                                                       \
        ::cv2xStub::Cv2xRadioFlowReply response;                                            \
        request.set_iptype(static_cast<uint32_t>(ipType));                                  \
        request.set_spsport(static_cast<uint32_t>(spsPort));                                \
        request.set_eventport(static_cast<uint32_t>(evtPort));                              \
        request.set_flowid(static_cast<int32_t>(flowId));                                   \
        request.set_serviceid(static_cast<uint32_t>(serviceId));                            \
        ClientContext context;                                                              \
        ::grpc::Status reqstatus = func(&context, request, &response);                      \
        if (reqstatus.ok()) {                                                               \
            status = static_cast<telux::common::Status>(response.status());                 \
            ec     = static_cast<telux::common::ErrorCode>(response.error());               \
            delay  = static_cast<int>(response.delay());                                    \
            flowId = static_cast<uint8_t>(response.flowid());                               \
        } else {                                                                            \
            status = telux::common::Status::FAILED;                                         \
            ec     = telux::common::ErrorCode::NOT_PROVISIONED;                             \
        }                                                                                   \
    }

#define RXSUBSCRIPTION_RPC_CALL(func, ipType, port, idList, status, ec, delay)      \
    {                                                                               \
        ::cv2xStub::RxSubscription request;                                         \
        ::cv2xStub::Cv2xCommandReply response;                                      \
        request.set_portnum(static_cast<uint32_t>(port));                           \
        request.set_iptype(static_cast<uint32_t>(ipType));                          \
        if (idList) {                                                               \
            for (uint32_t id : *idList) {                                           \
                request.add_ids(id);                                                \
            }                                                                       \
        }                                                                           \
                                                                                    \
        CALL_RPC(func, request, status, response, delay);                           \
        if (status != telux::common::Status::SUCCESS) {                             \
            LOG(ERROR, "RXSUBSCRIPTION_RPC_CALL failed", static_cast<int>(status)); \
            ec = telux::common::ErrorCode::MODEM_ERR;                               \
        } else {                                                                    \
            ec = telux::common::ErrorCode::SUCCESS;                                 \
        }                                                                           \
    }

namespace telux {
namespace cv2x {

// Max number of SPS flows supported
static constexpr uint32_t SIMULATION_SPS_MAX_NUM_FLOWS = 2u;
// Max number of Non-SPS flows supported
static constexpr uint32_t SIMULATION_NON_SPS_MAX_NUM_FLOWS = 255u;

// Capabilities are hardcoded for now
// Payload max size, plus IPV6 header is 1500 bytes
static constexpr uint32_t SIMULATION_LINK_IP_MTU_BYTES     = 1452u;
static constexpr uint32_t SIMULATION_LINK_NON_IP_MTU_BYTES = 2000u;
static constexpr RadioConcurrencyMode SIMULATION_SUPPORTED_CONCURRENCY_MODE
    = RadioConcurrencyMode::WWAN_CONCURRENT;

static constexpr uint16_t SIMULATION_TX_PAYLOAD_OFFSET_BYTES = 0u;
static constexpr uint16_t SIMULATION_RX_PAYLOAD_OFFSET_BYTES = 0u;

static constexpr uint8_t SIMULATION_MAX_NUM_AUTO_RETRANSMISSIONS = 1u;
static constexpr uint8_t SIMULATION_LAYER_2_MAC_ADDRESS_SIZE     = 3u;

static constexpr int SIMULATION_CV2X_MAX_TX_POWER = 33;
static constexpr int SIMULATION_CV2X_MIN_TX_POWER = -30;

static constexpr uint16_t SIMULATION_CV2X_MIN_FREQ = 54800;
static constexpr uint16_t SIMULATION_CV2X_MAX_FREQ = 54980;

static constexpr unsigned int SIMULATION_MINIMUM_PORT_NUMBER = 1024;

static std::string DEFAULT_DEST_IP_ADDR = "ff02::1";
static std::string LO_IPV6_ADDR         = "::1";

Cv2xRadioSimulation::Cv2xRadioSimulation() {
    LOG(DEBUG, __FUNCTION__);
    pEvtListener_ = std::make_shared<Cv2xEvtListener>();
    taskQ_        = std::make_shared<AsyncTaskQueue<void>>();
    serviceStub_  = CommonUtils::getGrpcStub<::cv2xStub::Cv2xRadioService>();

    dummyCap_.linkIpMtuBytes          = SIMULATION_LINK_IP_MTU_BYTES;
    dummyCap_.linkNonIpMtuBytes       = SIMULATION_LINK_NON_IP_MTU_BYTES;
    dummyCap_.maxSupportedConcurrency = SIMULATION_SUPPORTED_CONCURRENCY_MODE;

    dummyCap_.nonIpTxPayloadOffsetBytes = SIMULATION_TX_PAYLOAD_OFFSET_BYTES;
    dummyCap_.nonIpRxPayloadOffsetBytes = SIMULATION_RX_PAYLOAD_OFFSET_BYTES;

    dummyCap_.periodicitiesSupported.set(static_cast<int>(Periodicity::PERIODICITY_100MS));
    dummyCap_.periodicities.push_back(static_cast<uint64_t>(100));

    dummyCap_.maxNumAutoRetransmissions = SIMULATION_MAX_NUM_AUTO_RETRANSMISSIONS;
    dummyCap_.layer2MacAddressSize      = SIMULATION_LAYER_2_MAC_ADDRESS_SIZE;

    dummyCap_.prioritiesSupported.set(static_cast<int>(Priority::MOST_URGENT));
    dummyCap_.prioritiesSupported.set(static_cast<int>(Priority::PRIORITY_2));

    dummyCap_.maxNumSpsFlows    = SIMULATION_SPS_MAX_NUM_FLOWS;
    dummyCap_.maxNumNonSpsFlows = SIMULATION_NON_SPS_MAX_NUM_FLOWS;

    dummyCap_.maxTxPower = SIMULATION_CV2X_MAX_TX_POWER;
    dummyCap_.minTxPower = SIMULATION_CV2X_MIN_TX_POWER;
    TxPoolIdInfo dummy{0, SIMULATION_CV2X_MIN_FREQ, SIMULATION_CV2X_MAX_FREQ};
    dummyCap_.txPoolIdsSupported.emplace_back(dummy);
    dummyCap_.isUnicastSupported = 1;
}

Cv2xRadioSimulation::~Cv2xRadioSimulation() {
    LOG(DEBUG, __FUNCTION__);
    setInitializedStatus(telux::common::Status::FAILED, nullptr);
    // close all TCP sockets and remove associated Tx/Rx flows
    closeAllCv2xTcpSockets();

    // Unsubscribe from everything.
    unsubscribeAllRxSubs();

    // Deregister Non-SPS flows that the user hasn't explicitly closed.
    cleanupAllEventFlows();

    // Deregister SPS flows that the user hasn't explicitly closed.
    cleanupAllSpsFlows();

    taskQ_ = nullptr;
}

bool Cv2xRadioSimulation::isInitialized() const {
    return telux::common::Status::NOTREADY != initializedStatus_;
}

bool Cv2xRadioSimulation::isReady() const {
    return (serviceStatus_ == telux::common::ServiceStatus::SERVICE_AVAILABLE) ? true : false;
}

std::future<telux::common::Status> Cv2xRadioSimulation::onReady() {
    auto f = std::async(std::launch::async, [this] { return this->waitForInitialization(); });
    return f;
}

telux::common::ServiceStatus Cv2xRadioSimulation::getServiceStatus() {
    lock_guard<std::mutex> lock(mutex_);
    return serviceStatus_;
}

common::Status Cv2xRadioSimulation::waitForInitialization() {
    unique_lock<std::mutex> cvLock(mutex_);
    while (initializedStatus_ == telux::common::Status::NOTREADY) {
        initializedCv_.wait(cvLock);
    }
    return initializedStatus_;
}

void Cv2xRadioSimulation::setInitializedStatus(
    telux::common::Status status, telux::common::InitResponseCb cb) {
    LOG(INFO, __FUNCTION__, " ", static_cast<int>(status));
    telux::common::ServiceStatus subSys;
    {
        lock_guard<std::mutex> cvLock(mutex_);
        initializedStatus_ = (status == common::Status::NOTREADY) ? common::Status::FAILED : status;
        subSys             = (status != telux::common::Status::SUCCESS)
                                 ? telux::common::ServiceStatus::SERVICE_FAILED
                                 : telux::common::ServiceStatus::SERVICE_AVAILABLE;
        serviceStatus_     = subSys;
        initializedCv_.notify_all();
    }

    if (cb) {
        cb(subSys);
    }
}

int Cv2xRadioSimulation::getV6AddrByIface(string &ifaceName, struct in6_addr &v6Addr) {
    int result = -1;
    struct ifaddrs *ifap;
    struct ifaddrs *ifa;
    struct sockaddr_in6 *sockAddr;

    LOG(DEBUG, __FUNCTION__, " Interface name = ", ifaceName);

    if (ifaceName.empty()) {
        return result;
    }

    getifaddrs(&ifap);
    ifa = ifap;
    while (ifa) {
        if (ifa && ifa->ifa_name && ifa->ifa_addr && 0 == ifaceName.compare(ifa->ifa_name)) {
            LOG(DEBUG, __FUNCTION__, " iface=", ifa->ifa_name,
                " family=", ifa->ifa_addr->sa_family);
        } else {
            if (!ifa->ifa_name) {
                LOG(ERROR, __FUNCTION__, " null ifa name ptr should not happen.");
                return result;
            }
        }

        if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET6) {
            sockAddr = (struct sockaddr_in6 *)ifa->ifa_addr;
            string ifaName(ifa->ifa_name);
            if (sockAddr && ifaName == ifaceName) {
                result = 0;
                memcpy(&v6Addr, &sockAddr->sin6_addr, sizeof(struct in6_addr));
                LOG(DEBUG, __FUNCTION__, " has address.");
                break;
            }
        }
        ifa = ifa->ifa_next;
    }
    freeifaddrs(ifap);
    return result;
}

void Cv2xRadioSimulation::init(telux::common::InitResponseCb callback) {
    telux::common::Status res = telux::common::Status::FAILED;
    LOG(DEBUG, __FUNCTION__);

    if (pEvtListener_) {
        res = pEvtListener_->registerListener(shared_from_this());
    }
    if (res != telux::common::Status::SUCCESS) {
        setInitializedStatus(res, callback);
        return;
    }

    auto f = std::async(std::launch::async, [this, callback]() {
        telux::common::Status status = telux::common::Status::FAILED;
        const ::google::protobuf::Empty request;
        ::cv2xStub::Cv2xRequestStatusReply response;
        int delay = DEFAULT_DELAY;

        CALL_RPC(serviceStub_->requestCv2xStatus, request, status, response, delay);
        telux::cv2x::Cv2xStatus cv2xStatus;
        if (status == telux::common::Status::SUCCESS) {
            RPC_TO_CV2X_STATUS(response.cv2xstatus(), cv2xStatus);
            if (not(cv2xStatus.rxStatus == Cv2xStatusType::SUSPENDED
                    || cv2xStatus.rxStatus == Cv2xStatusType::ACTIVE)
                && (cv2xStatus.txStatus == Cv2xStatusType::SUSPENDED
                    || cv2xStatus.txStatus == Cv2xStatusType::ACTIVE)) {
                /*cv2x radio init success only if cv2x status ACTIVE |
                 * SUSPEND*/
                status = telux::common::Status::FAILED;
            }
        }

        auto ipIpface   = getIfaceNameFromIpType(TrafficIpType::TRAFFIC_IP);
        auto nonIpIface = getIfaceNameFromIpType(TrafficIpType::TRAFFIC_NON_IP);
        if (ipIpface.empty() || nonIpIface.empty()) {
            LOG(DEBUG, __FUNCTION__, " empty iface name");
            status = telux::common::Status::FAILED;
        }
        auto ipMtu    = getMTU(ipIpface);
        auto nonIpMtu = getMTU(nonIpIface);
        if (ipMtu <= 0 || nonIpMtu <= 0) {
            LOG(DEBUG, __FUNCTION__, " fail to get Mtu");
            status = telux::common::Status::FAILED;
        } else {
            dummyCap_.linkIpMtuBytes    = getMTU(ipIpface);
            dummyCap_.linkNonIpMtuBytes = getMTU(nonIpIface);
        }

        if (delay >= 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
        }
        setInitializedStatus(status, callback);
    }).share();
    taskQ_->add(f);
}

void Cv2xRadioSimulation::onStatusChanged(Cv2xStatus status) {
    LOG(DEBUG, __FUNCTION__);
    if (not(status.rxStatus == Cv2xStatusType::SUSPENDED
            || status.rxStatus == Cv2xStatusType::ACTIVE)
        && (status.txStatus == Cv2xStatusType::SUSPENDED
            || status.txStatus == Cv2xStatusType::ACTIVE)) {
        /*cv2x radio work only if cv2x status ACTIVE | SUSPEND*/
        setInitializedStatus(common::Status::INVALIDSTATE, nullptr);
    }

    notifyListenersStatusChange(status);
}

void Cv2xRadioSimulation::notifyListenersStatusChange(Cv2xStatus status) {
    auto f = std::async(std::launch::async, [this, status]() {
        telux::cv2x::Cv2xStatusEx statusEx;
        std::vector<std::weak_ptr<ICv2xRadioListener>> lists;

        statusEx.status = status;
        radioListenerMgr_.getAvailableListeners(lists);
        for (auto &wp : lists) {
            if (auto sp = wp.lock()) {
                sp->onStatusChanged(status);
                sp->onStatusChanged(statusEx);
            }
        }
    }).share();
    taskQ_->add(f);
}

telux::common::Status Cv2xRadioSimulation::registerListener(
    std::weak_ptr<ICv2xRadioListener> listener) {
    return radioListenerMgr_.registerListener(listener);
}

telux::common::Status Cv2xRadioSimulation::deregisterListener(
    std::weak_ptr<ICv2xRadioListener> listener) {
    return radioListenerMgr_.deRegisterListener(listener);
}

template <class T>
telux::common::Status Cv2xRadioSimulation::addFlow(
    shared_ptr<T> flow, map<uint32_t, std::shared_ptr<ICv2xTxFlow>> &vec) {

    lock_guard<recursive_mutex> lock(flowsMutex_);

    // Check whether the flow already exists ...
    uint32_t id = flow->getFlowId();
    auto itr    = vec.find(id);

    if (itr != end(vec)) {
        LOG(DEBUG, __FUNCTION__, " flow already added");
        return telux::common::Status::ALREADY;
    }

    // Store flow
    vec[id] = flow;
    return telux::common::Status::SUCCESS;
}

template <class T>
telux::common::Status Cv2xRadioSimulation::removeFlow(
    shared_ptr<T> flow, map<uint32_t, std::shared_ptr<ICv2xTxFlow>> &vec) {

    lock_guard<recursive_mutex> lock(flowsMutex_);

    // Check whether the flow exists ...
    uint32_t id = flow->getFlowId();
    auto itr    = vec.find(id);

    if (itr == end(vec)) {
        LOG(DEBUG, __FUNCTION__, " flow not found");
        return telux::common::Status::NOSUCH;
    }

    // Remove flow
    vec.erase(itr);
    return telux::common::Status::SUCCESS;
}

telux::common::Status Cv2xRadioSimulation::addSubscription(shared_ptr<ICv2xRxSubscription> sub) {
    lock_guard<recursive_mutex> lock(rxSubscriptionsMutex_);

    // Check whether sub already exists ...
    uint32_t id = sub->getSubscriptionId();
    auto itr    = rxSubscriptions_.find(id);

    if (itr != end(rxSubscriptions_)) {
        LOG(DEBUG, __FUNCTION__, " Rx Subscription already added");
        return telux::common::Status::ALREADY;
    }

    // Store sub
    rxSubscriptions_[id] = sub;
    return telux::common::Status::SUCCESS;
}

telux::common::Status Cv2xRadioSimulation::removeSubscription(shared_ptr<ICv2xRxSubscription> sub) {
    lock_guard<recursive_mutex> lock(rxSubscriptionsMutex_);

    // Check whether sub exists ...
    uint32_t id = sub->getSubscriptionId();
    auto itr    = rxSubscriptions_.find(id);

    if (itr == rxSubscriptions_.end()) {
        LOG(DEBUG, __FUNCTION__, " Rx Subscription not found");
        return telux::common::Status::NOSUCH;
    }

    // Remove sub
    rxSubscriptions_.erase(itr);

    return telux::common::Status::SUCCESS;
}

telux::common::Status Cv2xRadioSimulation::initRxSock(
    cv2x::TrafficIpType ipType, int &sock, uint16_t port, struct sockaddr_in6 &sockAddr) {
    auto res = telux::common::Status::FAILED;
    ;
    LOG(DEBUG, __FUNCTION__);

    // Create new socket
    if ((sock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        LOG(ERROR, __FUNCTION__, " Socket creation failed.");
        return telux::common::Status::FAILED;
    }

    auto ifaceName = getIfaceNameFromIpType(ipType);

    sockAddr.sin6_addr     = in6addr_any;
    sockAddr.sin6_family   = AF_INET6;
    sockAddr.sin6_scope_id = if_nametoindex(ifaceName.c_str());
    sockAddr.sin6_port     = htons(port);
    do {
        // The SO_REUSEPORT is helps by allowing  multiple applications to run on
        // the same AP and to all receive copies of the messages. This is critical
        // for congestion/scalability testing. This allows multiple instances of an
        // ITS stack/client test app to all run on the same machine, with their
        // normal SDK client code *HOWEVER* There is a very important caveat This
        // does not actually ensure a copy of each Received packet is duplicated to
        // each instance. That would have to be done by ip6tables or a second
        // application that intercepts the packets produced on this socket.
        int option = 1;
        if (setsockopt(
                sock, SOL_SOCKET, SO_REUSEPORT, reinterpret_cast<void *>(&option), sizeof(option))
            < 0) {
            LOG(ERROR, __FUNCTION__, " setsockopt(SO_REUSEPORT) failed\n");
            break;
        }

        struct sockaddr *bindAddrP = reinterpret_cast<struct sockaddr *>(&sockAddr);
        if (bind(sock, bindAddrP, sizeof(struct sockaddr_in6)) < 0) {
            LOG(ERROR, __FUNCTION__, " Bind failed: ", strerror(errno));
            break;
        }

        LOG(INFO, __FUNCTION__, " RX Socket setup success fd=", sock,
            ", port=", ntohs(sockAddr.sin6_port));

        // subscribe to the IPV6 broadcast addr, since this may be is what
        // transmitter is using
        struct ipv6_mreq mreq = {0};

        LOG(INFO, __FUNCTION__, " RX Socket setup subscribe to global broadcast addr***********");

        mreq.ipv6mr_interface             = if_nametoindex(ifaceName.c_str());
        mreq.ipv6mr_multiaddr.s6_addr[0]  = 0xff;
        mreq.ipv6mr_multiaddr.s6_addr[1]  = 0x02;
        mreq.ipv6mr_multiaddr.s6_addr[15] = 0x02;
        if (setsockopt(sock, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
            LOG(ERROR, __FUNCTION__, " Error! setsockopt(IPV6_ADD_MEMBERSHIP, ff:02::02)");
            break;
        }
        res = telux::common::Status::SUCCESS;
    } while (0);

    if (telux::common::Status::SUCCESS != res) {
        close(sock);
    }
    return res;
}

void Cv2xRadioSimulation::createRxSubscriptionSync(TrafficIpType ipType, uint16_t port,
    CreateRxSubscriptionCallback cb, shared_ptr<vector<uint32_t>> idList) {
    shared_ptr<ICv2xRxSubscription> rxSub = nullptr;
    telux::common::ErrorCode ec           = telux::common::ErrorCode::NO_RESOURCES;
    int delay                             = DEFAULT_DELAY;
    LOG(DEBUG, __FUNCTION__);

    do {
        auto ifaceName = getIfaceNameFromIpType(ipType);
        if (not serviceStub_) {
            break;
        }
        telux::common::Status status = telux::common::Status::FAILED;
        RXSUBSCRIPTION_RPC_CALL(
            serviceStub_->addRxSubscription, ipType, port, idList, status, ec, delay);
        if (status != telux::common::Status::SUCCESS) {
            ec = telux::common::ErrorCode::MODEM_ERR;
            break;
        }

        // Create and initialize Socket
        int sock                     = -1;
        struct sockaddr_in6 sockAddr = {0};
        status                       = initRxSock(ipType, sock, port, sockAddr);
        if (telux::common::Status::SUCCESS == status) {
            LOG(DEBUG, __FUNCTION__, " Rx subscription succeeded");

            rxSub = make_shared<Cv2xRxSubscription>(sock, sockAddr, ipType);

            // set SID list when subscription succeed
            rxSub->setServiceIDList(idList);

            // Add to list of subscriptions. This may be unnecessary.
            // TODO: Revisit if this is necessary. We may want to simply keep a count
            // of all of the service Ids that have been subscribed to.
            addSubscription(rxSub);
        } else {
            // Unsubscribe since socket initialization failed
            LOG(ERROR, "Error occurred in socket initialization. Unsubscribing Rx "
                       "subscription");
            RXSUBSCRIPTION_RPC_CALL(
                serviceStub_->delRxSubscription, ipType, port, idList, status, ec, delay);
            if (telux::common::Status::SUCCESS != status) {
                LOG(ERROR, "Error occurred when unsubscribing Rx Subscription");
                ec = telux::common::ErrorCode::INTERNAL_ERR;
            } else {
                ec = telux::common::ErrorCode::GENERIC_FAILURE;
            }
        }
    } while (0);

    if (cb) {
        if (delay > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
        }
        cb(rxSub, ec);
    }
}

telux::common::Status Cv2xRadioSimulation::createRxSubscription(TrafficIpType ipType, uint16_t port,
    CreateRxSubscriptionCallback cb, std::shared_ptr<std::vector<uint32_t>> idList) {
    if (port < SIMULATION_MINIMUM_PORT_NUMBER) {
        LOG(ERROR, __FUNCTION__, " Invalid port number");
        return telux::common::Status::INVALIDPARAM;
    }
    // User supplied callback must be valid since the Rx Subscription and
    // associated socket is returned in the callback
    if (!cb) {
        LOG(ERROR, __FUNCTION__, " Invalid callback supplied.");
        return telux::common::Status::INVALIDPARAM;
    }

    // Create RX Subscription in async thread
    auto f = std::async(std::launch::async, [this, ipType, port, idList, cb]() {
        this->createRxSubscriptionSync(ipType, port, cb, idList);
    }).share();
    taskQ_->add(f);

    return telux::common::Status::SUCCESS;
}

telux::common::Status Cv2xRadioSimulation::enableRxMetaDataReport(TrafficIpType ipType, bool enable,
    std::shared_ptr<std::vector<std::uint32_t>> idList, telux::common::ResponseCallback cb) {
    ::cv2xStub::RxSubscription request;
    telux::common::Status res = telux::common::Status::FAILED;
    LOG(DEBUG, __FUNCTION__);

    request.set_portnum(static_cast<uint32_t>(enable));
    request.set_iptype(static_cast<uint32_t>(ipType));
    if (idList) {
        for (uint32_t id : *idList) {
            request.add_ids(id);
        }
    }
    CALL_RPC_AND_RESPOND(serviceStub_->enableRxMetaDataReport, request, res, cb, taskQ_);
    return res;
}

telux::common::Status Cv2xRadioSimulation::initTxUdpSock(
    TrafficIpType ipType, int &sock, uint16_t port, struct sockaddr_in6 &sockAddr) {
    telux::common::Status res = telux::common::Status::FAILED;
    // Create Socket
    if ((sock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        LOG(ERROR, __FUNCTION__, " Cannot create socket");
        return telux::common::Status::FAILED;
    }

    // Get IP Address of network interface.
    auto ifaceName                   = getIfaceNameFromIpType(ipType);
    sockAddr.sin6_family             = AF_INET6;
    sockAddr.sin6_port               = htons(port);
    sockAddr.sin6_scope_id           = if_nametoindex(ifaceName.c_str());
    struct sockaddr_in6 destSockaddr = sockAddr;
    getV6AddrByIface(ifaceName, sockAddr.sin6_addr);

    do {
        // Allow multiple clients to bind to same port
        int option = 1;
        if (setsockopt(
                sock, SOL_SOCKET, SO_REUSEPORT, reinterpret_cast<void *>(&option), sizeof(option))
            < 0) {
            LOG(ERROR, __FUNCTION__, " Failed setsockopt(SO_REUSEPORT) on sps socket");
            break;
        }

        // Disable multicast loopback pkts to achieve better latency
        option = 0;
        if (setsockopt(sock, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, reinterpret_cast<void *>(&option),
                sizeof(option))
            < 0) {
            LOG(ERROR, __FUNCTION__, " Failed setsockopt(IPV6_MULTICAST_LOOP) on sps socket");
            break;
        }

        unsigned int connectErrCount = 0;
        std::vector<std::string> destAddr{DEFAULT_DEST_IP_ADDR, LO_IPV6_ADDR};
        for (unsigned int i = 0; i < destAddr.size(); ++i) {
            if (inet_pton(AF_INET6, destAddr[i].c_str(),
                    reinterpret_cast<void *>(&destSockaddr.sin6_addr))) {
            } else {
                LOG(ERROR, __FUNCTION__, " Error identifying , err=", strerror(errno));
                ++connectErrCount;
                continue;
            }

            // Set default destination address
            if (connect(sock, reinterpret_cast<struct sockaddr *>(&destSockaddr),
                    sizeof(struct sockaddr_in6))
                < 0) {
                LOG(WARNING, __FUNCTION__, " Err connecting socket to ", destAddr[i],
                    ", err=", strerror(errno));
                ++connectErrCount;
                continue;
            }
        }
        if (connectErrCount >= destAddr.size()) {
            break;
        }

        res = telux::common::Status::SUCCESS;
        LOG(INFO, __FUNCTION__, " Socket setup success fd=", sock, ", port=", port);
    } while (0);

    if (res != telux::common::Status::SUCCESS && sock >= 0) {
        close(sock);
        sock = -1;
    }
    return res;
}

telux::common::ErrorCode Cv2xRadioSimulation::initTxSpsFlow(TrafficIpType ipType,
    uint32_t serviceId, const SpsFlowInfo &spsInfo, uint16_t spsSrcPort, bool eventSrcPortValid,
    uint16_t eventSrcPort, shared_ptr<ICv2xTxFlow> &txSpsFlow, shared_ptr<ICv2xTxFlow> &txEventFlow,
    telux::common::Status &spsStatus, telux::common::Status &eventStatus, int &delay) {
    LOG(DEBUG, __FUNCTION__);
    telux::common::ErrorCode ec  = telux::common::ErrorCode::SUCCESS;
    auto evtPort                 = eventSrcPortValid ? eventSrcPort : 0;
    uint8_t spsId                = 0;
    telux::common::Status status = telux::common::Status::FAILED;
    do {
        FLOW_RPC_CALL(serviceStub_->registerFlow, ipType, spsSrcPort, evtPort, spsId, serviceId,
            status, ec, delay);
        if (status != telux::common::Status::SUCCESS) {
            spsStatus = telux::common::Status::FAILED;
            if (eventSrcPortValid) {
                eventStatus = telux::common::Status::FAILED;
            }
            break;
        }
        LOG(DEBUG, "SPS Id is ", +spsId);

        // Create and initialize SPS socket
        int spsSock                     = -1;
        struct sockaddr_in6 spsSockAddr = {0};
        spsStatus                       = initTxUdpSock(ipType, spsSock, spsSrcPort, spsSockAddr);
        if (telux::common::Status::SUCCESS == spsStatus) {
            auto flow = make_shared<Cv2xTxSpsFlow>(
                spsId, ipType, serviceId, spsSock, spsSockAddr, spsInfo);
            addFlow<Cv2xTxSpsFlow>(flow, spsFlows_);
            txSpsFlow = flow;
        } else {
            // sps socket init failed, deregister the sps flow in modem and close
            // created sps socket
            LOG(ERROR, "Error occurred during SPS socket initialization. "
                       "Deregistering SPS flow.");
            FLOW_RPC_CALL(serviceStub_->deregisterFlow, ipType, spsSrcPort, evtPort, spsId,
                serviceId, status, ec, delay);
            if (status != telux::common::Status::SUCCESS) {
                LOG(ERROR, "Error occurred in deregistering SPS flow");
            }

            if (eventSrcPortValid) {
                eventStatus = telux::common::Status::FAILED;
            }
            ec = (telux::common::ErrorCode::SUCCESS != ec)
                     ? ec
                     : telux::common::ErrorCode::GENERIC_FAILURE;
        }

        // SPS socket initialization succeeded. Create and initialize Event socket.
        if (eventSrcPortValid) {
            int eventSock                     = -1;
            struct sockaddr_in6 eventSockAddr = {0};
            eventStatus = initTxUdpSock(ipType, eventSock, eventSrcPort, eventSockAddr);
            if (telux::common::Status::SUCCESS == eventStatus) {
                auto flow = make_shared<Cv2xTxEventFlow>(
                    spsId, ipType, serviceId, eventSock, eventSockAddr);
                addFlow<Cv2xTxEventFlow>(flow, eventFlows_);
                txEventFlow = flow;
            } else {
                // event socket init failed, just close created socket, user can still
                // use the sps socket for Tx data pkts
                LOG(ERROR, "Error occurred during event socket initialization. close "
                           "event socket.");
                return telux::common::ErrorCode::GENERIC_FAILURE;
            }
        }
    } while (0);

    return ec;
}

telux::common::Status Cv2xRadioSimulation::createTxSpsFlowSync(TrafficIpType ipType,
    uint32_t serviceId, const SpsFlowInfo &spsInfo, uint16_t spsSrcPort, bool eventSrcPortValid,
    uint16_t eventSrcPort, CreateTxSpsFlowCallback cb) {

    // Create SPS flow
    shared_ptr<ICv2xTxFlow> txSpsFlow(nullptr);
    shared_ptr<ICv2xTxFlow> txEventFlow(nullptr);

    telux::common::Status spsStatus   = telux::common::Status::SUCCESS;
    telux::common::Status eventStatus = telux::common::Status::SUCCESS;
    int delay                         = 0;
    auto errorNum = initTxSpsFlow(ipType, serviceId, spsInfo, spsSrcPort, eventSrcPortValid,
        eventSrcPort, txSpsFlow, txEventFlow, spsStatus, eventStatus, delay);
    auto spsEc   = (telux::common::Status::SUCCESS == spsStatus) ? telux::common::ErrorCode::SUCCESS
                                                                 : errorNum;
    auto eventEc = (telux::common::Status::SUCCESS == eventStatus)
                       ? telux::common::ErrorCode::SUCCESS
                       : errorNum;

    if (cb) {
        if (delay > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
        }
        cb(txSpsFlow, txEventFlow, spsEc, eventEc);
        return telux::common::Status::SUCCESS;
    }

    // SPS flow creation failed
    LOG(ERROR, __FUNCTION__, " SPS Flow creation failed.");
    return telux::common::Status::FAILED;
}

telux::common::Status Cv2xRadioSimulation::createTxSpsFlow(TrafficIpType ipType, uint32_t serviceId,
    const SpsFlowInfo &spsInfo, uint16_t spsSrcPort, bool eventSrcPortValid, uint16_t eventSrcPort,
    CreateTxSpsFlowCallback cb) {

    if (spsSrcPort < SIMULATION_MINIMUM_PORT_NUMBER
        || (eventSrcPortValid && eventSrcPort < SIMULATION_MINIMUM_PORT_NUMBER)) {
        LOG(ERROR, __FUNCTION__, " Invalid port number", spsSrcPort, eventSrcPort);
        return telux::common::Status::INVALIDPARAM;
    }

    // Callback must be valid since the Tx flows and their associated sockets will
    // be returned to the user via the callback.
    if (!cb) {
        LOG(ERROR, __FUNCTION__, " Invalid callback supplied.");
        return telux::common::Status::INVALIDPARAM;
    }

    {
        lock_guard<std::mutex> lock(mutex_);
        if (telux::common::Status::SUCCESS != initializedStatus_) {
            LOG(ERROR, "Radio state (", static_cast<int>(initializedStatus_), ")");
            return telux::common::Status::INVALIDSTATE;
        }
    }

    // Launch async task to create TX SPS flow in background thread
    auto f = std::async(std::launch::async, [=]() {
        this->createTxSpsFlowSync(
            ipType, serviceId, spsInfo, spsSrcPort, eventSrcPortValid, eventSrcPort, cb);
    }).share();
    taskQ_->add(f);

    return telux::common::Status::SUCCESS;
}

telux::common::ErrorCode Cv2xRadioSimulation::initTxEventFlow(TrafficIpType ipType,
    uint32_t serviceId, const EventFlowInfo &flowInfo, uint16_t eventSrcPort,
    CreateTxEventFlowCallback cb) {
    LOG(DEBUG, __FUNCTION__);
    std::shared_ptr<Cv2xTxEventFlow> flow = nullptr;
    telux::common::ErrorCode ec           = telux::common::ErrorCode::GENERIC_FAILURE;
    telux::common::Status status          = telux::common::Status::FAILED;
    int delay                             = DEFAULT_DELAY;
    do {
        uint8_t flowId = 0;
        FLOW_RPC_CALL(serviceStub_->registerFlow, ipType, 0, eventSrcPort, flowId, serviceId,
            status, ec, delay);
        if (status != telux::common::Status::SUCCESS) {
            break;
        }

        // Create and initialize socket
        int sock                     = -1;
        struct sockaddr_in6 sockAddr = {0};
        auto status                  = initTxUdpSock(ipType, sock, eventSrcPort, sockAddr);

        if (telux::common::Status::SUCCESS == status) {
            flow = make_shared<Cv2xTxEventFlow>(flowId, ipType, serviceId, sock, sockAddr);
            addFlow<Cv2xTxEventFlow>(flow, eventFlows_);
        } else {
            // Socket initialization failed. Deregistering TX Event flow
            LOG(ERROR, "Error occurred in socket initialization. Deregistering Tx "
                       "Event flow");
            FLOW_RPC_CALL(serviceStub_->deregisterFlow, ipType, 0, eventSrcPort, flowId, serviceId,
                status, ec, delay);
            if (status != telux::common::Status::SUCCESS) {
                LOG(ERROR, "Error occured when deregistering event flows");
            } else {
                ec = telux::common::ErrorCode::GENERIC_FAILURE;
            }
        }
    } while (0);
    if (cb) {
        if (delay) {
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
        }
        cb(flow, ec);
    }
    return ec;
}

telux::common::Status Cv2xRadioSimulation::createTxEventFlow(
    TrafficIpType ipType, uint32_t serviceId, uint16_t eventSrcPort, CreateTxEventFlowCallback cb) {
    // Set all flowInfo members to invalid
    EventFlowInfo flowInfo;
    flowInfo.autoRetransEnabledValid = false;
    flowInfo.peakTxPowerValid        = false;
    flowInfo.mcsIndexValid           = false;
    flowInfo.txPoolIdValid           = false;

    return createTxEventFlow(ipType, serviceId, flowInfo, eventSrcPort, cb);
}

telux::common::Status Cv2xRadioSimulation::createTxEventFlow(TrafficIpType ipType,
    uint32_t serviceId, const EventFlowInfo &flowInfo, uint16_t eventSrcPort,
    CreateTxEventFlowCallback cb) {
    if (!cb) {
        LOG(ERROR, __FUNCTION__, " Invalid callback supplied.");
        return telux::common::Status::INVALIDPARAM;
    }

    {
        lock_guard<std::mutex> lock(mutex_);
        if (telux::common::Status::SUCCESS != initializedStatus_) {
            LOG(ERROR, "Radio state (", static_cast<int>(initializedStatus_), ")");
            return telux::common::Status::INVALIDSTATE;
        }
    }

    // Launch async task to create TX event flow in background thread
    auto f = std::async(std::launch::async, [=]() {
        this->initTxEventFlow(ipType, serviceId, flowInfo, eventSrcPort, cb);
    }).share();
    taskQ_->add(f);
    return telux::common::Status::SUCCESS;
}

telux::common::Status Cv2xRadioSimulation::closeRxSubscriptionSync(
    std::shared_ptr<ICv2xRxSubscription> rxSub, CloseRxSubscriptionCallback cb) {
    telux::common::Status status = telux::common::Status::FAILED;
    int delay                    = 0;

    lock_guard<recursive_mutex> lock(rxSubscriptionsMutex_);

    // call QMI unsubscribe
    if (rxSubscriptions_.size()) {
        auto subId = rxSub->getSubscriptionId();
        if (rxSubscriptions_.find(subId) != end(rxSubscriptions_)) {
            LOG(DEBUG, __FUNCTION__, " Subscribe ID=", subId);
            ::cv2xStub::RxSubscription request;
            ::cv2xStub::Cv2xCommandReply response;

            request.set_portnum(static_cast<uint32_t>(rxSub->getPortNum()));
            request.set_iptype(static_cast<uint32_t>(rxSub->getIpType()));
            auto idList = rxSub->getServiceIDList();
            if (idList) {
                for (uint32_t id : *idList) {
                    request.add_ids(id);
                }
            }

            CALL_RPC(serviceStub_->delRxSubscription, request, status, response, delay);
        } else {
            LOG(ERROR, __FUNCTION__, " Subscribe ID=", subId, "not found.");
        }
    }

    if (telux::common::Status::SUCCESS != removeSubscription(rxSub)) {
        LOG(WARNING, "Rx subscription was not found in subscriptions map");
    }

    // Close socket
    auto sp = dynamic_pointer_cast<Cv2xRxSubscription>(rxSub);
    if (sp) {
        sp->closeSock();
    }

    // Invoke user-supplied callback
    if (cb) {
        auto ec = (telux::common::Status::SUCCESS == status)
                      ? telux::common::ErrorCode::SUCCESS
                      : telux::common::ErrorCode::INTERNAL_ERROR;
        if (delay > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
        }
        cb(rxSub, ec);
    }

    return status;
}

telux::common::Status Cv2xRadioSimulation::closeRxSubscription(
    std::shared_ptr<ICv2xRxSubscription> rxSub, CloseRxSubscriptionCallback cb) {
    // Verify that the Rx Subscription is valid and that its associated sockeht
    // hasn't already beenn closed
    if (!rxSub or rxSub->getSock() < 0) {
        LOG(ERROR, "Invalid RxSubscription");
        return telux::common::Status::INVALIDPARAM;
    }

    {
        lock_guard<std::mutex> lock(mutex_);
        if (telux::common::Status::SUCCESS != initializedStatus_) {
            LOG(ERROR, "Radio state (", static_cast<int>(initializedStatus_), ")");
            return telux::common::Status::INVALIDSTATE;
        }
    }

    auto f = std::async(std::launch::async, [this, rxSub, cb]() {
        this->closeRxSubscriptionSync(rxSub, cb);
    }).share();
    taskQ_->add(f);
    return telux::common::Status::SUCCESS;
}

telux::common::ErrorCode Cv2xRadioSimulation::closeTxSpsFlowSync(
    std::shared_ptr<ICv2xTxFlow> txFlow, CloseTxFlowCallback cb) {

    auto ec                      = telux::common::ErrorCode::GENERIC_FAILURE;
    telux::common::Status status = telux::common::Status::FAILED;
    int delay                    = DEFAULT_DELAY;
    uint8_t flowId               = static_cast<uint8_t>(txFlow->getFlowId());
    FLOW_RPC_CALL(serviceStub_->deregisterFlow, txFlow->getIpType(), txFlow->getPortNum(), 0,
        flowId, txFlow->getServiceId(), status, ec, delay);
    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " deregisterFlow RPC call ", static_cast<int>(status));
    }

    // Close socket
    auto sp = dynamic_pointer_cast<Cv2xTxSpsFlow>(txFlow);
    if (sp) {
        sp->closeSock();
    }

    // Invoke user-supplied callback
    if (cb) {
        if (delay > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
        }
        cb(txFlow, ec);
    }

    return ec;
}

telux::common::ErrorCode Cv2xRadioSimulation::closeTxEventFlowsSync(
    vector<shared_ptr<ICv2xTxFlow>> &txFlows, CloseTxFlowCallback cb) {
    auto ec = telux::common::ErrorCode::SUCCESS;
    if (txFlows.empty()) {
        return ec;
    }

    LOG(DEBUG, __FUNCTION__);

    // TODO: deregister NonSps flow does not need to be called
    // if the Event flow is associated with an Sps flow. Modem will
    // return port unavailable error code in this case, currently
    // return success to telsdk user until above is resolved.

    for (auto &txFlow : txFlows) {
        telux::common::Status status = telux::common::Status::FAILED;
        int delay                    = DEFAULT_DELAY;
        uint8_t flowId               = static_cast<uint8_t>(txFlow->getFlowId());

        FLOW_RPC_CALL(serviceStub_->deregisterFlow, txFlow->getIpType(), 0, txFlow->getPortNum(),
            flowId, txFlow->getServiceId(), status, ec, delay);
        LOG(DEBUG, __FUNCTION__, " result:", static_cast<int>(status));
        if (telux::common::ErrorCode::V2X_ERR_PORT_UNAVAIL == ec) {
            LOG(DEBUG, __FUNCTION__, " event flow not found in modem, assume success");
            ec = telux::common::ErrorCode::SUCCESS;
        }

        if (telux::common::Status::SUCCESS
            != removeFlow<Cv2xTxEventFlow>(
                dynamic_pointer_cast<Cv2xTxEventFlow>(txFlow), eventFlows_)) {
            LOG(WARNING, "SPS flow was not found in SPS flows map");
        }

        // Close socket
        auto sp = dynamic_pointer_cast<Cv2xTxEventFlow>(txFlow);
        if (sp) {
            sp->closeSock();
        }
        if (cb) {
            if (delay > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(delay));
            }
            cb(txFlow, ec);
        }
    }

    txFlows.clear();
    return ec;
}

telux::common::Status Cv2xRadioSimulation::closeTxFlow(
    std::shared_ptr<ICv2xTxFlow> txFlow, CloseTxFlowCallback cb) {
    if (!txFlow or txFlow->getSock() < 0) {
        LOG(ERROR, " txFlow is invalid");
        return telux::common::Status::INVALIDPARAM;
    }

    {
        lock_guard<std::mutex> lock(mutex_);
        if (telux::common::Status::SUCCESS != initializedStatus_) {
            LOG(ERROR, "Radio state (", static_cast<int>(initializedStatus_), ")");
            return telux::common::Status::INVALIDSTATE;
        }
    }
    LOG(DEBUG, __FUNCTION__, " srvId:", txFlow->getServiceId(), " port:", txFlow->getPortNum());
    if (typeid(Cv2xTxSpsFlow) == typeid(*txFlow.get())) {
        // Close SPS flow in background thread
        auto f = std::async(std::launch::async, [this, txFlow, cb]() {
            this->closeTxSpsFlowSync(txFlow, cb);
            removeFlow<Cv2xTxSpsFlow>(dynamic_pointer_cast<Cv2xTxSpsFlow>(txFlow), spsFlows_);
        }).share();
        taskQ_->add(f);
    } else {
        // Close Non-SPS flow in background thread
        auto f = std::async(std::launch::async, [this, txFlow, cb]() {
            vector<shared_ptr<ICv2xTxFlow>> txFlows = {txFlow};
            this->closeTxEventFlowsSync(txFlows, cb);
        }).share();
        taskQ_->add(f);
    }

    return telux::common::Status::SUCCESS;
}

telux::common::Status Cv2xRadioSimulation::changeSpsFlowInfo(
    std::shared_ptr<ICv2xTxFlow> txFlow, const SpsFlowInfo &spsInfo, ChangeSpsFlowInfoCallback cb) {

    if (not txFlow) {
        LOG(ERROR, " txFlow is invalid");
        return telux::common::Status::INVALIDPARAM;
    }

    // Tx Flows can be either SPS flows or Event flows. The current implementation
    // makes this opaque to the end user - that is, a user can request an SPS flow
    // but the API may actually return an Event flow if the SPS registeration
    // fails. We need to use RTTI and downcasting to verify that this is an actual
    // SPS flow.
    if (typeid(Cv2xTxSpsFlow) != typeid(*txFlow.get())) {
        LOG(WARNING, "Cannot update TX Reservation for Non-SPS Flow");
        return telux::common::Status::INVALIDPARAM;
    }

    {
        lock_guard<std::mutex> lock(mutex_);
        if (telux::common::Status::SUCCESS != initializedStatus_) {
            LOG(ERROR, "Radio state (", static_cast<int>(initializedStatus_), ")");
            return telux::common::Status::INVALIDSTATE;
        }
    }

    auto f = std::async(std::launch::async, [this, txFlow, cb, spsInfo]() {
        auto ec         = telux::common::ErrorCode::GENERIC_FAILURE;
        auto txFlowImpl = std::dynamic_pointer_cast<Cv2xTxSpsFlow>(txFlow);
        if (txFlowImpl) {
            txFlowImpl->setSpsFlowInfo(spsInfo);
            ec = telux::common::ErrorCode::SUCCESS;
        }
        if (cb) {
            cb(txFlow, ec);
        }
    }).share();
    taskQ_->add(f);
    return telux::common::Status::SUCCESS;
}

telux::common::Status Cv2xRadioSimulation::requestSpsFlowInfo(
    std::shared_ptr<ICv2xTxFlow> txFlow, RequestSpsFlowInfoCallback cb) {
    // Valid callback must be supplied
    if (not cb) {
        LOG(ERROR, "Invalid callback.");
        return telux::common::Status::INVALIDPARAM;
    }

    {
        lock_guard<std::mutex> lock(mutex_);
        if (telux::common::Status::SUCCESS != initializedStatus_) {
            LOG(ERROR, "Radio state (", static_cast<int>(initializedStatus_), ")");
            return telux::common::Status::INVALIDSTATE;
        }
    }

    auto f = std::async(std::launch::async, [this, txFlow, cb]() {
        SpsFlowInfo spsInfo;
        auto ec         = telux::common::ErrorCode::RADIO_NOT_AVAILABLE;
        auto txFlowImpl = std::dynamic_pointer_cast<Cv2xTxSpsFlow>(txFlow);
        if (txFlowImpl) {
            spsInfo = txFlowImpl->getSpsFlowInfo();
            ec      = telux::common::ErrorCode::SUCCESS;
        }
        if (cb) {
            cb(txFlow, spsInfo, ec);
        }
    }).share();
    taskQ_->add(f);
    return telux::common::Status::SUCCESS;
}

telux::common::Status Cv2xRadioSimulation::changeEventFlowInfo(std::shared_ptr<ICv2xTxFlow> txFlow,
    const EventFlowInfo &flowInfo, ChangeEventFlowInfoCallback cb) {
    if (not txFlow) {
        LOG(ERROR, " txFlow is invalid");
        return telux::common::Status::INVALIDPARAM;
    }

    // Tx Flows can be either SPS flows or Event flows. The current implementation
    // makes this opaque to the end user - that is, a user can request an SPS flow
    // but the API may actually return an SPS flow if the SPS registeration fails.
    // We need to use RTTI and downcasting to verify that this is an actual Event
    // flow.
    if (typeid(Cv2xTxEventFlow) != typeid(*txFlow.get())) {
        LOG(WARNING, "Flow is not of Event Type");
        return telux::common::Status::INVALIDPARAM;
    }

    {
        lock_guard<std::mutex> lock(mutex_);
        if (telux::common::Status::SUCCESS != initializedStatus_) {
            LOG(ERROR, "Radio state (", static_cast<int>(initializedStatus_), ")");
            return telux::common::Status::INVALIDSTATE;
        }
    }
    auto f = std::async(std::launch::async, [this, txFlow, flowInfo, cb]() {
        auto ec         = telux::common::ErrorCode::RADIO_NOT_AVAILABLE;
        auto txFlowImpl = std::dynamic_pointer_cast<Cv2xTxEventFlow>(txFlow);
        if (txFlowImpl) {
            txFlowImpl->setFlowInfo(flowInfo);
            ec = telux::common::ErrorCode::SUCCESS;
        }
        if (cb) {
            cb(txFlow, ec);
        }
    }).share();
    taskQ_->add(f);
    return telux::common::Status::SUCCESS;
}

telux::common::Status Cv2xRadioSimulation::updateSrcL2Info(UpdateSrcL2InfoCallback cb) {
    telux::common::Status status = telux::common::Status::FAILED;
    const ::google::protobuf::Empty request;

    CALL_RPC_AND_RESPOND(serviceStub_->updateSrcL2Info, request, status, cb, taskQ_);
    return status;
}

telux::common::Status Cv2xRadioSimulation::updateTrustedUEList(
    const TrustedUEInfoList &infoList, UpdateTrustedUEListCallback cb) {
    telux::common::Status status = telux::common::Status::FAILED;
    const ::google::protobuf::Empty request;

    CALL_RPC_AND_RESPOND(serviceStub_->updateTrustedUEList, request, status, cb, taskQ_);
    return status;
}

std::string Cv2xRadioSimulation::getIfaceNameFromIpType(TrafficIpType ipType) {
    if (ifaces_[ipType].empty()) {
        telux::common::Status status = telux::common::Status::FAILED;
        ::cv2xStub::IpType request;
        ::cv2xStub::IfaceNameReply response;
        int delay = 0;
        (void)delay;
        request.set_type(static_cast<uint32_t>(ipType));
        CALL_RPC(serviceStub_->getIfaceNameFromIpType, request, status, response, delay);
        if (status == telux::common::Status::SUCCESS) {
            ifaces_[ipType] = response.name();
        }
    }
    return ifaces_[ipType];
}

int Cv2xRadioSimulation::getSockAddr(
    TrafficIpType ipType, uint16_t port, struct sockaddr_in6 &sockAddr) {
    LOG(DEBUG, __FUNCTION__);

    auto ifaceName = getIfaceNameFromIpType(ipType);
    if (ifaceName.empty()) {
        LOG(ERROR, __FUNCTION__, " interface invalid");
        return -EINVAL;
    }

    auto res               = getV6AddrByIface(ifaceName, sockAddr.sin6_addr);
    sockAddr.sin6_family   = AF_INET6;
    sockAddr.sin6_port     = htons(port);
    sockAddr.sin6_scope_id = if_nametoindex(ifaceName.c_str());
    return res;
}

// Creates a new TCP socket and binds it to the specified source port and
// local IPv6 address of IP interface.
telux::common::Status Cv2xRadioSimulation::initTcpSock(
    const SocketInfo &sockInfo, int &sock, struct sockaddr_in6 &sockAddr) {
    sock                         = -1;
    sockAddr                     = {0};
    telux::common::Status status = telux::common::Status::SUCCESS;

    LOG(DEBUG, __FUNCTION__, " SID=", sockInfo.serviceId, " localPort=", sockInfo.localPort);

    // create tcp socket
    if ((sock = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        LOG(ERROR, __FUNCTION__, " TCP Socket creation failed, err=\n", strerror(errno));
        return telux::common::Status::FAILED;
    }

    // allow multiple clients to bind to the same IP address with different port,
    // and allow binding a socket in TIME_WAIT state
    int option = 1;
    if (setsockopt(
            sock, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<void *>(&option), sizeof(option))
        < 0) {
        LOG(ERROR, __FUNCTION__, " setsockopt(SO_REUSEADDR) failed, err=\n", strerror(errno));
        status = telux::common::Status::FAILED;
    }

    // get local IP address for binding
    if (status == telux::common::Status::SUCCESS) {
        if (getSockAddr(TrafficIpType::TRAFFIC_IP, sockInfo.localPort, sockAddr)
            || bind(sock, reinterpret_cast<struct sockaddr *>(&sockAddr),
                   sizeof(struct sockaddr_in6))
                   < 0) {
            LOG(ERROR, __FUNCTION__, " TCP sock bind failed: ", strerror(errno));
            status = telux::common::Status::FAILED;
        } else {
            auto ifaceName = getIfaceNameFromIpType(TrafficIpType::TRAFFIC_IP);
            if (ifaceName.empty()) {
                LOG(ERROR, __FUNCTION__, " interface invalid");
                status = telux::common::Status::FAILED;
            }
        }
    }

    if (status != telux::common::Status::SUCCESS) {
        close(sock);
        sock = -1;
        return status;
    }

    LOG(INFO, __FUNCTION__, " succeeded in creating TCP socket, fd=", sock);
    return telux::common::Status::SUCCESS;
}

// The caller must call this API to close a TCP socket before removing
// the associated Tx/Rx flow since a 4-way close is needed if the socket
// is connected to the peer.
void Cv2xRadioSimulation::closeTcpSock(int sock) {
    LOG(INFO, __FUNCTION__, " fd=", sock);

    if (sock < 0) {
        return;
    }

    // the TCP socket may be marked as a listening socket by userspace,
    // use SO_ACCEPTCONN value to identify a listening socket
    int val;
    socklen_t len = sizeof(val);
    if (getsockopt(sock, SOL_SOCKET, SO_ACCEPTCONN, &val, &len) < 0) {
        LOG(ERROR, __FUNCTION__, " getsockopt SO_ACCEPTCONN failed, err=", strerror(errno));
    } else if (val) {
        LOG(INFO, __FUNCTION__, " close listening sock:", sock);
    } else {
        // For non-listening TCP socket, calls shutdown to send out FIN to the peer.
        if (shutdown(sock, SHUT_WR) < 0) {
            // The socket is not connected
            LOG(INFO, __FUNCTION__, " shutdown sock=", sock, " err=", strerror(errno));
        } else {
            // The socket is connected
            struct timeval tv;
            tv.tv_sec  = 1;  // 1 sec
            tv.tv_usec = 0;
            if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
                LOG(ERROR, __FUNCTION__, " setsockopt SO_RCVTIMEO failed, err=", strerror(errno));
            } else {
                char buf[512] = {0};
                int rv        = -1;
                // wait the FIN ACK and the 2nd FIN from the peer for 1 sec.
                while ((rv = recv(sock, buf, sizeof(buf), 0)) > 0) {
                    LOG(DEBUG, __FUNCTION__, " drop Rx pkt from sock=", sock, " len=", rv);
                }
            }

            // set NODELAY to push out the last ACK in send buffer before removing
            // rx/tx flow, otherwise the connection at the peer side might stuck in
            // state LAST_ACK.
            int option = 1;
            if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<void *>(&option),
                    sizeof(option))
                < 0) {
                LOG(ERROR, __FUNCTION__, " set no delay failed, err=", strerror(errno));
            }
        }
    }

    // close the fd
    close(sock);
}

// check whether a TCP socket associated with the specified service ID is
// present, excluding socket ID if specified
bool Cv2xRadioSimulation::isTcpSocketPresent(uint32_t serviceId, bool isExclId, uint32_t exclId) {
    lock_guard<recursive_mutex> lock(tcpSockMutex_);

    auto it = std::find_if(tcpSockets_.begin(), tcpSockets_.end(),
        [=](std::pair<uint32_t, std::shared_ptr<ICv2xTxRxSocket>> sock) {
            return ((sock.second->getServiceId() == serviceId)
                    && (isExclId ? (sock.second->getId() != exclId) : true));
        });
    if (it != tcpSockets_.end()) {
        LOG(DEBUG, __FUNCTION__, " find TCP socket for the same SID.");
        return true;
    }

    return false;
}

telux::common::Status Cv2xRadioSimulation::addTcpSocket(shared_ptr<ICv2xTxRxSocket> sock) {
    lock_guard<recursive_mutex> lock(tcpSockMutex_);

    uint32_t id = sock->getId();
    auto itr    = tcpSockets_.find(id);

    if (itr != end(tcpSockets_)) {
        LOG(ERROR, __FUNCTION__, " socket ID already exists");
        return telux::common::Status::ALREADY;
    }

    tcpSockets_[id] = sock;
    return telux::common::Status::SUCCESS;
}

telux::common::Status Cv2xRadioSimulation::removeTcpSocket(shared_ptr<ICv2xTxRxSocket> sock) {
    lock_guard<recursive_mutex> lock(tcpSockMutex_);

    uint32_t id = sock->getId();
    auto itr    = tcpSockets_.find(id);

    if (itr == std::end(tcpSockets_)) {
        LOG(ERROR, __FUNCTION__, " socket ID not found");
        return telux::common::Status::NOSUCH;
    }

    tcpSockets_.erase(itr);
    return telux::common::Status::SUCCESS;
}

telux::common::Status Cv2xRadioSimulation::createCv2xTcpSocketSync(
    const EventFlowInfo &eventInfo, const SocketInfo &sockInfo, CreateTcpSocketCallback cb) {
    int sock                        = -1;
    struct sockaddr_in6 srcSockAddr = {0};
    telux::common::Status status    = telux::common::Status::FAILED;
    telux::common::ErrorCode ec     = telux::common::ErrorCode::GENERIC_FAILURE;
    int delay                       = 0;
    int32_t flowId                  = 0;
    auto ipType                     = static_cast<uint32_t>(TrafficIpType::TRAFFIC_IP);
    bool addRx                      = false;
    bool addTx                      = false;
    std::vector<uint32_t> idVec{sockInfo.serviceId};
    auto idList                            = make_shared<std::vector<uint32_t>>(idVec);
    std::shared_ptr<Cv2xTxRxSocket> socket = nullptr;

    do {
        // Creates a new TCP socket and binds to the specified src port
        status = initTcpSock(sockInfo, sock, srcSockAddr);
        if (status != telux::common::Status::SUCCESS) {
            LOG(ERROR, __FUNCTION__, " error occurred when creating socket.");
            break;
        }

        // subscribes Rx service ID for TCP/IP socket if the service ID not
        // subscribed yet.
        //  UDP and TCP should always use different service IDs.
        //  TODO: need add a mechanism to handle dupicated SID
        //  subscription/unsubscription among processes.
        if (isTcpSocketPresent(sockInfo.serviceId, false, 0)) {
            LOG(DEBUG, __FUNCTION__, " SID already suscribed");
            status = telux::common::Status::ALREADY;
            break;
        } else {
            RXSUBSCRIPTION_RPC_CALL(serviceStub_->addRxSubscription, ipType, sockInfo.localPort,
                idList, status, ec, delay);
            if (status == telux::common::Status::SUCCESS) {
                LOG(DEBUG, __FUNCTION__, " succeeded in adding Rx to socket, fd=", sock);
                addRx = true;
            } else {
                // add Rx failed, close socket and return failure
                LOG(ERROR, __FUNCTION__, " error occurred when adding Rx to socket, fd=", sock);
                break;
            }
        }

        // register event Tx flow for the TCP/IP channel
        FLOW_RPC_CALL(serviceStub_->registerFlow, ipType, 0, sockInfo.localPort, flowId,
            sockInfo.serviceId, status, ec, delay);
        if (status == telux::common::Status::SUCCESS) {
            LOG(DEBUG, __FUNCTION__, " succeeded in adding Tx to socket, fd=", sock);
            addTx = true;
        } else {
            // add Tx failed, remove Rx if added and close socket later
            LOG(ERROR, __FUNCTION__, " error occurred when adding Tx to socket, fd=", sock);
            break;
        }

        socket = make_shared<Cv2xTxRxSocket>(
            sockInfo.serviceId, sock, srcSockAddr, static_cast<uint32_t>(flowId));
        status = addTcpSocket(socket);
        if (telux::common::Status::SUCCESS != status) {
            // This should not occur, but if the socket ID already exists in the map,
            // take it as an error and close the new coming socket.
            ec = telux::common::ErrorCode::DEVICE_IN_USE;
            LOG(ERROR, __FUNCTION__, "error occurred when adding Id=", socket->getId());
            socket = nullptr;
        }
    } while (0);

    if (telux::common::Status::SUCCESS != status) {
        // error proc  close created sock
        if (sock > 0) {
            close(sock);
        }

        // remove new added Rx subscription
        if (addRx) {
            RXSUBSCRIPTION_RPC_CALL(serviceStub_->delRxSubscription, ipType, sockInfo.localPort,
                idList, status, ec, delay);
            if (telux::common::ErrorCode::SUCCESS != ec) {
                LOG(ERROR, __FUNCTION__, " error occurred when removing Rx.");
            }
        }

        // remove new added Tx event flow
        if (addTx) {
            FLOW_RPC_CALL(serviceStub_->deregisterFlow, ipType, 0, sockInfo.localPort, flowId,
                sockInfo.serviceId, status, ec, delay);
            if (telux::common::ErrorCode::SUCCESS != ec) {
                LOG(ERROR, __FUNCTION__, " error occurred when removing Tx.");
            }
        }
    } else {
        LOG(DEBUG, __FUNCTION__, " succeeded in TCP socket creation, Id=", socket->getId(),
            " fd=", sock);
    }
    if (cb) {
        if (delay > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
        }
        cb(socket, ec);
    }
    return status;
}

telux::common::Status Cv2xRadioSimulation::createCv2xTcpSocket(
    const EventFlowInfo &eventInfo, const SocketInfo &sockInfo, CreateTcpSocketCallback cb) {
    LOG(DEBUG, __FUNCTION__, " SID=", sockInfo.serviceId, ", localPort=", sockInfo.localPort);

    if (!cb) {
        LOG(ERROR, __FUNCTION__, " Invalid parameters.");
        return telux::common::Status::INVALIDPARAM;
    }

    // Launch async task to create TCP socket in background thread
    auto f = std::async(std::launch::async, [=]() {
        this->createCv2xTcpSocketSync(eventInfo, sockInfo, cb);
    }).share();
    taskQ_->add(f);
    return telux::common::Status::SUCCESS;
}

telux::common::Status Cv2xRadioSimulation::closeCv2xTcpSocketSync(
    shared_ptr<ICv2xTxRxSocket> sock, CloseTcpSocketCallback cb) {
    LOG(DEBUG, __FUNCTION__, " Id=", sock->getId());
    auto ipType                  = static_cast<uint32_t>(TrafficIpType::TRAFFIC_IP);
    telux::common::Status status = telux::common::Status::FAILED;
    int delay                    = 0;
    // close TCP socket before removing Tx/Rx flow
    closeTcpSock(sock->getSocket());

    // remove associated Rx subs if no other TCP socket is using the same SID
    telux::common::ErrorCode rxStatus = telux::common::ErrorCode::SUCCESS;
    if (isTcpSocketPresent(sock->getServiceId(), true, sock->getId())) {
        LOG(DEBUG, __FUNCTION__, " exist other TCP socket with the same SID, not remove Rx.");
    } else {
        vector<uint32_t> idVec{sock->getServiceId()};
        auto idList = make_shared<std::vector<uint32_t>>(idVec);
        RXSUBSCRIPTION_RPC_CALL(serviceStub_->delRxSubscription, ipType, sock->getPortNum(), idList,
            status, rxStatus, delay);
        if (rxStatus != telux::common::ErrorCode::SUCCESS) {
            LOG(ERROR, __FUNCTION__, " error occurred when removing Rx.");
        } else {
            LOG(DEBUG, __FUNCTION__, " succeeded in removing Rx.");
        }
    }

    // remove associated event Tx flow
    telux::common::ErrorCode txStatus = telux::common::ErrorCode::SUCCESS;
    auto flowId                       = sock->getId();
    FLOW_RPC_CALL(serviceStub_->deregisterFlow, ipType, 0, sock->getPortNum(), flowId,
        sock->getServiceId(), status, txStatus, delay);
    if (txStatus != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " error occurred when removing Tx.");
    } else {
        LOG(DEBUG, __FUNCTION__, " succeeded in removing Tx.");
    }

    // remove socket from map
    auto rmStatus = removeTcpSocket(sock);
    if (rmStatus != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " error occurred when removing socket.");
    }

    telux::common::ErrorCode ec = telux::common::ErrorCode::GENERIC_FAILURE;
    if (rxStatus == telux::common::ErrorCode::SUCCESS
        && txStatus == telux::common::ErrorCode::SUCCESS
        && rmStatus == telux::common::Status::SUCCESS) {
        ec = telux::common::ErrorCode::SUCCESS;
    } else if (rxStatus != telux::common::ErrorCode::SUCCESS) {
        ec = rxStatus;
    } else if (txStatus != telux::common::ErrorCode::SUCCESS) {
        ec = txStatus;
    }

    if (cb) {
        if (delay > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
        }
        cb(sock, ec);
    }
    return (ec == telux::common::ErrorCode::SUCCESS) ? telux::common::Status::SUCCESS
                                                     : telux::common::Status::FAILED;
}

telux::common::Status Cv2xRadioSimulation::closeCv2xTcpSocket(
    shared_ptr<ICv2xTxRxSocket> sock, CloseTcpSocketCallback cb) {
    LOG(DEBUG, __FUNCTION__);

    if (!sock or sock->getSocket() < 0) {
        LOG(ERROR, __FUNCTION__, " Invalid TCP socket");
        return telux::common::Status::INVALIDPARAM;
    }

    // Launch async task to close TCP socket in background thread
    auto f
        = std::async(std::launch::async, [=]() { this->closeCv2xTcpSocketSync(sock, cb); }).share();
    taskQ_->add(f);
    return telux::common::Status::SUCCESS;
}

telux::common::Status Cv2xRadioSimulation::registerTxStatusReportListener(uint16_t port,
    std::shared_ptr<ICv2xTxStatusReportListener> listener, telux::common::ResponseCallback cb) {
    telux::common::Status status = telux::common::Status::FAILED;
    ::cv2xStub::UintNum request;
    ::cv2xStub::Cv2xCommandReply response;
    int delay = DEFAULT_DELAY;

    request.set_num(static_cast<uint32_t>(port));
    CALL_RPC(serviceStub_->enableTxStatusReport, request, status, response, delay);
    if (telux::common::Status::SUCCESS == static_cast<telux::common::Status>(status)) {
        {
            lock_guard<std::mutex> lock(txStatusMtx_);
            if (txStatusListeners_.find(port) == txStatusListeners_.end()) {
                txStatusListeners_[port] = listener;
            } else {
                LOG(ERROR, __FUNCTION__, " ALREADY ", static_cast<int>(port));
                return telux::common::Status::ALREADY;
            }
        }
        if (cb && taskQ_) {
            auto f = std::async(std::launch::async, [=]() {
                if (delay > 0) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                }
                cb(static_cast<telux::common::ErrorCode>(response.error()));
            }).share();
            taskQ_->add(f);
        }
    } else {
        LOG(ERROR, __FUNCTION__, " Failed from RPC call");
    }
    return status;
}

telux::common::Status Cv2xRadioSimulation::deregisterTxStatusReportListener(
    uint16_t port, telux::common::ResponseCallback cb) {
    telux::common::Status status = telux::common::Status::FAILED;
    ::cv2xStub::UintNum request;

    {
        lock_guard<std::mutex> lock(txStatusMtx_);
        auto itr = txStatusListeners_.find(port);
        if (itr != std::end(txStatusListeners_)) {
            txStatusListeners_.erase(itr);
        } else {
            LOG(ERROR, __FUNCTION__, " ALREADY ", static_cast<int>(port));
            return telux::common::Status::ALREADY;
        }
    }

    request.set_num(static_cast<uint32_t>(port));
    CALL_RPC_AND_RESPOND(serviceStub_->disableTxStatusReport, request, status, cb, taskQ_);
    return status;
}

void Cv2xRadioSimulation::closeAllCv2xTcpSockets() {
    std::lock_guard<recursive_mutex> lock(tcpSockMutex_);

    while (tcpSockets_.size() > 0) {
        auto &sock = tcpSockets_.begin()->second;
        // not wait for indicaiton when remove TCP flows in destructor
        closeCv2xTcpSocketSync(sock, nullptr);
    }
}

void Cv2xRadioSimulation::unsubscribeAllRxSubs() {
    // call closeRxSubscription for every RX Sub and each one will cause a QMI
    // Unsubscribe
    std::lock_guard<recursive_mutex> lock(rxSubscriptionsMutex_);

    // Since the size of the map is being modified we cannot use any iterator
    // based loops.
    while (rxSubscriptions_.size() > 0) {
        auto &rxSub = rxSubscriptions_.begin()->second;
        closeRxSubscriptionSync(rxSub, nullptr);
    }
}

void Cv2xRadioSimulation::cleanupAllEventFlows() {

    // Iterate over all remaining event flows and close them. We can do this in
    // a single QMI call by passing in a list of event flows.'
    std::lock_guard<recursive_mutex> lock(flowsMutex_);
    vector<shared_ptr<ICv2xTxFlow>> txFlows;
    for (auto &kv : eventFlows_) {
        txFlows.push_back(kv.second);
    }
    closeTxEventFlowsSync(txFlows, nullptr);
}

void Cv2xRadioSimulation::cleanupAllSpsFlows() {
    std::lock_guard<recursive_mutex> lock(flowsMutex_);
    for (auto &kv : spsFlows_) {
        auto &txFlow = kv.second;
        closeTxSpsFlowSync(txFlow, nullptr);
    }

    spsFlows_.clear();
}

telux::common::Status Cv2xRadioSimulation::setGlobalIPInfo(
    const IPv6AddrType &ipv6Addr, common::ResponseCallback cb) {
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status Cv2xRadioSimulation::setGlobalIPUnicastRoutingInfo(
    const GlobalIPUnicastRoutingInfo &destL2Addr, common::ResponseCallback cb) {
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status Cv2xRadioSimulation::requestCapabilities(RequestCapabilitiesCallback cb) {
    cb(dummyCap_, telux::common::ErrorCode::SUCCESS);
    return telux::common::Status::SUCCESS;
}

telux::common::Status Cv2xRadioSimulation::requestDataSessionSettings(
    RequestDataSessionSettingsCallback cb) {
    return telux::common::Status::NOTSUPPORTED;
}

Cv2xRadioCapabilities Cv2xRadioSimulation::getCapabilities() const {
    return dummyCap_;
}

int Cv2xRadioSimulation::getMTU(std::string interfaceName) {
    struct ifreq ifr = {0};
    int mtu          = 0;
    int fd           = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (fd >= 0) {
        memcpy(ifr.ifr_name, interfaceName.c_str(), sizeof(ifr.ifr_name));
        if (ioctl(fd, SIOCGIFFLAGS, &ifr) >= 0 && ifr.ifr_flags & IFF_RUNNING) {
            if (ioctl(fd, SIOCGIFMTU, &ifr) != -1) {
                mtu = ifr.ifr_mtu;
            }
        }
        close(fd);
    }
    return mtu;
}

telux::common::Status Cv2xRadioSimulation::injectVehicleSpeed(
    uint32_t speed, telux::common::ResponseCallback cb) {
    telux::common::Status status = telux::common::Status::FAILED;
    ::cv2xStub::UintNum request;

    request.set_num(static_cast<uint32_t>(speed));
    CALL_RPC_AND_RESPOND(serviceStub_->injectVehicleSpeed, request, status, cb, taskQ_);
    return status;
}

}  // namespace cv2x
}  // namespace telux
