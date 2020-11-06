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

#include <iostream>
#include <algorithm>
#include <iterator>

#include <telux/tel/PhoneFactory.hpp>

#include "HttpTransactionListener.hpp"
#include "Log.hpp"

#define PRINT_CB std::cout << "\033[1;35mCALLBACK: \033[0m"
#define PRINT_NOTIFICATION std::cout << "\033[1;35mNOTIFICATION: \033[0m"
#define UIM_HTTP_RESULT_SUCCESS 0

HttpTransactionListener::HttpTransactionListener(
    std::shared_ptr<telux::tel::IHttpTransactionManager> httpMgr)
   : httpTransactionManager_(httpMgr) {
}

void HttpTransactionListener::onSendHttpTransactionResp(telux::common::ErrorCode errCode) {
    LOGI(" CALLBACK: onSendHttpTransactionResp code: %d \n", static_cast<int>(errCode));
}

void HttpTransactionListener::onNewHttpRequest(const std::string &url, uint32_t tokenId,
    const std::vector<telux::tel::CustomHeader> &headers, const std::vector<uint8_t> &reqPayload) {

    telux::tel::HttpResult httpResult = telux::tel::HttpResult::TRANSACTION_SUCCESSFUL;
    LOGI(" NOTIFICATION: onNewHttpRequest \n");

    LOGD(" Http transaction Request \n");
    LOGD(" URL: %s \n", url.c_str());
    for (auto &h : headers) {
        LOGD(" Header: %s, Value: %s \n", h.name.c_str(), h.value.c_str());
    }

    std::string postResponse = "";

    std::string reqPayloadStr(reqPayload.begin(), reqPayload.end());
    auto curlCode = curlPost(url, reqPayloadStr, postResponse, headers);
    LOGD(" After Http POST payload Response: %s \n", postResponse.c_str());

    if (curlCode != CURLE_OK) {
        LOGE("Curl Post failed: %s \n", curl_easy_strerror(curlCode));
        httpResult = telux::tel::HttpResult::UNKNOWN_ERROR;
    }

    std::vector<uint8_t> httpResp;
    std::copy(postResponse.begin(), postResponse.end(), std::back_inserter(httpResp));

    LOGD(" After Http transaction Request \n");
    auto responseCb = [&](telux::common::ErrorCode errCode) { onSendHttpTransactionResp(errCode); };
    auto result = httpTransactionManager_->sendHttpTransactionResult(tokenId,
        httpResult, headers, httpResp, responseCb);
    if (result != telux::common::Status::SUCCESS) {
        LOGE(" Unable to send the sendHttpTransactionResult() request \n");
    }
}

CURLcode HttpTransactionListener::curlPost(const std::string &url,
    const std::string &postParameters, std::string &postResponse,
    const std::vector<telux::tel::CustomHeader> &headers) {
    LOGI(" curlPost \n");

    std::lock_guard<std::mutex> lock(mtx_);
    CURL *curl = curl_easy_init();  // CURL handle

    if (!curl) {
        LOGE("Curl init failed \n");
        return CURLE_FAILED_INIT ;
    }
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    setPostParams(url, postParameters, postResponse, headers, curl);

    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    // This tells the CURL to fail the request if the HTTP code returned is equal to
    // or larger than 400
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);

    auto curlCode = curl_easy_perform(curl);

    // Gather HTTP response code
    if(curlCode == CURLE_HTTP_RETURNED_ERROR) {
        long responseCode = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
        LOGI(" Http Response Code: \n", responseCode);
    }
    curl_easy_cleanup(curl);

    return curlCode;
}

void HttpTransactionListener::setPostParams(const std::string &url,
    const std::string &postParameters, std::string &postResponse,
    const std::vector<telux::tel::CustomHeader> &headers, CURL *curl) {
    // set URL
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

    // set content type as json
    struct curl_slist *curlHeaders = NULL;
    curlHeaders = curl_slist_append(curlHeaders, "Content-Type: application/json");
    std::string contentLength = "Content-Length: " + std::to_string(postParameters.length());
    curlHeaders = curl_slist_append(curlHeaders, contentLength.c_str());

    for (auto &header : headers) {
        std::string curlHeader = header.name + ": " + header.value;
        curlHeaders = curl_slist_append(curlHeaders, curlHeader.c_str());
    }

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, curlHeaders);

    // set callback to receive server response json
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &postResponse);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteCb);

    // set POST json payload
    if (!postParameters.empty()) {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postParameters.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, postParameters.length());
    }
}

size_t HttpTransactionListener::curlWriteCb(void *ptr, size_t size, size_t nmemb, void *userData) {
    (static_cast<std::string *>(userData))->append(static_cast<char *>(ptr), size * nmemb);
    return size * nmemb;
}