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

#define print_cb std::cout << "\033[1;35mCALLBACK: \033[0m"

using namespace telux::tel;
using namespace telux::common;

void MyCellInfoCallback::cellInfoListResponse(
   std::vector<std::shared_ptr<telux::tel::CellInfo>> cellInfoList,
   telux::common::ErrorCode error) {
   print_cb << "Received call back for requestCellInfo in MyCellInfoCallback"
                      << std::endl;
   for(auto cellinfo : cellInfoList) {
      print_cb <<  "CellInfo Type: " << (int)cellinfo->getType() << std::endl;
      if(cellinfo->getType() == telux::tel::CellType::GSM) {
         print_cb << "GSM Cellinfo " << std::endl;
         auto gsmCellInfo = std::static_pointer_cast<telux::tel::GsmCellInfo>(cellinfo);
         print_cb << "GSM isRegistered: " << gsmCellInfo->isRegistered()
                            << std::endl;
         print_cb << "GSM mcc: " << gsmCellInfo->getCellIdentity().getMcc() << std::endl;
         print_cb << "GSM mnc: " << gsmCellInfo->getCellIdentity().getMnc() << std::endl;
         print_cb << "GSM lac: " << gsmCellInfo->getCellIdentity().getLac() << std::endl;
         print_cb << "GSM cid: " << gsmCellInfo->getCellIdentity().getIdentity() << std::endl;
         print_cb << "GSM arfcn: " << gsmCellInfo->getCellIdentity().getArfcn() << std::endl;
         // GSM signal strength
         print_cb << "GSM Signal Strength: "
                            << gsmCellInfo->getSignalStrengthInfo().getGsmSignalStrength()
                            << std::endl;
         print_cb
            << "GSM Bit error rate: " << gsmCellInfo->getSignalStrengthInfo().getGsmBitErrorRate()
            << std::endl;
         print_cb
            << "GSM TimingAdvance: " << gsmCellInfo->getSignalStrengthInfo().getTimingAdvance()
            << std::endl;
         print_cb
            << "GSM Signal Level: " << (int)gsmCellInfo->getSignalStrengthInfo().getLevel()
            << std::endl;
      } else if(cellinfo->getType() == telux::tel::CellType::CDMA) {
         print_cb << "CDMA Cellinfo " << std::endl;
         auto cdmaCellInfo = std::static_pointer_cast<telux::tel::CdmaCellInfo>(cellinfo);
         print_cb << "CDMA isRegistered: " << cdmaCellInfo->isRegistered()
                            << std::endl;
         print_cb << "CDMA networkId: " << cdmaCellInfo->getCellIdentity().getNid()
                            << std::endl;
         print_cb << "CDMA SystemId: " << cdmaCellInfo->getCellIdentity().getSid()
                            << std::endl;
         print_cb
            << "CDMA BaseStationId: " << cdmaCellInfo->getCellIdentity().getBaseStationId() << std::endl;
         print_cb << "CDMA Longitude: " << cdmaCellInfo->getCellIdentity().getLongitude()
                            << std::endl;
         print_cb << "CDMA Latitude: " << cdmaCellInfo->getCellIdentity().getLatitude()
                            << std::endl;
         // CDMA Signal Strength
         print_cb << "CDMA Dbm: " << cdmaCellInfo->getSignalStrengthInfo().getDbm()
                            << std::endl;
         print_cb << "CDMA Ecio: " << cdmaCellInfo->getSignalStrengthInfo().getCdmaEcio()
                            << std::endl;
         print_cb << "EVDO Ecio: " << cdmaCellInfo->getSignalStrengthInfo().getEvdoEcio()
                            << std::endl;
         print_cb
            << "EVDO SNR: " << cdmaCellInfo->getSignalStrengthInfo().getEvdoSignalNoiseRatio()
            << std::endl;
         print_cb
            << "CDMA/EVDO Signal Level: " << (int)cdmaCellInfo->getSignalStrengthInfo().getLevel()
            << std::endl;
      } else if(cellinfo->getType() == telux::tel::CellType::LTE) {
         print_cb << "LTE Cellinfo  " << std::endl;
         auto lteCellInfo = std::static_pointer_cast<telux::tel::LteCellInfo>(cellinfo);
         print_cb << "LTE isRegistered: " << lteCellInfo->isRegistered()
                            << std::endl;
         print_cb << "LTE mcc: " << lteCellInfo->getCellIdentity().getMcc() << std::endl;
         print_cb << "LTE mnc: " << lteCellInfo->getCellIdentity().getMnc() << std::endl;
         print_cb << "LTE cid: " << lteCellInfo->getCellIdentity().getIdentity() << std::endl;
         print_cb << "LTE pid: " << lteCellInfo->getCellIdentity().getPhysicalCellId()
                            << std::endl;
         print_cb << "LTE tac: " << lteCellInfo->getCellIdentity().getTrackingAreaCode()
                            << std::endl;
         print_cb << "LTE arfcn: " << lteCellInfo->getCellIdentity().getEarfcn() << std::endl;
         // LTE Signal Strength
         print_cb << "LTE signal strength: "
                            << lteCellInfo->getSignalStrengthInfo().getLteSignalStrength()
                            << std::endl;
         print_cb << "LTE Rsrp: " << lteCellInfo->getSignalStrengthInfo().getDbm()
                            << std::endl;
         print_cb
            << "LTE Rsrq: "
            << lteCellInfo->getSignalStrengthInfo().getLteReferenceSignalReceiveQuality()
            << std::endl;
         print_cb
            << "LTE Rssnr: " << lteCellInfo->getSignalStrengthInfo().getLteReferenceSignalSnr()
            << std::endl;
         print_cb
            << "LTE Cqi: " << lteCellInfo->getSignalStrengthInfo().getLteChannelQualityIndicator()
            << std::endl;
         print_cb
            << "LTE Timing Advance: " << lteCellInfo->getSignalStrengthInfo().getTimingAdvance()
            << std::endl;
         print_cb
            << "LTE Signal Level: " << (int)lteCellInfo->getSignalStrengthInfo().getLevel()
            << std::endl;
      } else if(cellinfo->getType() == telux::tel::CellType::WCDMA) {
         print_cb << "WCDMA Cellinfo " << std::endl;
         auto wcdmaCellInfo = std::static_pointer_cast<telux::tel::WcdmaCellInfo>(cellinfo);
         print_cb << "WCDMA isRegistered: " << wcdmaCellInfo->isRegistered()
                            << std::endl;
         print_cb << "WCDMA mcc: " << wcdmaCellInfo->getCellIdentity().getMcc() << std::endl;
         print_cb << "WCDMA mnc: " << wcdmaCellInfo->getCellIdentity().getMnc() << std::endl;
         print_cb << "WCDMA lac: " << wcdmaCellInfo->getCellIdentity().getLac() << std::endl;
         print_cb << "WCDMA cid: " << wcdmaCellInfo->getCellIdentity().getIdentity()
                            << std::endl;
         print_cb
            << "WCDMA psc: " << wcdmaCellInfo->getCellIdentity().getPrimaryScramblingCode() << std::endl;
         print_cb << "WCDMA arfcn: " << wcdmaCellInfo->getCellIdentity().getUarfcn()
                            << std::endl;
         // WCDMA Signal Strength
         print_cb << "WCDMA Signal Strength: "
                            << wcdmaCellInfo->getSignalStrengthInfo().getSignalStrength()
                            << std::endl;
         print_cb
            << "WCDMA bit error rate: " << wcdmaCellInfo->getSignalStrengthInfo().getBitErrorRate()
            << std::endl;
         print_cb
            << "WCDMA Signal Level: " << (int)wcdmaCellInfo->getSignalStrengthInfo().getLevel()
            << std::endl;
      } else if(cellinfo->getType() == telux::tel::CellType::TDSCDMA) {
         print_cb << "TDSCDMA Cellinfo " << std::endl;
         auto tdsCdmaCellInfo = std::static_pointer_cast<telux::tel::TdscdmaCellInfo>(cellinfo);
         print_cb << "TDSCDMA isRegistered: " << tdsCdmaCellInfo->isRegistered()
                            << std::endl;
         print_cb << "TDSCDMA MCC: " << tdsCdmaCellInfo->getCellIdentity().getMcc()
                            << std::endl;
         print_cb << "TDSCDMA MNC: " << tdsCdmaCellInfo->getCellIdentity().getMnc()
                            << std::endl;
         print_cb << "TDSCDMA LAC : " << tdsCdmaCellInfo->getCellIdentity().getLac()
                            << std::endl;
         print_cb << "TDSCDMA CID: " << tdsCdmaCellInfo->getCellIdentity().getIdentity()
                            << std::endl;
         print_cb << "TDSCDMA Cell Parameters Id : "
                            << tdsCdmaCellInfo->getCellIdentity().getParametersId() << std::endl;
         // TDSCDMA signal strength..
         print_cb
            << "TDSCDMA power : "
            << tdsCdmaCellInfo->getSignalStrengthInfo().getRscp() << std::endl;
      }
   }
}

void MyCellInfoCallback::cellInfoListRateResponse(telux::common::ErrorCode error) {
   if(error == telux::common::ErrorCode::SUCCESS) {
      print_cb << "Set cell info list rate request executed successfully" << std::endl;
   } else {
      print_cb << "Set cell info list rate request failed" << std::endl;
   }
}