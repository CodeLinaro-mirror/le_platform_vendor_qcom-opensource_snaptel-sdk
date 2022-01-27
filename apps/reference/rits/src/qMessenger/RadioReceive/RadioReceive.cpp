/*
 *  Copyright (c) 2020-2021, The Linux Foundation. All rights reserved.
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
/*
 *  Changes from Qualcomm Innovation Center are provided under the following license:
 *
 *  Copyright (c) 2021 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


 /**
  * @file: RadioReceive.cpp
  *
  * @brief: Implementation of RadioReceive
  *
  */

#include "RadioReceive.h"
static int gRxCount = 0;
void RadioReceive::rxSubCallback(shared_ptr<ICv2xRxSubscription> rxSub, ErrorCode error) {
    if (ErrorCode::SUCCESS == error) {
        this->gRxSub = rxSub;
    }
    this->gCallbackPromise.set_value(error);
};

void RadioReceive::commonStatusCallback(ErrorCode error) {
    this->gCallbackPromise.set_value(error);
}

void RadioReceive::createTcpSocketCallback(std::shared_ptr<ICv2xTxRxSocket> sock, ErrorCode error) {
    if (ErrorCode::SUCCESS == error) {
        this->tcpSockInfo = sock;
    }
    this->gCallbackPromise.set_value(error);
}

void RadioReceive::closeTcpSocketCallback(std::shared_ptr<ICv2xTxRxSocket> sock, ErrorCode error) {
    this->gCallbackPromise.set_value(error);
}

RadioReceive::RadioReceive(const TrafficCategory category, const TrafficIpType trafficIpType,
                            const uint16_t port){

    if (!this->ready(category, RadioType::RX)) {
        cout << "Radio Checks on RadioReceive creation fail\n";
    }
    this->category = category;
    auto cv2xRadio = this->cv2xRadioManager->getCv2xRadio(category);
    auto respCb = [&](std::shared_ptr<ICv2xRxSubscription> rxSub,
                            ErrorCode error){
                                rxSubCallback(rxSub, error);
                            };
    if (Status::SUCCESS == cv2xRadio->createRxSubscription(trafficIpType, port, respCb))
    {
        if (ErrorCode::SUCCESS == this->gCallbackPromise.get_future().get())
        {
            cout<<"Rx Subscription creation succeeds.\n";
        }else{
            cout<<"Rx Subscription creation fails.\n";
        }
    }else{
            cout<<"Rx Subscription creation fails.\n";
    }
    this->resetCallbackPromise();
}
RadioReceive::RadioReceive(const TrafficCategory category, 
                            const TrafficIpType trafficIpType, const uint16_t port, 
                            std::shared_ptr<std::vector<uint32_t>> idList){

    if (!this->ready(category, RadioType::RX)) {
        cout << "Radio Checks on RadioReceive creation fail\n";
    }
    this->category = category;
    auto cv2xRadio = this->cv2xRadioManager->getCv2xRadio(category);
    auto respCb = [&](std::shared_ptr<ICv2xRxSubscription> rxSub,
                            ErrorCode error){
                                rxSubCallback(rxSub, error);
                            };
    if (Status::SUCCESS == cv2xRadio->createRxSubscription(trafficIpType, port, respCb, idList))
    {
        if (ErrorCode::SUCCESS == this->gCallbackPromise.get_future().get())
        {
            cout<<"Rx Subscription creation succeeds for SID: ";
            auto idp = idList.get();
            for(int i=0; i < idp->size(); i++)
                std::cout << idp->at(i) << ' ';
        }else{
            cout<<"Rx Subscription creation fails.\n";
        }
    }else{
            cout<<"Rx Subscription creation fails.\n";
    }
    this->resetCallbackPromise();
}

/*
 * RadioReceive ctor for only simulation purposes. Communication over Ethernet.
 */
