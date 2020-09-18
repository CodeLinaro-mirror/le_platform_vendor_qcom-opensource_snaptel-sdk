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
  * @file: RadioReceive.cpp
  *
  * @brief: Implementation of RadioReceive
  *
  */

#include "RadioReceive.h"

void RadioReceive::rxSubCallback(shared_ptr<ICv2xRxSubscription> rxSub, ErrorCode error) {
    if (ErrorCode::SUCCESS == error) {
        this->gRxSub = rxSub;
    }
    this->gCallbackPromise.set_value(error);
};

RadioReceive::RadioReceive(const TrafficCategory category, const TrafficIpType trafficIpType,
const uint16_t port){

    if (!this->ready(category, RadioType::RX)) {
        cout << "Radio Checks on RadioReceive creation fail\n";
        //return static_cast<uint8_t>(Status::FAILED);
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
            //return -static_cast<uint8_t>(Status::FAILED);
        }else{
            cout<<"Rx Subscription creation fails.\n";
            //return static_cast<uint8_t>(Status::FAILED);
        }
    }else{
            cout<<"Rx Subscription creation fails.\n";
            //return static_cast<uint8_t>(Status::FAILED);
    }
    this->resetCallbackPromise();
}

uint32_t RadioReceive::receive(const char* buf) {
    int socket = -1;
    // check if this receive is for simulation and/or for UDP
    if(isSim) {
        cout << "Setting up simulation receive socket\n";
        socket = simListenSock;
    }else {
        socket = this->gRxSub->getSock();
    }

    uint32_t srcAddressSize = sizeof(this->srcAddress);
    uint32_t bytesReceived;
    int returnVal;
    if(isSim && this->enableUdp){ //udp
        returnVal  = recvfrom(socket, (char*) buf, RadioReceive::MAX_BUF_LEN, 0,
            (struct sockaddr *) & (this->srcAddress), & srcAddressSize);
        if(returnVal != -1 && returnVal != 0)
            bytesReceived = returnVal;
        else
            cout << "Errno is: "<< errno <<"\n";
    }else{ //tcp
         bytesReceived = recv(socket, (char*) buf, RadioReceive::MAX_BUF_LEN, 0);
    }
    if (returnVal  <= 0 && this->enableUdp) {
        cout << "Radio Receive error in receive. Return value is: " << returnVal << "\n";
        return returnVal;
    }else{
        cout << "Number of bytes received is: "  << bytesReceived << "\n";
        return bytesReceived;
    }
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
            cout << "Problem closing Simulation Sockets in RadioReceive.\n";
        }

    }
    auto cv2xRadio = this->cv2xRadioManager->getCv2xRadio(this->category);
    auto respCb = [&](std::shared_ptr<ICv2xRxSubscription> rxSub,
                            ErrorCode error){
                                rxSubCallback(rxSub, error);
                            };
    if (Status::SUCCESS == cv2xRadio->closeRxSubscription(this->gRxSub, respCb)){
        if (ErrorCode::SUCCESS == gCallbackPromise.get_future().get())
        {
            return static_cast<uint8_t>(Status::SUCCESS);
        }else{
            return static_cast<uint8_t>(Status::FAILED);
        }
    }else{
        return static_cast<uint8_t>(Status::FAILED);
    }
    this->resetCallbackPromise();
}


RadioReceive::RadioReceive(RadioOpt radioOpt, const string ipv4_dst, const uint16_t port) {
    struct sockaddr_in address;
    isSim = true;
    this->enableUdp = radioOpt.enableUdp;
    this->ipv4_src = radioOpt.ipv4_src;
    // Creating socket file descriptor

    if (!this->enableUdp) {
        this->simListenSock = socket(AF_INET, SOCK_STREAM, 0);
    } else {
        this->simListenSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        address = this->srcAddress;
    }

    if (this->simListenSock <= 0)
    {
        cout << "Error Creating Socket";
    }

    if(!this->enableUdp){
        address.sin_family = AF_INET;
        // convert values between host and network byte order
        address.sin_port = htons(port);
        if (ipv4_src.compare("*") == 0)
        {
            address.sin_addr.s_addr = INADDR_ANY;
        }
        else {
            if (inet_pton(AF_INET, ipv4_dst.data(), &address.sin_addr) <= 0) {
                cout << "TCP; simulation:: Invalid ip address: " << ipv4_dst << endl;
            }
            else {
                if (bind(simListenSock, (struct sockaddr*) & address,
                    sizeof(address)) < 0)
                {
                    cout << "Socket " << simListenSock << " with IP: " << ipv4_dst << " and port: " << endl;
                    cout << port << " failed binding" << endl;
                }
                else {
                    if (listen(simListenSock, 1) < 0) {
                        cout << "Socket fails to listen\n";
                    }
                    else {
                        const auto len = sizeof(address);
                        // accept is only for tcp, probably need to make this for udp as well
                            simRxSock = accept(simListenSock, (struct sockaddr*) & address,
                                    (socklen_t*)& len);
                        cout << "Connection Received";
                    }
                }
            }
        }

    }else{
        this->srcAddress.sin_family = AF_INET;
        this->srcAddress.sin_port = htons(port);
        if(inet_pton(AF_INET, ipv4_dst.data(), &(this->srcAddress.sin_addr)) <= 0){
            cout << "Invalid ip address for other device: " << ipv4_dst << endl;
        }
        // Can set to listening to anybody instead
        //this->srcAddress.sin_addr.s_addr = htonl(INADDR_ANY);

        this->serverAddress.sin_family = AF_INET;
        this->serverAddress.sin_port = htons(port);

        if(inet_pton(AF_INET, ipv4_src.data(), &(this->serverAddress.sin_addr)) <= 0){
            cout << "Invalid ip address for this device: " << ipv4_src << endl;
        }

        // bind to socket
        if (bind(this->simListenSock, (struct sockaddr *) &this->serverAddress,
               sizeof(this->serverAddress)) < 0){
            cout << "ERROR on UDP binding" << endl;
        }else{
            cout << "UDP bind successful" << endl;
        }
    }
}
