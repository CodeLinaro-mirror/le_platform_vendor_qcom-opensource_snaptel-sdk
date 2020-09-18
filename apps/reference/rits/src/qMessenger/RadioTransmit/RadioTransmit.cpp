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
  * @file: RadioTransmit.cpp
  *
  * @brief: Implementation of RadioTransmit
  *
  */

#include "RadioTransmit.h"

RadioTransmit::RadioTransmit(const SpsFlowInfo spsInfo, const TrafficCategory category,
                const TrafficIpType trafficType, const uint16_t port, const uint32_t serviceId,
                const bool withEventFlow, const uint16_t eventFlowPort){

    if (!ready(category, RadioType::TX)) {
        cout << "Radio Checks on Sps Transmit Event Fail\n";
        //return static_cast<uint8_t>(Status::FAILED);
    }
    auto cv2xRadio = cv2xRadioManager->getCv2xRadio(category);
    auto respCallback = [&](std::shared_ptr<ICv2xTxFlow> txSpsFlow,
                            std::shared_ptr<ICv2xTxFlow> txEventFlow,
                            ErrorCode spsError, ErrorCode eventError){
                                spsFlowCallbackOnCreate(txSpsFlow,txEventFlow,spsError,eventError);
                            };
    if(Status::SUCCESS == cv2xRadio->createTxSpsFlow(trafficType, serviceId, spsInfo,
                port, withEventFlow, eventFlowPort, respCallback)){
        if(ErrorCode::SUCCESS == gCallbackPromise.get_future().get()){
            cout<<"Sps flow created succesfully\n";
            //return static_cast<uint8_t>(Status::SUCCESS);
        }
        else{
            cout<<"Sps Flow creation fails\n";
            //return static_cast<uint8_t>(Status::FAILED);
        }
    }
    else {
        cout << "Sps Flow creation fails\n";
        //return static_cast<uint8_t>(Status::FAILED);
    }
    this->resetCallbackPromise();
}

RadioTransmit::RadioTransmit(const EventFlowInfo eventInfo, const TrafficCategory category,
                            const TrafficIpType trafficType, const uint16_t port,
                            const uint32_t serviceId){
    if (!this->ready(category, RadioType::TX)) {
        cout << "Radio Checks on Transmit Event fail\n";
        //return static_cast<uint8_t>(Status::FAILED);;
    }
    this->category = category;
    auto cv2xRadio = this->cv2xRadioManager->getCv2xRadio(category);
    auto respCallback = [&](std::shared_ptr<ICv2xTxFlow> txEventFlow,
                            ErrorCode eventError){
                                eventFlowCallbackOnCreate(txEventFlow,eventError);
                            };
    if(Status::SUCCESS == cv2xRadio->createTxEventFlow(trafficType, serviceId, eventInfo,
                port, respCallback)){
        if(ErrorCode::SUCCESS == this->gCallbackPromise.get_future().get()){
            cout<<"Event Flow created succesfully\n";
            //return static_cast<uint8_t>(Status::SUCCESS);
        }else{
            cout<<"Event Flow creation fails, future.get\n";
            //return static_cast<uint8_t>(Status::FAILED);
        }
    }else{
            cout<<"Event Flow creation fails\n";
            //return static_cast<uint8_t>(Status::FAILED);
    }
    this->resetCallbackPromise();

}

RadioTransmit::RadioTransmit(const RadioOpt radioOpt, const string ipv4_dst, const uint16_t port) {
    cout << "Now simulating transmission of messages..."<< endl;
    isSim = true;
    struct sockaddr_in addr;
    this->enableUdp = radioOpt.enableUdp;
    this->ipv4_src = radioOpt.ipv4_src;
    if(!this->enableUdp) { // not udp
        this->simSock = socket(AF_INET, SOCK_STREAM, 0);
    } else { // udp
        this->simSock = socket(AF_INET, SOCK_DGRAM, 0);
    }
    if (simSock < 0)
    {
        cout << "Error Creating Socket";
    }
    else {
        if(!this->enableUdp){ // tcp
            addr.sin_family = AF_INET;
            addr.sin_port = htons(port);
            if (inet_pton(AF_INET, ipv4_dst.data(), &addr.sin_addr) <= 0) {
                cout << "Invalid ip address: " << ipv4_dst << endl;
            }
            const auto creation = connect(simSock, (struct sockaddr*) & addr, sizeof(addr));
            if (creation < 0)
            {
                cout << "Connection failed with port: " << port << " ip: " << ipv4_dst << endl;
            }
        }else{ //udp
            this->destAddress.sin_family = AF_INET;
            this->destAddress.sin_port = htons(port);
            if (inet_pton(AF_INET, ipv4_dst.data(), &(this->destAddress.sin_addr)) <= 0) {
                cout << "Invalid ip address: " << ipv4_dst << endl;
            }

            this->clientAddress.sin_family = AF_INET;
            this->clientAddress.sin_port = htons(port);
            if(inet_pton(AF_INET, this->ipv4_src.data(), &(this->clientAddress.sin_addr)) <= 0) {
                cout << "Invalid ip address for client: " << ipv4_src << endl;
            }
        }
    }
}



void RadioTransmit::configureIpv6(const uint16_t port, const char* destAddress, const char* iface) {
    this->destSock.sin6_family = AF_INET6;
    this->destSock.sin6_port = htons((uint16_t)port);
    inet_pton(AF_INET6, destAddress, (void*) &this->destSock.sin6_addr);
    this->destSock.sin6_scope_id = if_nametoindex(iface);
    //this->destSock.sin6_flowinfo missing...
}