RadioReceive::RadioReceive(RadioOpt radioOpt, const string ipv4_dst,
                             const uint16_t port) {
    isSim = true;
    this->enableUdp = radioOpt.enableUdp;
    this->ipv4_src = radioOpt.ipv4_src;
    this->srcAddress = {0};
    this->serverAddress = {0};
    this->simRxSock = -1;
    // Creating socket file descriptor

    if (!this->enableUdp) {
        this->simListenSock = socket(AF_INET, SOCK_STREAM, 0);
    } else {
        this->simListenSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    }

    if (this->simListenSock < 0)
    {
        cout << "Error Creating Socket";
        return;
    }

    if(!this->enableUdp){
        this->srcAddress.sin_family = AF_INET;
        this->srcAddress.sin_port = htons(port);
        if(inet_pton(AF_INET, ipv4_dst.data(), &(this->srcAddress.sin_addr)) <= 0) {
            cerr << "TCP: Invalid ip address of other device " << ipv4_dst << endl;
            cerr << "TCP: Will attempt accepting from any ip address now " << endl;
            this->srcAddress.sin_addr.s_addr = htonl(INADDR_ANY);
        }

        this->serverAddress.sin_family = AF_INET;
        this->serverAddress.sin_port = htons(port);
        if(inet_pton(AF_INET, ipv4_src.data(), &(this->serverAddress.sin_addr)) <= 0) {
            cerr << "Invalid ip address for this device: " << ipv4_src << endl;
        } else {
            if (bind(simListenSock, (struct sockaddr*) &(this->serverAddress),
                     sizeof(this->serverAddress)) < 0) {
                cerr << "Socket " << simListenSock <<
                        " with IP: " << ipv4_dst << " and port: " << endl;
                cerr << port << " failed binding" << endl;
            } else {
                if (listen(simListenSock, 1) < 0) {
                    cerr << "Socket fails to listen\n";
                } else {
                    const auto len = sizeof(this->srcAddress);
                    simRxSock = accept(simListenSock, (struct sockaddr*)&(this->srcAddress),
                                    (socklen_t*)&len);
                    cout << "Connection Received";
                }
            }
        }
    }else{
        /* UDP Communication */
        // setting up network parameters for sender and receiving devices
        this->srcAddress.sin_family = AF_INET;
        this->srcAddress.sin_port = htons(port);
        if(inet_pton(AF_INET, ipv4_dst.data(), &(this->srcAddress.sin_addr)) <= 0){
            cerr << "UDP: Invalid ip address of other device " << ipv4_dst << endl;
            cerr << "UDP: Will attempt accepting from any ip address now " << endl;
            this->srcAddress.sin_addr.s_addr = htonl(INADDR_ANY);
        }

        this->serverAddress.sin_family = AF_INET;
        this->serverAddress.sin_port = htons(port);
        if(inet_pton(AF_INET, ipv4_src.data(), &(this->serverAddress.sin_addr)) <= 0){
            cerr << "Invalid ip address for this device: " << ipv4_src << endl;
        } else {
            // bind to socket
            if (bind(this->simListenSock, (struct sockaddr *) &this->serverAddress,
                     sizeof(this->serverAddress)) < 0){
                cerr << "ERROR on UDP binding" << endl;
                exit(0);
            } else {
                cout << "UDP bind successful" << endl;
            }
        }
    }
}


uint32_t RadioReceive::receive(const char* buf, int len) {
    uint8_t sourceMac[CV2X_MAC_ADDR_LEN];
    int cv2x_mac_addr_len = CV2X_MAC_ADDR_LEN;

    return receive(buf, len, sourceMac, cv2x_mac_addr_len);
}

