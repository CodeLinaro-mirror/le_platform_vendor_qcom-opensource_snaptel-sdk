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
  * @file: SaeApplication.hpp
  *
  * @brief: class for ITS stack - SAE
  */
#include "ApplicationBase.hpp"

class SaeApplication : public ApplicationBase {
public:
    SaeApplication(char *fileConfiguration);
    SaeApplication(const string txIpv4, const uint16_t txPort,
        const string rxIpv4, const uint16_t rxPort, char* fileConfiguration);
    ~SaeApplication();

    void fillMsg(std::shared_ptr<msg_contents> msg);

    /**
    * Method that decodes bsm from raw buffer to bsm contents data structure in ldm.
    * It also handles the Tunc. This method should be deprecated once Encoding and Decoding
    * libraries can take the Tunc in the j2735 dictionary and protocol.
    * @param index - An uint8_t that gives which receive buffer to access.
    * @param bufLen -The length of given buffer.
    * @param ldmIndex -The index of the ldm bsm content to store decoded value.
    */
    void receiveTuncBsm(const uint8_t index, const uint16_t bufLen, const uint32_t ldmIndex);

    /**
    * Method to illustrate Tunnel Timing by adding Tunc.
    * This method should be deprecated once Decoding and Encoding libraries
    * for BSMs can actually take Tunc as part of the J2735 dictionary and protocol.
    * @param index - An uint8_t that shows all elements used form vectors.
    * @param type - The type of flow (event, sps) in which bsm will transmit.
    * reports in milliseconds. It can be interval or more.
    */
    void sendTuncBsm(uint8_t index, TransmitType txType);

private:
    void transmit(uint8_t index, std::shared_ptr<msg_contents>mc, int16_t bufLen,
            TransmitType txType);
    void initMsg(std::shared_ptr<msg_contents> mc);
    void freeMsg(std::shared_ptr<msg_contents> mc);
    void fillBsm(bsm_value_t *bsm);
    void fillBsmCan(bsm_value_t *bsm);
    void fillBsmLocation(bsm_value_t *bsm);
    void fillWsmp(wsmp_data_t *wsmp);
    //Pre-recorded file methods
    void initRecordedBsm(bsm_value_t* bsm);
};
