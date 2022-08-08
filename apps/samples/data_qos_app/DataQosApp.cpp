/*
 *  Copyright (c) 2020, The Linux Foundation. All rights reserved.
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
/*
 *  Changes from Qualcomm Innovation Center are provided under the following license:
 *
 *  Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted (subject to the limitations in the
 *  disclaimer below) provided that the following conditions are met:
 *
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *
 *      * Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials provided
 *        with the distribution.
 *
 *      * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *        contributors may be used to endorse or promote products derived
 *        from this software without specific prior written permission.
 *
 *  NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 *  GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 *  HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 *  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 *  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 *  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <iostream>
#include <memory>
#include <cstdlib>
#include <assert.h>
#include <assert.h>
#include <iterator>

#include <telux/common/CommonDefines.hpp>
#include <telux/data/DataDefines.hpp>
#include <telux/data/DataFactory.hpp>

using namespace std;
using std::promise;
using std::shared_ptr;
using telux::data::QosFlowMaskType;
using telux::data::QosIPFlowMaskType;
using telux::data::TrafficFlowTemplate;

#define PROTO_TCP 6
#define PROTO_UDP 17
#define PROTO_TCP_UDP 253
/**
 * @file: DataQosApp.cpp
 *
 * @brief: Simple application to demostrate Qos notification on a data call
 */

static std::promise<telux::common::ErrorCode> gCallbackPromise;
static std::shared_ptr<telux::data::IDataCall> gDataCall;
static int gProfileID;

static void printStatus(telux::common::Status status) {
   switch (status)
   {
      case telux::common::Status::SUCCESS:
         std::cout << "Operation processed successfully" << std::endl;
         break;
      case telux::common::Status::FAILED:
         std::cout << "Operation processing failed" << std::endl;
         break;
      case telux::common::Status::NOCONNECTION:
         std::cout << "Connection to Socket server has not been established" << std::endl;
         break;
      case telux::common::Status::NOSUBSCRIPTION:
         std::cout << "Subscription not available" << std::endl;
         break;
      case telux::common::Status::INVALIDPARAM:
         std::cout << "Input parameters are invalid" << std::endl;
         break;
      case telux::common::Status::INVALIDSTATE:
         std::cout << "Invalid State detected" << std::endl;
         break;
      case telux::common::Status::NOTREADY:
         std::cout << "Subsystem is not ready" << std::endl;
         break;
      case telux::common::Status::NOTALLOWED:
         std::cout << "Operation not allowed" << std::endl;
         break;
      case telux::common::Status::NOTIMPLEMENTED:
         std::cout << "Feature not supported" << std::endl;
         break;
      case telux::common::Status::CONNECTIONLOST:
         std::cout << "Connection to Socket server lost" << std::endl;
         break;
      case telux::common::Status::EXPIRED:
         std::cout << "Operation has expired" << std::endl;
         break;
      case telux::common::Status::ALREADY:
         std::cout << "Already registered handler" << std::endl;
         break;
      case telux::common::Status::NOSUCH:
         std::cout << "No such object" << std::endl;
         break;
      case telux::common::Status::NOTSUPPORTED:
         std::cout << "Not supported on target platform" << std::endl;
         break;
      default:
         break;
   }
}

static std::string trafficClassToString(telux::data::IpTrafficClassType tc) {
   switch(tc) {
      case telux::data::IpTrafficClassType::CONVERSATIONAL:
         return "CONVERSATIONAL";
      case telux::data::IpTrafficClassType::STREAMING:
         return "STREAMING";
      case telux::data::IpTrafficClassType::INTERACTIVE:
         return "INTERACTIVE";
      case telux::data::IpTrafficClassType::BACKGROUND:
         return "BACKGROUND";
      default: {
         return "UNKNOWN";
      }
   }
}

static std::string flowStateEventToString(telux::data::QosFlowStateChangeEvent state) {
   switch(state) {
      case telux::data::QosFlowStateChangeEvent::ACTIVATED:
         return "ACTIVATED";
      case telux::data::QosFlowStateChangeEvent::MODIFIED:
         return "MODIFIED";
      case telux::data::QosFlowStateChangeEvent::DELETED:
         return "DELETED";
      default: {
         return "Unknown";
      }
   }
}