uint32_t RadioReceive::receive(const char* buf, int len, 
            uint8_t *sourceMacAddr, int& macAdrLen) {
    int socket = -1;
    if (!buf || !sourceMacAddr ||
            macAdrLen < CV2X_MAC_ADDR_LEN) {
        cerr << "Invalid input params" << endl;
        return -1;
    }
    // check if this receive is for simulation and/or for UDP
    if(isSim) {
        socket = simListenSock;
    }else {
        socket = this->gRxSub->getSock();
    }

    struct pollfd fd;
    int ret;
    fd.fd = socket;
    fd.events = POLLIN;
    fd.revents = 0;
    ret = poll(&fd, 1, 100); // 100 milisec timeout
    // timed out or had error receiving
    if(ret <= 0)
    {
        if(rVerbosity && ret < 0){
            fprintf(stderr, "%s\n", strerror(errno));
        }
        return ret;
    }

    uint32_t srcAddressSize = sizeof(this->srcAddress);
    uint32_t bytesReceived;
    int returnVal;
    struct sockaddr_in6 from;
    socklen_t fromLen = sizeof(from);
    struct msghdr message = {0};
    char control[CMSG_SPACE(sizeof(int))];
    struct iovec iov[1] = {0};
    iov[0].iov_base = (char*)buf;
    iov[0].iov_len = len;
    message.msg_name = &from;
    message.msg_namelen = fromLen;
    message.msg_iov = iov;
    message.msg_iovlen = 1;
    message.msg_control = control;
    message.msg_controllen = sizeof(control);

    if(this->enableUdp || !isSim){ // udp connection over eth or radio
        bytesReceived = recvmsg(socket, &message, 0);
    }
    else{ //tcp only
        bytesReceived = recv(socket, (char*) buf, len, 0);
    }

    if(bytesReceived > 0){
        sourceMacAddr[0] = 0;
        sourceMacAddr[1] = 0;
        sourceMacAddr[2] = 0;
        sourceMacAddr[3] = from.sin6_addr.s6_addr[13];
        sourceMacAddr[4] = from.sin6_addr.s6_addr[14];
        sourceMacAddr[5] = from.sin6_addr.s6_addr[15];
        gRxCount++;
        if(rVerbosity){
            cout << "#" << gRxCount << " Source MAC: ";
            for (int i = 0; i < CV2X_MAC_ADDR_LEN; i++) {
                cout << std::hex << static_cast<int>(sourceMacAddr[i]) << " ";
            }
            cout << endl;
        }
    }else{
        if(rVerbosity){
            cerr << "Invalid message" << std::endl;
        }
        return -1;
    }

    return bytesReceived;
}

uint8_t RadioReceive::closeFlow(){
    if (isSim)
    {
        const auto ans = close(simListenSock);
        const auto rxAns = close(simRxSock);
        if (ans >= 0 and rxAns >= 0)
        {
            cout << "Simulation Receives socket closed succesfully.\n";
        }
        else {
            cerr << "Problem closing Simulation Sockets in RadioReceive.\n";
        }

    }

    if(rVerbosity) printf("Attempting to close wra-related flows\n");
    clearGlobalIPInfo();

    if (this->gRxSub) {
        auto resp = -1;
        auto cv2xRadio = this->cv2xRadioManager->getCv2xRadio(this->category);
        auto respCb = [&](std::shared_ptr<ICv2xRxSubscription> rxSub,
                                ErrorCode error){
                                    rxSubCallback(rxSub, error);
                                };
        if (Status::SUCCESS == cv2xRadio->closeRxSubscription(this->gRxSub, respCb)){
            if (ErrorCode::SUCCESS == gCallbackPromise.get_future().get())
            {
                resp = static_cast<uint8_t>(Status::SUCCESS);
            }else{
                resp = static_cast<uint8_t>(Status::FAILED);
            }
        }else{
            resp = static_cast<uint8_t>(Status::FAILED);
        }
        this->resetCallbackPromise();
        this->gRxSub = nullptr;
        cout << "Rx flow closed.\n";
        return resp;
    }
    return 0;
}

/* set the Global IP addres prefix */
int RadioReceive::setGlobalIPInfo(const telux::cv2x::IPv6AddrType &ipv6Addr,
    const uint32_t serviceId)
{
    int ret = 0;

    SocketInfo tcpInfo;
    EventFlowInfo eventInfo;
    auto cv2xRadio = this->cv2xRadioManager->getCv2xRadio(this->category);
    auto respCb = [&](ErrorCode error){
                commonStatusCallback(error);
        };
    if (Status::SUCCESS == cv2xRadio->setGlobalIPInfo(ipv6Addr, respCb))
    {
        ErrorCode error =  this->gCallbackPromise.get_future().get();
        if (ErrorCode::SUCCESS == error)
        {
            cout<<"setGlobalIPInfo succeeds." << endl;;
            ret = 0;
        }else{
            if(rVerbosity)
                cerr<<"setGlobalIPInfo fails:" << static_cast<int>(error) << endl;;
            ret = -1;
        }
    } else{
        if(rVerbosity)
            cerr<<"setGlobalIPInfo sync fails." << endl;
        ret = -1;
    }
    this->resetCallbackPromise();

    /* Create IP unicast flow on port 0 */
    tcpInfo.serviceId = serviceId;
    tcpInfo.localPort = 0;
    eventInfo.isUnicast = true;
    auto sockRespCb = [&](shared_ptr<ICv2xTxRxSocket> sock, ErrorCode error){
                createTcpSocketCallback(sock, error);
        };
    if (Status::SUCCESS == 
        cv2xRadio->createCv2xTcpSocket(eventInfo, tcpInfo, sockRespCb))
    {
        auto error = this->gCallbackPromise.get_future().get();
        if (ErrorCode::SUCCESS == error)
        {
            if(rVerbosity)
                cout<<"createCv2xTcpSocket succeeds." << endl;;
            ret = 0;
        }else{
            if(rVerbosity)
                cerr<<"createCv2xTcpSocket fails: ." << 
                    static_cast<int>(error) << endl;;
            ret = -1;
        }
    } else {
        if(rVerbosity)
            cerr << "createCv2xTcpSocket sync fails" << endl;
        ret = -1;
    }

    this->resetCallbackPromise();
    if(rVerbosity)
        cout << "Global IP Info Set" << endl;

    return ret;
}

