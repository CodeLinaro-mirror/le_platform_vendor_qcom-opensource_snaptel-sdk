/*
 *  Copyright (c) 2018-2019, The Linux Foundation. All rights reserved.
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

#ifndef DATALISTENER_HPP
#define DATALISTENER_HPP

#include <mutex>
#include <map>

#include <telux/data/DataFactory.hpp>
#include <telux/data/DataConnectionManager.hpp>

class DataListener : public telux::data::IDataConnectionListener {
public:
   void onDataCallInfoChanged(const std::shared_ptr<telux::data::IDataCall> &dataCall) override;
   void onServiceStatusChange(telux::common::ServiceStatus status) override;
   std::shared_ptr<telux::data::IDataCall> getDataCall(int slotId, int profileId);
   void initDataCallListResponseCb(
       const std::vector<std::shared_ptr<telux::data::IDataCall>> &dataCallList,
       telux::common::ErrorCode error);

private:
   std::mutex mtx_;
   // Associate profileId, ipfamily type with data call impl
   std::multimap<int, std::shared_ptr<telux::data::IDataCall>> dataCallMap_;

   void updateDataCallMap(const std::shared_ptr<telux::data::IDataCall> &dataCall);
   void logDataCallDetails(const std::shared_ptr<telux::data::IDataCall> &dataCall);
};

#endif  // DATALISTENER_HPP
