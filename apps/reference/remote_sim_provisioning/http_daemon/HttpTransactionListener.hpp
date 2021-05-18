/*
 *  Copyright (c) 2021, The Linux Foundation. All rights reserved.
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

#ifndef HTTPTRANSACTIONLISTENER_HPP
#define HTTPTRANSACTIONLISTENER_HPP

#include <curl/curl.h>

#include <mutex>

#include <telux/tel/HttpTransactionManager.hpp>

class HttpTransactionListener : public telux::tel::IHttpTransactionListener {
 public:
    HttpTransactionListener(std::shared_ptr<telux::tel::IHttpTransactionManager> httpMgr);
    void onNewHttpRequest(const std::string &url, uint32_t tokenId,
        const std::vector<telux::tel::CustomHeader> &headers,
        const std::vector<uint8_t> &reqPayload) override;

 private:
    std::mutex mtx_;
    std::shared_ptr<telux::tel::IHttpTransactionManager> httpTransactionManager_;

    void processHttpRequest(uint32_t tokenId, const std::string &url,
         const std::string &reqPayload, const std::vector<telux::tel::CustomHeader> &headers);

    CURLcode curlPost(const std::string &url, const std::string &postParameters,
        std::string &postResponse, const std::vector<telux::tel::CustomHeader> &headers);
    void setPostParams(const std::string &url, const std::string &postParameters,
        std::string &postResponse, const std::vector<telux::tel::CustomHeader> &headers,
        CURL *curl);

    // This callback function gets called by libcurl as soon as there is data received that needs
    // to be saved.
    static size_t curlWriteCb(void *ptr, size_t size, size_t nmemb, void *userData);

    void onSendHttpTransactionResp(telux::common::ErrorCode errCode);
};

#endif  // HTTPTRANSACTIONLISTENER_HPP