static std::string ipFamilyTypeToString(telux::data::IpFamilyType ipType) {
   switch(ipType) {
      case telux::data::IpFamilyType::IPV4:
         return "IPv4";
      case telux::data::IpFamilyType::IPV6:
         return "IPv6";
      case telux::data::IpFamilyType::IPV4V6:
         return "IPv4v6";
      case telux::data::IpFamilyType::UNKNOWN:
      default:
         return "NA";
   }
}

void printFilterDetails(std::shared_ptr<telux::data::IIpFilter> filter) {

   telux::data::IPv4Info ipv4Info_ = filter->getIPv4Info();
   if(!ipv4Info_.srcAddr.empty()) {
      std::cout << "\tIPv4 Src Address : " << ipv4Info_.srcAddr << std::endl;
   }
   if(!ipv4Info_.srcSubnetMask.empty()) {
      std::cout << "\tIPv4 Src Subnet Mask : " << ipv4Info_.srcSubnetMask << std::endl;
   }
   if(!ipv4Info_.destAddr.empty()) {
      std::cout << "\tIPv4 Dest Address : " << ipv4Info_.destAddr << std::endl;
   }
   if(!ipv4Info_.destSubnetMask.empty()) {
      std::cout << "\tIPv4 Dest Subnet Mask : " << ipv4Info_.destSubnetMask << std::endl;
   }
   if(ipv4Info_.value > 0) {
      std::cout << "\tIPv4 Type of service value : " << (int)ipv4Info_.value << std::endl;
   }
   if(ipv4Info_.mask > 0) {
      std::cout << "\tIPv4 Type of service mask : " << (int)ipv4Info_.mask << std::endl;
   }

   telux::data::IPv6Info ipv6Info_ = filter->getIPv6Info();
   if(!ipv6Info_.srcAddr.empty()) {
      std::cout << "\tIPv6 Src Address : " << ipv6Info_.srcAddr << std::endl;
   }
   if(!ipv6Info_.destAddr.empty()) {
      std::cout << "\tIPv6 Dest Address : " << ipv6Info_.destAddr << std::endl;
   }
   if(ipv6Info_.val > 0) {
      std::cout << "\tIPv6 Traffic class value : " << (int)ipv6Info_.val << std::endl;
   }
   if(ipv6Info_.mask > 0) {
      std::cout << "\tIPv6 Traffic class mask : " << (int)ipv6Info_.mask << std::endl;
   }
   if(ipv6Info_.flowLabel > 0) {
      std::cout << "\tIPv6 Flow label : " << (int)ipv6Info_.flowLabel << std::endl;
   }

   telux::data::IpProtocol proto = filter->getIpProtocol();
   switch (proto) {
      case PROTO_TCP: {
         auto tcpFilter = std::dynamic_pointer_cast<telux::data::ITcpFilter>(filter);
         if (tcpFilter) {
            telux::data::TcpInfo portInfo_ = tcpFilter->getTcpInfo();
            if (portInfo_.src.port > 0) {
               std::cout << "\tTCP Src Port: " << portInfo_.src.port << std::endl;
            }
            if (portInfo_.src.range > 0) {
               std::cout << "\tTCP Src Range: " << portInfo_.src.range << std::endl;
            }
            if (portInfo_.dest.port > 0) {
               std::cout << "\tTCP Dest Port: " << portInfo_.dest.port << std::endl;
            }
            if (portInfo_.dest.range > 0) {
               std::cout << "\tTCP Dest Range: " << portInfo_.dest.range << std::endl;
            }
         }
      } break;
      case PROTO_UDP: {
         auto udpFilter = std::dynamic_pointer_cast<telux::data::IUdpFilter>(filter);
         if (udpFilter) {
            telux::data::UdpInfo portInfo_ = udpFilter->getUdpInfo();
            if (portInfo_.src.port > 0) {
               std::cout << "\tUDP Src Port: " << portInfo_.src.port << std::endl;
            }
            if (portInfo_.src.range > 0) {
               std::cout << "\tUDP Src Range: " << portInfo_.src.range << std::endl;
            }
            if (portInfo_.dest.port > 0) {
               std::cout << "\tUDP Dest Port: " << portInfo_.dest.port << std::endl;
            }
            if (portInfo_.dest.range > 0) {
               std::cout << "\tUDP Dest Range: " << portInfo_.dest.range << std::endl;
            }
         }
      } break;
      case PROTO_TCP_UDP: {
         auto tcpFilter = std::dynamic_pointer_cast<telux::data::ITcpFilter>(filter);
         if (tcpFilter) {
            telux::data::TcpInfo portInfo_ = tcpFilter->getTcpInfo();
            if (portInfo_.src.port > 0) {
               std::cout << "\tTCP Src Port: " << portInfo_.src.port << std::endl;
            }
            if (portInfo_.src.range > 0) {
               std::cout << "\tTCP Src Range: " << portInfo_.src.range << std::endl;
            }
            if (portInfo_.dest.port > 0) {
               std::cout << "\tTCP Dest Port: " << portInfo_.dest.port << std::endl;
            }
            if (portInfo_.dest.range > 0) {
               std::cout << "\tTCP Dest Range: " << portInfo_.dest.range << std::endl;
            }
         }
         auto udpFilter = std::dynamic_pointer_cast<telux::data::IUdpFilter>(filter);
         if (udpFilter) {
            telux::data::UdpInfo portInfo_ = udpFilter->getUdpInfo();
            if (portInfo_.src.port > 0) {
               std::cout << "\tUDP Src Port: " << portInfo_.src.port << std::endl;
            }
            if (portInfo_.src.range > 0) {
               std::cout << "\tUDP Src Range: " << portInfo_.src.range << std::endl;
            }
            if (portInfo_.dest.port > 0) {
               std::cout << "\tUDP Dest Port: " << portInfo_.dest.port << std::endl;
            }
            if (portInfo_.dest.range > 0) {
               std::cout << "\tUDP Dest Range: " << portInfo_.dest.range << std::endl;
            }
         }
      } break;
      default: {
         std::cout << " Invalid XPort Protocol" <<std::endl;
      }
   }
}

