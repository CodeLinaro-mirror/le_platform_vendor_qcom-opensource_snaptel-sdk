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

#ifndef MYLOCATIONCOMMANDCALLACK_HPP
#define MYLOCATIONCOMMANDCALLACK_HPP

#include "telux/common/CommonDefines.hpp"
#include "telux/loc/LocationDefines.hpp"

class MyLocationCommandCallback : public telux::common::ICommandResponseCallback {
public:
   MyLocationCommandCallback(std::string cmdName);
   void commandResponse(telux::common::ErrorCode error);
   void onGnssEnergyConsumedInfo(telux::loc::GnssEnergyConsumedInfo gnssEnergyConsumed,
       telux::common::ErrorCode error);
   void onGetYearOfHwInfo(uint16_t yearOfHw, telux::common::ErrorCode error);
   void onMinGpsWeekInfo(uint16_t minGpsWeek, telux::common::ErrorCode error);
   void onMinSVElevationInfo(uint8_t minSVElevation, telux::common::ErrorCode error);
   void onRobustLocationInfo(const telux::loc::RobustLocationConfiguration rLConfig,
       telux::common::ErrorCode error);
   void onSecondaryBandInfo(const telux::loc::ConstellationSet set,
     telux::common::ErrorCode error);

private:
   std::string commandName_;
};

#endif  // MYLOCATIONCOMMANDCALLACK_HPP
