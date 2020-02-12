/*
 *  Copyright (c) 2018-2020 The Linux Foundation. All rights reserved.
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

#ifndef MYSERVINGSYSTEMHANDLER_HPP
#define MYSERVINGSYSTEMHANDLER_HPP

#include <memory>
#include <string>
#include <vector>

#include <telux/common/CommonDefines.hpp>
#include <telux/tel/ServingSystemManager.hpp>

class MyRatPreferenceResponseCallback {
public:
   static void ratPreferenceResponse(telux::tel::RatPreference preference,
                                     telux::common::ErrorCode error);
};

class MyServiceDomainResponseCallback {
public:
   static void serviceDomainResponse(telux::tel::ServiceDomainPreference preference,
                                     telux::common::ErrorCode error);
};

class MyServingSystemResponsecallback {
public:
   static void servingSystemResponse(telux::common::ErrorCode error);
};

class MyServingSystemHelper {
public:
   static std::string getRatPreference(telux::tel::RatPreference preference);
   static std::string getServiceDomain(telux::tel::ServiceDomainPreference preference);
   static std::string getEndcAvailability(telux::tel::EndcAvailability isAvailable);
   static std::string getDcnrRestriction(telux::tel::DcnrRestriction isRestricted);
};

class MyServingSystemListener : public telux::tel::IServingSystemListener {
public:
   void onRatPreferenceChanged(telux::tel::RatPreference preference) override;
   void onServiceDomainPreferenceChanged(telux::tel::ServiceDomainPreference preference) override;
   void onDcStatusChanged(telux::tel::DcStatus dcStatus) override;
};

#endif  // MYSERVINGSYSTEMHANDLER_HPP