void logQosDetails(int profileId, std::shared_ptr<TrafficFlowTemplate> &tft) {
   std::cout << " QoS Identifier : " << tft->qosId << std::endl;
   std::cout << " Profile Id : " << profileId << std::endl;

   if (tft->mask.test(QosFlowMaskType::MASK_FLOW_TX_GRANTED) &&
         (tft->txGrantedFlow.mask.test(QosIPFlowMaskType::MASK_IP_FLOW_TRF_CLASS) ||
            tft->txGrantedFlow.mask.test(QosIPFlowMaskType::MASK_IP_FLOW_DATA_RATE_MIN_MAX))) {
      std::cout << " TX QOS FLow Granted: " << std::endl;

      if (tft->txGrantedFlow.mask.test(QosIPFlowMaskType::MASK_IP_FLOW_TRF_CLASS)) {
         std::cout << "\tIP FLow Traffic class: "
            << trafficClassToString(tft->txGrantedFlow.tfClass) << std::endl;

      }
      if (tft->txGrantedFlow.mask.test(QosIPFlowMaskType::MASK_IP_FLOW_DATA_RATE_MIN_MAX)) {
         std::cout << "\tMaximum required data rate (bits per second): "
            << tft->txGrantedFlow.dataRate.maxRate << std::endl;
         std::cout << "\tMinimum required data rate (bits per second): "
            << tft->txGrantedFlow.dataRate.minRate << std::endl;
      }
   }

   if (tft->mask.test(QosFlowMaskType::MASK_FLOW_RX_GRANTED) &&
         (tft->rxGrantedFlow.mask.test(QosIPFlowMaskType::MASK_IP_FLOW_TRF_CLASS) ||
            tft->rxGrantedFlow.mask.test(QosIPFlowMaskType::MASK_IP_FLOW_DATA_RATE_MIN_MAX))) {
      std::cout << " RX QOS FLow Granted: " << std::endl;

      if (tft->rxGrantedFlow.mask.test(QosIPFlowMaskType::MASK_IP_FLOW_TRF_CLASS)) {
         std::cout << "\tIP FLow Traffic class: "
            << trafficClassToString(tft->rxGrantedFlow.tfClass) << std::endl;

      }
      if (tft->rxGrantedFlow.mask.test(QosIPFlowMaskType::MASK_IP_FLOW_DATA_RATE_MIN_MAX)) {
         std::cout << "\tMaximum required data rate (bits per second): "
            << tft->rxGrantedFlow.dataRate.maxRate << std::endl;
         std::cout << "\tMinimum required data rate (bits per second): "
            << tft->rxGrantedFlow.dataRate.minRate << std::endl;
      }
   }

   if (tft->mask.test(QosFlowMaskType::MASK_FLOW_TX_FILTERS)) {
      for (uint32_t i = 0 ; i < tft->txFiltersLength ; i++) {
         for (auto filter:tft->txFilters[i].filter) {
            telux::data::IpProtocol proto = filter->getIpProtocol();
            std::string protocol = "TCP";
            if (PROTO_UDP == proto) {
               protocol = "UDP";
            }
            std::cout <<  " " << protocol << " TX Filter: " << (i+1) << std::endl;
            std::cout << "\tFilter ID: " << tft->txFilters[i].filterId << std::endl;
            std::cout << "\tFilter Precedence: " << tft->txFilters[i].filterPrecedence << std::endl;
            if (filter) {
               std::cout << "\tIP Family: "
                  << ipFamilyTypeToString(filter->getIpFamily()) << std::endl;
               printFilterDetails(filter);
            }
         }
      }
   }

   if (tft->mask.test(QosFlowMaskType::MASK_FLOW_RX_FILTERS)) {
      for (uint32_t i = 0 ; i < tft->rxFiltersLength ; i++) {
         for (auto filter:tft->rxFilters[i].filter) {
            telux::data::IpProtocol proto = filter->getIpProtocol();
            std::string protocol = "TCP";
            if (PROTO_UDP == proto) {
               protocol = "UDP";
            }
            std::cout << " " << protocol << " RX Filter: " << (i+1) << std::endl;
            std::cout << "\tFilter ID: " << tft->rxFilters[i].filterId << std::endl;
            std::cout << "\tFilter Precedence: " << tft->rxFilters[i].filterPrecedence << std::endl;
            if (filter) {
               std::cout << "\tIP Family: "
                  << ipFamilyTypeToString(filter->getIpFamily()) << std::endl;
               printFilterDetails(filter);
            }
         }
      }
   }
}

