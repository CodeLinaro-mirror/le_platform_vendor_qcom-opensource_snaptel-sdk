/*
 *  Copyright (c) 2017, The Linux Foundation. All rights reserved.
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

#ifndef MYPHONELISTENER_HPP
#define MYPHONELISTENER_HPP

#include <vector>

#include <telux/tel/Phone.hpp>
#include <telux/tel/PhoneListener.hpp>
#include <telux/common/CommonDefines.hpp>

class MyPhoneListener : public telux::tel::IPhoneListener {
public:
   void onServiceStateChanged(std::shared_ptr<telux::tel::IPhone> phone,
                              telux::tel::ServiceState state) override;
   void onSignalStrengthChanged(std::shared_ptr<telux::tel::IPhone> phone,
                                std::shared_ptr<telux::tel::SignalStrength> signalStrength) override;
   std::string getCurrentTime();

   ~MyPhoneListener() {
   }
};

class MySignalStrengthCallback : public telux::tel::ISignalStrengthCallback {
public:
   MySignalStrengthCallback();
   void signalStrengthResponse(std::shared_ptr<telux::tel::SignalStrength> signalStrength,
                               telux::common::ErrorCode error) override;
};

#endif  // MYPHONELISTENER_HPP
