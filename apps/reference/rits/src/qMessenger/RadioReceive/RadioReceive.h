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
  * @file: RadioReceive.h
  *
  * @brief: Application that handles and abstracts CV2x SDK radio receiving
  *
  */

#pragma once

#include "RadioInterface.h"
#include <vector>
#include <ifaddrs.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <string>

using std::array;
using std::make_shared;
using telux::cv2x::ICv2xRxSubscription;

class RadioReceive : public RadioInterface {
private:
    shared_ptr<ICv2xRxSubscription> gRxSub;
    TrafficCategory category;
    void rxSubCallback(shared_ptr<ICv2xRxSubscription> rxSub, ErrorCode error);
    bool isSim = false;
    int simListenSock, simRxSock;
    struct sockaddr_in srcAddress;
    struct sockaddr_in serverAddress;
    uint16_t srcPort;
    bool enableUdp = false;
    string ipv4_src;

protected:

public:
    /**
    * Constant value of largest possible buffer length.
    */
    static constexpr uint32_t MAX_BUF_LEN = 3000;

    /**
    * Constructor that creates a RadioReceive Object
    * @param category a TrafficCategory.
    * @param type a TrafficType.
    */
    RadioReceive(const TrafficCategory category, const TrafficIpType trafficIpType,
    const uint16_t port);

    /**
    * Constructor for Simulation of Radio Receives.
    */
    //RadioReceive(const string ipv4, const uint16_t port);
    RadioReceive(const RadioOpt radioOpt, const string ipv4_dst, const uint16_t port);

    /**
    * Blocking mehtod that receives from created flow's socket.
    * @param buf a char pointer to store the data received.
    * @return bytes received, -1 if error.
    */
    uint32_t receive(const char* buf);


    /**
    * Method that closes Receive Subscription and returns fail or success
    * @param buf a char pointer to store the data received.
    * @return bytes received, -1 if error.
    */
    uint8_t closeFlow();

};