// Response callback for start or stop dataCall
void responseCallback(const std::shared_ptr<telux::data::IDataCall> &dataCall,
                      telux::common::ErrorCode errorCode) {
   std::cout << "startCallResponse: errorCode: " << static_cast<int>(errorCode) << std::endl;
    if (telux::common::ErrorCode::SUCCESS == errorCode) {
        gDataCall = dataCall;
    }
   gCallbackPromise.set_value(errorCode);
}

void onTFTResponse(const std::vector<std::shared_ptr<TrafficFlowTemplate>> &tfts,
   telux::common::ErrorCode error) {
   std::cout << "\n onTFTResponse" << std::endl;

   if (error == telux::common::ErrorCode::SUCCESS) {
      for (auto tft:tfts) {
         std::cout << " ----------------------------------------------------------\n";
         std::cout << " ** TFT Details **\n";
         std::cout << " Flow State: "
            << flowStateEventToString(telux::data::QosFlowStateChangeEvent::ACTIVATED) << std::endl;
         logQosDetails(gProfileID, tft);
         std::cout << " ----------------------------------------------------------\n\n";
      }
   } else {
      std::cout << "ErrorCode: " << static_cast<int>(error) << std::endl;
   }
}

// Implementation of IDataConnectionListener
class DataConnectionListener : public telux::data::IDataConnectionListener {
public:
   void onDataCallInfoChanged(const std::shared_ptr<telux::data::IDataCall> &dataCall) override {
      std::cout << "\n onDataCallInfoChanged";
      logDataCallDetails(dataCall);
   }

