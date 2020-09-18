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
  * @file: SecurityService.hpp
  *
  * @brief: Base class for security service
  */
#ifndef SECURITYSERVICE_HPP_
#define SECURITYSERVICE_HPP_
#include <cstdint>
#include <string>

/**
 * Security options when invoking signing or verification operation.
 */
typedef struct SecurityOpt {
    uint32_t psidValue;
    uint8_t sspValue [32];
    uint32_t sspLength;
    uint8_t externalDataHash[32];
    int32_t latitude;
    int32_t longitude;
    uint16_t elevation;
    bool enableAsync;
} SecurityOpt_t;

class SecurityService {
public:
    SecurityService(const std::string ctxName, uint16_t countryCode):
        SecurityCtxName_(ctxName),
        countryCode_(countryCode) {
        }
    ~SecurityService(){
    }
    virtual int SignMsg(const SecurityOpt opt, const uint8_t *msg, uint32_t msgLen,
                                                    uint8_t *signedSpdu, uint32_t &signedSpduLen) = 0;
    virtual int VerifyMsg(const SecurityOpt opt, const uint8_t *msg, uint32_t msgLen, uint32_t &dot2HdrLen) = 0;

protected:
    virtual int init(void) = 0;
    virtual void deinit(void) = 0;
    std::string SecurityCtxName_;
    uint16_t countryCode_;
};
#endif
