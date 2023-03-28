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
/*
 *  Changes from Qualcomm Innovation Center are provided under the following license:
 *
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted (subject to the limitations in the
 *  disclaimer below) provided that the following conditions are met:
 *
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *
 *      * Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials provided
 *        with the distribution.
 *
 *      * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *        contributors may be used to endorse or promote products derived
 *        from this software without specific prior written permission.
 *
 *  NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 *  GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 *  HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 *  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 *  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 *  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
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
                                     (socklen_t*)& len);
                   cout << "Connection Received";
                   }
                }
            }

        } else{
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
        } else{
            // bind to socket
            if (bind(this->simListenSock, (struct sockaddr *) &this->serverAddress,
                     sizeof(this->serverAddress)) < 0){
                cerr << "ERROR on UDP binding" << endl;
                exit(0);
            } else{
                cout << "UDP bind successful" << endl;
            }
        }
    }
}

uint32_t RadioReceive::receive(const char* buf) {
    int socket = -1;
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
    ret = poll(&fd, 1, 1000); // 1sec timeout
    // timed out or had error receiving
    if(ret <= 0)
        return -1;

    uint32_t srcAddressSize = sizeof(this->srcAddress);
    uint32_t bytesReceived;
    int returnVal = 0;
    if(isSim && this->enableUdp){ //udp
        returnVal  = recvfrom(socket, (char*) buf, RadioReceive::MAX_BUF_LEN, 0,
            (struct sockaddr *) & (this->srcAddress), & srcAddressSize);
        if(returnVal != -1 && returnVal != 0)
            bytesReceived = returnVal;
    }else{ //tcp
         bytesReceived = recv(socket, (char*) buf, RadioReceive::MAX_BUF_LEN, 0);
    }
    if (returnVal  <= 0 && this->enableUdp) {
        return returnVal;
    }else{
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