uint8_t RadioTransmit::transmit(const char* buf, const uint16_t bufLen) {
    if (isSim)
    {
        uint8_t  bytes_sent;
        if(enableUdp){ //udp
            cout << "\nTransmitting data via udp...\n";
            bytes_sent = sendto(this->simSock, buf, bufLen,  0,
                        (const struct sockaddr *) &(this->destAddress), sizeof(this->destAddress));
        } else{ //tcp - default
            bytes_sent = send(simSock, buf, bufLen, 0);
        }
        return bytes_sent;
    }

    auto resp = -1;
    cout << "Sending data in Flow: len=" << bufLen << endl;
    auto sock = this->flow->getSock();

    if (sock == -1) {
        cout << "Error on transmit, with socket value -1\n";
        resp = static_cast<uint8_t>(Status::FAILED);
    }

    struct msghdr message = { 0 };
    struct iovec iov[1] = { 0 };
    struct cmsghdr* cmsghp = NULL;
    char control[CMSG_SPACE(sizeof(int))];

    //IPV6_TCLASS internal configuration
    iov[0].iov_base = (char*)buf;
    iov[0].iov_len = bufLen;
    message.msg_name = &this->destSock;
    message.msg_namelen = sizeof(this->destSock);
    message.msg_iov = iov;
    message.msg_iovlen = 1;
    message.msg_control = control;
    message.msg_controllen = sizeof(control);

    cmsghp = CMSG_FIRSTHDR(&message);
    cmsghp->cmsg_level = IPPROTO_IPV6;
    cmsghp->cmsg_type = IPV6_TCLASS;
    cmsghp->cmsg_len = CMSG_LEN(sizeof(int));

    //auto bytes_sent = sendmsg(sock, &message, 0);
    auto bytes_sent = send(sock, buf, bufLen, 0);
    if(bytes_sent == bufLen){
        resp = static_cast<uint8_t>(Status::SUCCESS);

#if 1
        printf("RadioTransmit::transmit\n");
        for (int i = 0; i < bufLen; i++) {
            printf("%02x ", *(buf + i));

        }
        printf("\n");
#endif
    }else{
        cout << "Error Sending Data.\n";
        resp = static_cast<uint8_t>(Status::FAILED);
    }

    return resp;
}



void RadioTransmit::spsFlowCallbackOnCreate(shared_ptr<ICv2xTxFlow> txSpsFlow,
    shared_ptr<ICv2xTxFlow> unusedFlow,ErrorCode spsError,ErrorCode unusedError) {
    if (ErrorCode::SUCCESS == spsError) {
        this->flow = txSpsFlow;
    }
    this->gCallbackPromise.set_value(spsError);
}

void RadioTransmit::eventFlowCallbackOnCreate(
    shared_ptr<ICv2xTxFlow> txEventFlow,
    ErrorCode eventError) {
    if (ErrorCode::SUCCESS == eventError) {
        this->flow = txEventFlow;
    }
    std::cout << "callback error=" << static_cast<int>(eventError) << std::endl;
    this->gCallbackPromise.set_value(eventError);
}

void RadioTransmit::spsFlowCallbackOnChanges(shared_ptr<ICv2xTxFlow> txEventFlow,
        ErrorCode eventError) {
    if (ErrorCode::SUCCESS == eventError) {
        this->flow = txEventFlow;
    }
    this->gCallbackPromise.set_value(eventError);
}


uint8_t RadioTransmit::updteSpsFlow(const SpsFlowInfo spsInfo) {
    auto resp = -1;
    auto cv2xRadio = this->cv2xRadioManager->getCv2xRadio(this->category);
    auto respCallback = [&](std::shared_ptr<ICv2xTxFlow> txSpsFlow,
                            ErrorCode spsError){
                                spsFlowCallbackOnChanges(txSpsFlow, spsError);
                            };
    if(Status::SUCCESS == cv2xRadio->changeSpsFlowInfo(this->flow, spsInfo, respCallback)){
        if(ErrorCode::SUCCESS == this->gCallbackPromise.get_future().get()){
            resp = static_cast<uint8_t>(Status::SUCCESS);
        }else{
            resp =  static_cast<uint8_t>(Status::FAILED);
        }
    }else{
        resp =  static_cast<uint8_t>(Status::FAILED);
    }
    this->resetCallbackPromise();
    return resp;
}


void RadioTransmit::closeCallback(shared_ptr<ICv2xTxFlow> flow, ErrorCode error) {
    this->gCallbackPromise.set_value(error);
}

uint8_t RadioTransmit::closeFlow() {

    if (isSim)
    {
        const auto ans = close(simSock);
        if (ans < 0)
        {
            cout << "Simulation socket failed to close.\n";
            return ans;
        }
        else {
            cout << "Simulation socket closed succesfully.\n";
            return ans;
        }
    }
    auto resp = -1;
    auto cv2xRadio = this->cv2xRadioManager->getCv2xRadio(this->category);
    auto respCallback = [&](std::shared_ptr<ICv2xTxFlow> flow,
                            ErrorCode eventError){
                                closeCallback(flow, eventError);
                            };
    if(Status::SUCCESS == cv2xRadio->closeTxFlow(this->flow, respCallback)){
        if (ErrorCode::SUCCESS == this->gCallbackPromise.get_future().get()){
            resp = static_cast<uint8_t>(Status::SUCCESS);
        }
        else{
            resp = static_cast<uint8_t>(Status::FAILED);
        }
    }else{
            resp = static_cast<uint8_t>(Status::FAILED);
    }
    this->resetCallbackPromise();
    this->flow = nullptr;
    cout << "Flow closed.\n";
    return resp;
}