//For RSU use case, clear Global IP info and unregister catch all flow
int RadioReceive::clearGlobalIPInfo(void)
{
    if (not this->tcpSockInfo) {
        return 0;
    }

    int ret = 0;
    telux::cv2x::IPv6AddrType ipv6Prefix;
    auto cv2xRadio = this->cv2xRadioManager->getCv2xRadio(this->category);
    auto closeSockCb = [&](shared_ptr<ICv2xTxRxSocket> sock, ErrorCode error) {
        closeTcpSocketCallback(sock, error);
    };
    resetCallbackPromise();
    if (Status::SUCCESS !=
        cv2xRadio->closeCv2xTcpSocket(tcpSockInfo, closeSockCb) ||
        ErrorCode::SUCCESS != gCallbackPromise.get_future().get()) {
        if(rVerbosity)
            cerr << "close tcp socket err or already closed" << endl;
        ret = -1;
    } else {
        this->tcpSockInfo = nullptr;
    }

    resetCallbackPromise();
    if (ret)
        return ret;

    ipv6Prefix.prefixLen = 64;
    memset(&ipv6Prefix.ipv6Addr[0], 0, CV2X_IPV6_ADDR_ARRAY_LEN);
    auto respCb = [&](ErrorCode error) {
        commonStatusCallback(error);
    };
    if (Status::SUCCESS == cv2xRadio->setGlobalIPInfo(ipv6Prefix, respCb))
    {
        if (ErrorCode::SUCCESS == this->gCallbackPromise.get_future().get())
        {
            if(rVerbosity)
                cout<<"setGlobalIPInfo succeeds." << endl;;
            ret = 0;
        }else{
            if(rVerbosity)
                cerr<<"setGlobalIPInfo fails." << endl;;
            ret = -1;
        }
    } else{
        if(rVerbosity)
            cout<<"setGlobalIPInfo sync fails." << endl;
        ret = -1;
    }

    resetCallbackPromise();
    if(rVerbosity)
        cout << "Global IP session stopped" << endl;

    return ret;
}

int RadioReceive::setRoutingInfo(const telux::cv2x::GlobalIPUnicastRoutingInfo &destL2Addr)
{
    int ret = 0;
    auto cv2xRadio = this->cv2xRadioManager->getCv2xRadio(this->category);
    auto respCb = [&](ErrorCode error){
                commonStatusCallback(error);
        };

    if (Status::SUCCESS == cv2xRadio->setGlobalIPUnicastRoutingInfo(destL2Addr, respCb))
    {
        if (ErrorCode::SUCCESS == this->gCallbackPromise.get_future().get())
        {
            ret = 0;
            if(rVerbosity)
                cout<<"setGlobalIPUnicastRoutingInfo succeeds." << endl;;
        }else{
            if(rVerbosity)
                cerr<<"setGlobalIPUnicastRoutingInfo fails." << endl;;
            ret = -1;
        }
    }else{
        if(rVerbosity)
            cerr<< "setGlobalIPUnicastRoutingInfo sync fails." << endl;
        ret = -1;
    }
    this->resetCallbackPromise();

    return ret;
}

int RadioReceive::onWraTimedout(void)
{
    return clearGlobalIPInfo();
}
