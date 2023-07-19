/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef RESPONSEHANDLER_HPP
#define RESPONSEHANDLER_HPP

#include <telux/common/CommonDefines.hpp>
#include <jsoncpp/json/json.h>
#include "AsyncTaskQueue.hpp"

namespace telux {
namespace common {

class ResponseHandler {
    public:
        ResponseHandler(Json::Value& obj);
        virtual ~ResponseHandler();

        /*
        * This API could be used for invoking InitResponseCb.
        * For this API the rootObj will be the json object that contains
        * managers/subsystems.
        */
        telux::common::Status initResponseHandler(std::string subsystem,
            telux::common::InitResponseCb callback);

        /*
        * This API could be used for invoking ResponseCallback.
        * For this API the object passed shall be the json object that contains
        * canned responses for the public headers.
        * For Example: consider the below json having two managers/subsystems, the managers
        * further contains the canned responses for public headers like setDefaultProfile,
        * getDefaultProfile, etc. In this case passed object will be as shown below.
        *
        *               {
        *  Root Object -->   "IDataConnectionManager" :
        *                    {
        *                        "isSubsystemReady":true,
        *                        "isSubsystemReadyDelay":200,
        *  publicInterface -->   "setDefaultProfile":
        *                        {
        *                            "Status":"SUCCESS",
        *                            "callbackDelay":400,
        *                            "error":"SUCCESS"
        *                        },
        *                    },
        *  Root Object -->   "IDataProfileManager":
        *                    {
        *                        "isSubsystemReady":true,
        *                        "isSubsystemReadyDelay":200,
        *                        "createProfile":"SUCCESS",
        *                        "deleteProfile":"SUCCESS",
        *                    }
        *                }
        */
        telux::common::Status asyncResponseHandler(Json::Value& rootObj,
             std::string publicInterface, telux::common::ResponseCallback callback);

        telux::common::ServiceStatus getServiceStatus(std::string subsystem);
        std::future<bool> onSubSystemReady(std::string subsystem);
        bool isSubSystemReady(std::string subsystem);

    private:
        Json::Value rootObj_;
        std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;

        void invokeInitResponseCallback(int cbDelay, int cbStatus,
            telux::common::InitResponseCb callback);
        void invokeResponseCallback(int cbDelay, int cbErrorCode,
            telux::common::ResponseCallback callback);
        void mapServiceStatus(std::string subsystem, ServiceStatus &status);
};

}
}

#endif //RESPONSEHANDLER_HPP