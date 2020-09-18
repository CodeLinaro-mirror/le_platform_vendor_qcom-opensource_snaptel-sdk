/*
 *  Copyright (c) 2019-2020, The Linux Foundation. All rights reserved.
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
  * @file: EtsiApplication.hpp
  *
  * @brief: class for ITS stack - ETSI
  */
#include "ApplicationBase.hpp"
#include "GeoNetRouterImpl.hpp"
using namespace gn;

class EtsiApplication : public ApplicationBase {
public:
    EtsiApplication(char *fileConfiguration);
    EtsiApplication(const string txIpv4, const uint16_t txPort,
        const string rxIpv4, const uint16_t rxPort, char* fileConfiguration);

    ~EtsiApplication() {
        GnRouter->Stop();
    }
    void fillMsg(std::shared_ptr<msg_contents> mc);
    // overload to support GeoNetwork
    void transmit(uint8_t index, std::shared_ptr<msg_contents>mc, int16_t bufLen,
            TransmitType txType);
    int receive(const uint8_t index, const uint16_t bufLen);

private:
    void initMsg(std::shared_ptr<msg_contents> mc);
    void freeMsg(std::shared_ptr<msg_contents> mc);
    void fillBtp(btp_data_t *btp);
    void fillCam(CAM_t *cam);
    void fillCamLocation(CAM_t *cam);
    void fillCamCan(CAM_t *cam);
    std::unique_ptr<GeoNetRouterImpl> GnRouter;
};
