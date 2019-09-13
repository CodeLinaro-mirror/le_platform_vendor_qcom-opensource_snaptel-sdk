/*
 *  Copyright (c) 2018, The Linux Foundation. All rights reserved.
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

#include "iostream"
#include "MyCellInfoHandler.hpp"
#include "Utils.hpp"

#define PRINT_CB std::cout << "\033[1;35mCallback: \033[0m"

using namespace telux::tel;
using namespace telux::common;

std::string MyCellInfoCallback::signalLevelToString(telux::tel::SignalStrengthLevel level) {
    switch(level){
        case telux::tel::SignalStrengthLevel::LEVEL_1 : return "LEVEL_1";
        case telux::tel::SignalStrengthLevel::LEVEL_2 : return "LEVEL_2";
        case telux::tel::SignalStrengthLevel::LEVEL_3 : return "LEVEL_3";
        case telux::tel::SignalStrengthLevel::LEVEL_4 : return "LEVEL_4";
        case telux::tel::SignalStrengthLevel::LEVEL_5 : return "LEVEL_5";
        case telux::tel::SignalStrengthLevel::LEVEL_UNKNOWN : return "LEVEL_UNKNOWN";
        default:
            return "Invalid Signal Level";
    }
}

void MyCellInfoCallback::cellInfoListResponse(
   std::vector<std::shared_ptr<telux::tel::CellInfo>> cellInfoList,
   telux::common::ErrorCode error) {
   if(error == telux::common::ErrorCode::SUCCESS) {
      PRINT_CB << "Received call back for requestCellInfo in MyCellInfoCallback" << std::endl;
      for(auto cellinfo : cellInfoList) {
         PRINT_CB << "CellInfo Type: " << (int)cellinfo->getType() << std::endl;
         if(cellinfo->getType() == telux::tel::CellType::GSM) {
            PRINT_CB << "GSM Cellinfo " << std::endl;
            auto gsmCellInfo = std::static_pointer_cast<telux::tel::GsmCellInfo>(cellinfo);
            PRINT_CB << "GSM isRegistered: " << gsmCellInfo->isRegistered() << std::endl;
            PRINT_CB << "GSM mcc: " << gsmCellInfo->getCellIdentity().getMcc() << std::endl;
            PRINT_CB << "GSM mnc: " << gsmCellInfo->getCellIdentity().getMnc() << std::endl;
            PRINT_CB << "GSM lac: " << gsmCellInfo->getCellIdentity().getLac() << std::endl;
            PRINT_CB << "GSM cid: " << gsmCellInfo->getCellIdentity().getIdentity() << std::endl;
            PRINT_CB << "GSM arfcn: " << gsmCellInfo->getCellIdentity().getArfcn() << std::endl;
            // GSM signal strength
            if(gsmCellInfo->getSignalStrengthInfo().getGsmSignalStrength()
                == INVALID_SIGNAL_STRENGTH_VALUE) {
               PRINT_CB << "GSM Signal Strength: "<< "UNAVAILABLE" << std::endl;
            } else {
               PRINT_CB << "GSM Signal Strength: "
                    << gsmCellInfo->getSignalStrengthInfo().getGsmSignalStrength() << std::endl;
            }

            if(gsmCellInfo->getSignalStrengthInfo().getGsmBitErrorRate()
                == INVALID_SIGNAL_STRENGTH_VALUE) {
               PRINT_CB << "GSM Bit Error Rate: "<< "UNAVAILABLE" << std::endl;
            } else {
               PRINT_CB << "GSM Bit Error Rate: "
                   << gsmCellInfo->getSignalStrengthInfo().getGsmBitErrorRate()<< std::endl;
            }

            if(gsmCellInfo->getSignalStrengthInfo().getDbm() == INVALID_SIGNAL_STRENGTH_VALUE) {
               PRINT_CB << "GSM Signal Strength(in dBm): " << "UNAVAILABLE" << std::endl;
            } else {
               PRINT_CB << "GSM Signal Strength(in dBm): "
                   << gsmCellInfo->getSignalStrengthInfo().getDbm() << std::endl;
            }

            if(gsmCellInfo->getSignalStrengthInfo().getTimingAdvance()
                == INVALID_SIGNAL_STRENGTH_VALUE) {
               PRINT_CB << "GSM Timing Advance(in bit periods): " << "UNAVAILABLE" << std::endl;
            } else {
               PRINT_CB << "GSM Timing Advance(in bit periods): "
                   << gsmCellInfo->getSignalStrengthInfo().getTimingAdvance() << std::endl;
            }

            PRINT_CB << "GSM Signal Level: "
               << signalLevelToString(gsmCellInfo->getSignalStrengthInfo().getLevel())<< std::endl;

         } else if(cellinfo->getType() == telux::tel::CellType::CDMA) {
            PRINT_CB << "CDMA Cellinfo " << std::endl;
            auto cdmaCellInfo = std::static_pointer_cast<telux::tel::CdmaCellInfo>(cellinfo);
            PRINT_CB << "CDMA isRegistered: " << cdmaCellInfo->isRegistered() << std::endl;
            PRINT_CB << "CDMA networkId: " << cdmaCellInfo->getCellIdentity().getNid() << std::endl;
            PRINT_CB << "CDMA SystemId: " << cdmaCellInfo->getCellIdentity().getSid() << std::endl;
            PRINT_CB << "CDMA BaseStationId: " << cdmaCellInfo->getCellIdentity().getBaseStationId()
                     << std::endl;
            PRINT_CB << "CDMA Longitude: " << cdmaCellInfo->getCellIdentity().getLongitude()
                     << std::endl;
            PRINT_CB << "CDMA Latitude: " << cdmaCellInfo->getCellIdentity().getLatitude()
                     << std::endl;
            // CDMA Signal Strength
            if(cdmaCellInfo->getSignalStrengthInfo().getDbm() == INVALID_SIGNAL_STRENGTH_VALUE) {
               PRINT_CB << "CDMA/EVDO Signal Strength(in dBm): "<< "UNAVAILABLE" << std::endl;
            } else {
               PRINT_CB << "CDMA/EVDO Signal Strength(in dBm): "
               << cdmaCellInfo->getSignalStrengthInfo().getDbm() << std::endl;
            }

            if(cdmaCellInfo->getSignalStrengthInfo().getCdmaEcio()
                == INVALID_SIGNAL_STRENGTH_VALUE) {
               PRINT_CB << "CDMA Ec/Io(in dB): "<< "UNAVAILABLE" << std::endl;
            } else {
               PRINT_CB << "CDMA Ec/Io(in dB): " <<
                   cdmaCellInfo->getSignalStrengthInfo().getCdmaEcio() << std::endl;
            }

            if(cdmaCellInfo->getSignalStrengthInfo().getEvdoEcio()
                == INVALID_SIGNAL_STRENGTH_VALUE) {
               PRINT_CB << "EVDO Ec/Io(in dB): "<< "UNAVAILABLE" << std::endl;
            } else {
               PRINT_CB << "EVDO Ec/Io(in dB): " <<
                   cdmaCellInfo->getSignalStrengthInfo().getEvdoEcio() << std::endl;
            }

            if(cdmaCellInfo->getSignalStrengthInfo().getEvdoSignalNoiseRatio()
                == INVALID_SIGNAL_STRENGTH_VALUE) {
               PRINT_CB << "EVDO Signal Noise Ratio: "<< "UNAVAILABLE" << std::endl;
            } else {
               PRINT_CB << "EVDO Signal Noise Ratio: "
                    << cdmaCellInfo->getSignalStrengthInfo().getEvdoSignalNoiseRatio() << std::endl;
            }
            PRINT_CB
            << "CDMA Signal Level: "
            << signalLevelToString(cdmaCellInfo->getSignalStrengthInfo().getLevel())<< std::endl;
         } else if(cellinfo->getType() == telux::tel::CellType::LTE) {
            PRINT_CB << "LTE Cellinfo  " << std::endl;
            auto lteCellInfo = std::static_pointer_cast<telux::tel::LteCellInfo>(cellinfo);
            PRINT_CB << "LTE isRegistered: " << lteCellInfo->isRegistered() << std::endl;
            PRINT_CB << "LTE mcc: " << lteCellInfo->getCellIdentity().getMcc() << std::endl;
            PRINT_CB << "LTE mnc: " << lteCellInfo->getCellIdentity().getMnc() << std::endl;
            PRINT_CB << "LTE cid: " << lteCellInfo->getCellIdentity().getIdentity() << std::endl;
            PRINT_CB << "LTE pid: " << lteCellInfo->getCellIdentity().getPhysicalCellId()
                     << std::endl;
            PRINT_CB << "LTE tac: " << lteCellInfo->getCellIdentity().getTrackingAreaCode()
                     << std::endl;
            PRINT_CB << "LTE arfcn: " << lteCellInfo->getCellIdentity().getEarfcn() << std::endl;
            // LTE Signal Strength

            if(lteCellInfo->getSignalStrengthInfo().getLteSignalStrength()
                == INVALID_SIGNAL_STRENGTH_VALUE) {
               PRINT_CB << "LTE Signal Strength: "<< "UNAVAILABLE" << std::endl;
            } else {
               PRINT_CB << "LTE Signal Strength: "
                    << lteCellInfo->getSignalStrengthInfo().getLteSignalStrength() << std::endl;
            }

            if(lteCellInfo->getSignalStrengthInfo().getDbm() == INVALID_SIGNAL_STRENGTH_VALUE) {
               PRINT_CB << "LTE Signal Strength(in dBm): "<< "UNAVAILABLE" << std::endl;
            } else {
               PRINT_CB << "LTE Signal Strength(in dBm): "
                    << lteCellInfo->getSignalStrengthInfo().getDbm() << std::endl;
            }

            if(lteCellInfo->getSignalStrengthInfo().getDbm() == INVALID_SIGNAL_STRENGTH_VALUE) {
               PRINT_CB << "LTE Reference Signal Receive Power(in dBm): "<< "UNAVAILABLE"
               << std::endl;
            } else {
               PRINT_CB << "LTE Reference Signal Receive Power(in dBm): "
                    << lteCellInfo->getSignalStrengthInfo().getDbm() << std::endl;
            }

            if(lteCellInfo->getSignalStrengthInfo().getLteReferenceSignalReceiveQuality()
                == INVALID_SIGNAL_STRENGTH_VALUE) {
               PRINT_CB << "LTE Reference Signal Receive Quality(in dB): "
                   << "UNAVAILABLE" << std::endl;
            } else {
               PRINT_CB << "LTE Reference Signal Receive Quality(in dB): "
                   << lteCellInfo->getSignalStrengthInfo().getLteReferenceSignalReceiveQuality()
                   << std::endl;
            }

            if(lteCellInfo->getSignalStrengthInfo().getLteReferenceSignalSnr()
                == INVALID_SIGNAL_STRENGTH_VALUE) {
               PRINT_CB << "LTE Reference Signal SNR(in dB): "<< "UNAVAILABLE" << std::endl;
            } else {
               PRINT_CB << "LTE Reference Signal SNR(in dB): "
                    << lteCellInfo->getSignalStrengthInfo().getLteReferenceSignalSnr() * 0.1
                    << std::endl;
            }

            if(lteCellInfo->getSignalStrengthInfo().getLteChannelQualityIndicator()
                == INVALID_SIGNAL_STRENGTH_VALUE) {
               PRINT_CB << "LTE Channel Quality Indicator: "<< "UNAVAILABLE" << std::endl;
            } else {
               PRINT_CB << "LTE Channel Quality Indicator: "
                    << lteCellInfo->getSignalStrengthInfo().getLteChannelQualityIndicator()
                    << std::endl;
            }

            if(lteCellInfo->getSignalStrengthInfo().getTimingAdvance()
                == INVALID_SIGNAL_STRENGTH_VALUE) {
               PRINT_CB << "LTE Timing Advance: "<< "UNAVAILABLE" << std::endl;
            } else {
               PRINT_CB << "LTE Timing Advance: "
                    << lteCellInfo->getSignalStrengthInfo().getTimingAdvance()
                    << std::endl;
            }
            PRINT_CB << "LTE Signal Level: "
            << signalLevelToString(lteCellInfo->getSignalStrengthInfo().getLevel())
             << std::endl;
         } else if(cellinfo->getType() == telux::tel::CellType::WCDMA) {
            PRINT_CB << "WCDMA Cellinfo " << std::endl;
            auto wcdmaCellInfo = std::static_pointer_cast<telux::tel::WcdmaCellInfo>(cellinfo);
            PRINT_CB << "WCDMA isRegistered: " << wcdmaCellInfo->isRegistered() << std::endl;
            PRINT_CB << "WCDMA mcc: " << wcdmaCellInfo->getCellIdentity().getMcc() << std::endl;
            PRINT_CB << "WCDMA mnc: " << wcdmaCellInfo->getCellIdentity().getMnc() << std::endl;
            PRINT_CB << "WCDMA lac: " << wcdmaCellInfo->getCellIdentity().getLac() << std::endl;
            PRINT_CB << "WCDMA cid: " << wcdmaCellInfo->getCellIdentity().getIdentity()
                     << std::endl;
            PRINT_CB << "WCDMA psc: " << wcdmaCellInfo->getCellIdentity().getPrimaryScramblingCode()
                     << std::endl;
            PRINT_CB << "WCDMA arfcn: " << wcdmaCellInfo->getCellIdentity().getUarfcn()
                     << std::endl;
            // WCDMA Signal Strength
            if(wcdmaCellInfo->getSignalStrengthInfo().getSignalStrength()
                == INVALID_SIGNAL_STRENGTH_VALUE) {
               PRINT_CB << "WCDMA Signal Strength: "<< "UNAVAILABLE" << std::endl;
            } else {
               PRINT_CB << "WCDMA Signal Strength: "
                    << wcdmaCellInfo->getSignalStrengthInfo().getSignalStrength() << std::endl;
            }

            if(wcdmaCellInfo->getSignalStrengthInfo().getDbm() == INVALID_SIGNAL_STRENGTH_VALUE) {
               PRINT_CB << "WCDMA Signal Strength(in dBm): "<< "UNAVAILABLE" << std::endl;
            } else {
               PRINT_CB << "WCDMA Signal Strength(in dBm): "
                    << wcdmaCellInfo->getSignalStrengthInfo().getDbm() << std::endl;
            }

            if(wcdmaCellInfo->getSignalStrengthInfo().getBitErrorRate()
                == INVALID_SIGNAL_STRENGTH_VALUE) {
               PRINT_CB << "WCDMA Bit Error Rate: "<< "UNAVAILABLE" << std::endl;
            } else {
               PRINT_CB << "WCDMA Bit Error Rate: "
                    << wcdmaCellInfo->getSignalStrengthInfo().getBitErrorRate() << std::endl;
            }
            PRINT_CB
               << "WCDMA Signal Level: "
               << signalLevelToString(wcdmaCellInfo->getSignalStrengthInfo().getLevel())
               << std::endl;
         } else if(cellinfo->getType() == telux::tel::CellType::TDSCDMA) {
            PRINT_CB << "TDSCDMA Cellinfo " << std::endl;
            auto tdsCdmaCellInfo = std::static_pointer_cast<telux::tel::TdscdmaCellInfo>(cellinfo);
            PRINT_CB << "TDSCDMA isRegistered: " << tdsCdmaCellInfo->isRegistered() << std::endl;
            PRINT_CB << "TDSCDMA MCC: " << tdsCdmaCellInfo->getCellIdentity().getMcc() << std::endl;
            PRINT_CB << "TDSCDMA MNC: " << tdsCdmaCellInfo->getCellIdentity().getMnc() << std::endl;
            PRINT_CB << "TDSCDMA LAC : " << tdsCdmaCellInfo->getCellIdentity().getLac()
                     << std::endl;
            PRINT_CB << "TDSCDMA CID: " << tdsCdmaCellInfo->getCellIdentity().getIdentity()
                     << std::endl;
            PRINT_CB << "TDSCDMA Cell Parameters Id : "
                     << tdsCdmaCellInfo->getCellIdentity().getParametersId() << std::endl;
            // TDSCDMA signal strength..
            if(tdsCdmaCellInfo->getSignalStrengthInfo().getRscp()
                == INVALID_SIGNAL_STRENGTH_VALUE) {
               PRINT_CB << "TDSCDMA  Reference Signal Code Power(in dBm): "<< "UNAVAILABLE"
                   << std::endl;
            } else {
               PRINT_CB
               << "TDSCDMA  Reference Signal Code Power(in dBm): "
               << tdsCdmaCellInfo->getSignalStrengthInfo().getRscp() << std::endl;
            }
         }
      }
   } else {
      PRINT_CB << "RequestCellInfo failed, errorCode: " << static_cast<int>(error)
               << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
   }
}

void MyCellInfoCallback::cellInfoListRateResponse(telux::common::ErrorCode error) {
   if(error == telux::common::ErrorCode::SUCCESS) {
      PRINT_CB << "Set cell info list rate request executed successfully" << std::endl;
   } else {
      PRINT_CB << "Set cell info list rate request failed, errorCode: " << static_cast<int>(error)
               << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
   }
}