   void onTrafficFlowTemplateChange(const std::shared_ptr<telux::data::IDataCall> &dataCall,
      const std::vector<std::shared_ptr<telux::data::TftChangeInfo>> &tfts) override {

      std::cout << "\n onTrafficFlowTemplateChange" << std::endl;
      for (auto tft:tfts) {
         std::cout << " ----------------------------------------------------------\n";
         std::cout << " ** TFT Details **\n";
         std::cout << " Flow State: "
            << flowStateEventToString(tft->stateChange) << std::endl;
         logQosDetails(dataCall->getProfileId(), tft->tft);
         std::cout << " ----------------------------------------------------------\n\n";
      }
   }

private:
   void logDataCallDetails(const std::shared_ptr<telux::data::IDataCall> &dataCall) {
      std::cout << " ** DataCall Details **\n";
      std::cout << " ProfileID: " << dataCall->getProfileId() << std::endl;
      std::cout << " interfaceName: " << dataCall->getInterfaceName() << std::endl;
      std::cout << " DataCallStatus: " << (int)dataCall->getDataCallStatus() << std::endl;
      std::cout
         << " DataCallEndReason: Type = " << static_cast<int>(dataCall->getDataCallEndReason().type)
         << std::endl;
      std::list<telux::data::IpAddrInfo> ipAddrList = dataCall->getIpAddressInfo();
      for(auto &it : ipAddrList) {
         std::cout << "\n ifAddress: " << it.ifAddress
                   << "\n primaryDnsAddress: " << it.primaryDnsAddress
                   << "\n secondaryDnsAddress: " << it.secondaryDnsAddress << '\n';
      }
      std::cout << " IpFamilyType: " << static_cast<int>(dataCall->getIpFamilyType()) << '\n';
      std::cout << " TechPreference: " << static_cast<int>(dataCall->getTechPreference()) << '\n';
      std::cout << " DataBearerTechnology: " << static_cast<int>(dataCall->getCurrentBearerTech())
                << '\n';
   }

};

int main(int argc, char *argv[]) {
   // [1] Get the DataFactory
   auto &dataFactory = telux::data::DataFactory::getInstance();
   // [1.1] Get data connection manager object
   auto dataConnMgr = dataFactory.getDataConnectionManager();

   // [2] Check if data connection subsystem is ready
   bool dataConnectionSubSystemStatus = dataConnMgr->isSubsystemReady();

   // [2.1] If data connection subsystem is not ready, wait for it to be ready
   if(!dataConnectionSubSystemStatus) {
      std::cout << "DATA connection subsystem is not ready" << std::endl;
      std::cout << "wait unconditionally for it to be ready " << std::endl;
      std::future<bool> f = dataConnMgr->onSubsystemReady();
      // If we want to wait unconditionally for data subsystem to be ready
      dataConnectionSubSystemStatus = f.get();
   }

   // [2.2] Exit the application, if SDK is unable to initialize data subsystems
   if(dataConnectionSubSystemStatus) {
      std::cout << " *** DATA connection subsystem is Ready *** " << std::endl;
   } else {
      std::cout << " *** ERROR - Unable to initialize data subsystem *** " << std::endl;
      return 1;
   }

   // [3] Register for Data connection listener
   std::shared_ptr<telux::data::IDataConnectionListener> dataConnectionListener
      = std::make_shared<DataConnectionListener>();
   dataConnMgr->registerListener(dataConnectionListener);

   // [4] Start data call on the mentioned profile id, retrive qos TFT info on IDatacall.
   if(argc == 2) {
      gProfileID = std::atoi(argv[1]);
      telux::data::IpFamilyType ipFamilyType = telux::data::IpFamilyType::IPV4;
      dataConnMgr->startDataCall(gProfileID, ipFamilyType, responseCallback);
      assert(telux::common::ErrorCode::SUCCESS == gCallbackPromise.get_future().get());

      if (gDataCall) {
         telux::common::Status retStat =
            gDataCall->requestTrafficFlowTemplate(ipFamilyType, onTFTResponse);
         printStatus(retStat);
      }

   } else {
      std::cout << "\n Invalid argument!!! \n\n";
      std::cout << "\n Sample command is: \n";
      std::cout << "\n\t ./data_qos_app <profieId>\n";
      std::cout << "\n\t ./data_qos_app 1 --> to start the data call on \
      profile Id 1 and query QOS tft information \n";
   }

   // [5] Exit logic for the application
   std::cout << "\n\nPress ENTER to exit!!! \n\n";
   std::cin.ignore();

   // [6] Cleanup
   dataConnMgr->deregisterListener(dataConnectionListener);
   dataConnMgr = nullptr;
   return 0;
}
