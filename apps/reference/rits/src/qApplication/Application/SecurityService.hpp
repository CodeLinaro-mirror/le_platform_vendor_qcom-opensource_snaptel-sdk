/*
 *  Copyright (c) 2019-2021, The Linux Foundation. All rights reserved.
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
 *
 * Changes from Qualcomm Innovation Center are provided under the following license:
 *
 * Copyright (c) 2022, 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

 /**
  * @file: SecurityService.hpp
  *
  * @brief: Base class for security service
  */
#ifndef SECURITYSERVICE_HPP_
#define SECURITYSERVICE_HPP_
#include <cstdint>
#include <string>
#include <semaphore.h>

const uint8_t NO_KEY_GEN=0;
const uint8_t ASYMMETRIC_KEY_GEN=1;
const uint8_t SYMMETRIC_KEY_GEN=2;
const uint8_t IMPORT_SYMMETRIC_KEY=3;

struct VerifStats
{
    double timestamp;
    double verifLatency;
};

struct SignStats
{
    double timestamp;
    double signLatency;
};

struct MisbehaviorStats{
    double timestamp = 0.0;
    double misbehaviorLatency = 0.0;
};

struct Kinematics
{
    int32_t latitude;
    int32_t longitude;
    uint16_t elevation;
    uint32_t id;
    uint32_t dataType;
    uint8_t msgCount;
    int16_t speed;
    uint16_t heading;
    int16_t longitudeAcceleration;
    int16_t latitudeAcceleration;
    int32_t yawAcceleration;
    uint16_t brakes;
};

/**
 * Security options when invoking signing or verification operation.
 */
typedef struct SecurityOpt {
    uint32_t psidValue;
    uint8_t sspValue [31];
    uint32_t sspLength = 0;
    uint8_t externalDataHash[31];
    char *bitMapToken = NULL;
    char *sspMaskTokens = NULL;
    char *sspTokens = NULL;
    Kinematics hvKine;
    Kinematics rvKine;
    bool enableAsync;
    uint32_t sspMaskValue [31];
    uint32_t sspMaskLength = 0;
    bool enableEnc;
    bool enableMbd = false;
    uint8_t secVerbosity;
    VerifStats* verifStat;
    SignStats* signStat;
    MisbehaviorStats* misbehaviorStat;
} SecurityOpt_t;

/*
 * Fields related to ID change operation
 */
typedef struct IDChangeData {
    unsigned char tempId [4]; // 32 bits
    unsigned char certId [8]; // last 8 bytes of cert id
    bool idChanged;
    sem_t idSem;
} IDChangeData_t;

class SecurityService {
public:
    SecurityService(const std::string ctxName, uint16_t countryCode):
        SecurityCtxName_(ctxName),
        countryCode_(countryCode) {
        }

    virtual int ExtractMsg(const SecurityOpt opt,
                            const uint8_t * msg,
                            uint32_t msgLen,
                            uint8_t const *payload,
                            uint32_t       payloadLen,
                            uint32_t       &dot2HdrLen) = 0;

    /**
    * Method to setup and sign packets based on the provided parameters and config.
    * Needs to be implemented.
    * @param opt - Struct that contains security-related information
    * @param msgLen - Length of the message that will be signed
    * @param signedSpdu - Pointer to the signed packet
    * @param signedSpduLen - Size of the signed packet
    * @return int - A non-negative integer value upon success or -1 on failure
    */
    virtual int SignMsg(const SecurityOpt opt, const uint8_t *msg, uint32_t msgLen,
                                uint8_t *signedSpdu, uint32_t &signedSpduLen) = 0;
    /**
    * Method to setup and verify packets based on the provided parameters and config.
    * Needs to be implemented.
    * @param opt - Struct that contains security-related information
    * @return int - A non-negative integer value upon success or -1 on failure
    */
    virtual int VerifyMsg(const SecurityOpt opt) = 0;
    /**
    * Method to alert Aerolink to initiate and complete a cert/id change.
    * The application should update the rest of the related parameters such
    * as temp id and L2 address.
    * @param
    * @return int - Reports -1 on failure, else success.
    */
    virtual int idChange() = 0;

    /**
    * Destructor for SecurityService
    */
    virtual ~SecurityService(){};
protected:
    /**
    * Virtual method to setup and initialize security instance.
    * Needs to be implemented.
    * @return int - Integer value representing success or not.
    */
    virtual int init(void) = 0;

    /**
    * Virtual method to deinitialize security instance.
    * Needs to be implemented.
    */
    virtual void deinit(void) = 0;
    std::string SecurityCtxName_;
    uint16_t countryCode_;
};
#endif
