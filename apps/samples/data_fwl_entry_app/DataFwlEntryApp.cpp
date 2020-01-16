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

#include <iostream>
#include <memory>
#include <cstdlib>

#include <telux/data/DataDefines.hpp>
#include <telux/data/DataFactory.hpp>
#include <telux/data/net/FirewallManager.hpp>
#include "ConfigParser.hpp"


/**
 * @file: DataFwlEntryApp.cpp
 *
 * @brief: Simple application to add Firewall Entry. This application assumes firewall is enabled
 *         by running data_fwl_enable_app
 *         ./fwlEntry_sample_app <configuration file>
 */

std::promise<int> promise;

int main(int argc, char *argv[]) {
   if(argc == 2) {
      std::string configFile = argv[1];
      std::shared_ptr<ConfigParser> configParser = std::make_shared<ConfigParser>(configFile);

      telux::data::OperationType opType = static_cast<telux::data::OperationType>
          (std::atoi(configParser->getValue(std::string("OPERATION_TYPE")).c_str()));
      bool fwEnable = false;
      int profileId = std::atoi(configParser->getValue(std::string("PROFILE_ID")).c_str());
      telux::data::Direction fwDir = static_cast<telux::data::Direction>(
          std::atoi(configParser->getValue(std::string("DIRECTION")).c_str()));
      telux::data::IpProtocol proto;
      std::string prtocol = configParser->getValue(std::string("PROTOCOL"));
      if (!prtocol.compare("TCP")) {
         proto = 6;
      }
      else if (!prtocol.compare("UDP")) {
         proto = 17;
      }
      else
      {
         std::cout <<"Error: Unrecognized Protocol used .. exiting app" <<std::endl;
         return 1;
      }
      telux::data::IpFamilyType ipFamType = static_cast<telux::data::IpFamilyType>(
          std::atoi(configParser->getValue(std::string("IP_FAMILY")).c_str()));
       std::string srcAddr = configParser->getValue(std::string("SOURCE_ADDR"));
       std::string destAddr = configParser->getValue(std::string("DEST_ADDR"));
       int protSrcPort = std::atoi(configParser->getValue(
          std::string("PROTOCOL_SRC_PORT")).c_str());
       int protSrcRange = std::atoi(configParser->getValue(
          std::string("PROTOCOL_SRC_RANGE")).c_str());
       int protDestPort = std::atoi(configParser->getValue(
          std::string("PROTOCOL_DEST_PORT")).c_str());
       int protDestRange = std::atoi(configParser->getValue(
          std::string("PROTOCOL_DEST_RANGE")).c_str());

      // [1] Get the DataFactory and Firewall Manager instance
      auto &dataFactory = telux::data::DataFactory::getInstance();
      auto dataFwMgr  = dataFactory.getFirewallManager(opType);

      // [2] Check if data subsystem is ready
      bool subSystemStatus = dataFwMgr->isSubsystemReady();

      // [2.1] If data subsystem is not ready, wait for it to be ready
      if(!subSystemStatus) {
         std::cout << "Firewall subsystem is not ready" << std::endl;
         std::cout << "wait unconditionally for it to be ready " << std::endl;
         std::future<bool> f = dataFwMgr->onSubsystemReady();
         // If we want to wait unconditionally for data subsystem to be ready
         subSystemStatus = f.get();
      }

      // [3] Exit the application, if SDK is unable to initialize firewall subsystems
      if(subSystemStatus) {
         std::cout << " *** Firewall Sub System is Ready *** " << std::endl;
      } else {
         std::cout << " *** ERROR - Unable to initialize Firewall subsystem *** " << std::endl;
         return 1;
      }

      // [4] Get firewall Entry instance
      std::shared_ptr<telux::data::net::IFirewallEntry> fwEntry
         = dataFactory.getNewFirewallEntry(proto, fwDir, ipFamType);

      // [5] Get pointer to Ip Filter
      std::shared_ptr<telux::data::IIpFilter> ipFilter = fwEntry->getIProtocolFilter();

      // [6] Populate Ip Filter based on Ip Family type
      switch (ipFamType) {
         case telux::data::IpFamilyType::IPV4: {
               telux::data::IPv4Info info;
               info.srcAddr = srcAddr;
               info.destAddr = destAddr;
               info.srcSubnetMask = configParser->getValue(std::string("IPV4_SRC_SUBNET_MASK"));
               info.destSubnetMask = configParser->getValue(std::string("IPV4_DEST_SUBNET_MASK"));
               info.value = (uint8_t)std::atoi(
                  configParser->getValue(std::string("IPV4_SERVICE_TYPE")).c_str());
               info.mask = (uint8_t)std::atoi(
                  configParser->getValue(std::string("IPV4_SERVICE_TYPE_MASK")).c_str());
               info.nextProtoId = proto;
               ipFilter->setIPv4Info(info);
         } break;
         case telux::data::IpFamilyType::IPV6: {
               telux::data::IPv6Info info;
               info.srcAddr = srcAddr;
               info.destAddr = destAddr;
               info.nextProtoId = proto;
               info.val = (uint8_t)std::atoi(
                  configParser->getValue(std::string("IPV6_TRAFFIC_CLASS")).c_str());
               info.mask = (uint8_t)std::atoi(
                  configParser->getValue(std::string("IPV6_TRAFFIC_CLASS_MASK")).c_str());
               info.flowLabel = (uint32_t)std::atoi(
                  configParser->getValue(std::string("IPV6_FLOW_LABEL")).c_str());
               ipFilter->setIPv6Info(info);
         } break;
         default: {
            std::cout <<"Error: Unrecognized Ip Family used .. exiting app" <<std::endl;
            return 1;
         } break;
      }

      // [7] Populate Protocol information
      switch (proto) {
         case 6: {   // TCP
            telux::data::TcpInfo tcpInfo;
            tcpInfo.src.port = (uint16_t)protSrcPort;
            tcpInfo.src.range = (uint16_t)protSrcRange;
            tcpInfo.dest.port = (uint16_t)protDestPort;
            tcpInfo.dest.range = (uint16_t)protDestRange;
            auto tcpFilter = std::dynamic_pointer_cast<telux::data::ITcpFilter>(ipFilter);
            if(tcpFilter) {
                tcpFilter->setTcpInfo(tcpInfo);
            }
         } break;
         case 17: {  //UDP
            telux::data::UdpInfo info;
            info.src.port = (uint16_t)protSrcPort;
            info.src.range = (uint16_t)protSrcRange;
            info.dest.port = (uint16_t)protDestPort;
            info.dest.range = (uint16_t)protDestRange;
            auto udpFilter = std::dynamic_pointer_cast<telux::data::IUdpFilter>(ipFilter);
            if(udpFilter) {
                udpFilter->setUdpInfo(info);
            }
         } break;
         default: {
         } break;
      }

      // [8] Instantiate add firewall entry callback instance - this is optional
      auto respCb = [](telux::common::ErrorCode error) {
         std::cout << std::endl << std::endl;
         std::cout << "CALLBACK: "
                  << "addFirewallEntry Response"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error) << std::endl;
                  promise.set_value(1);
      };

      std::future<int> future = promise.get_future();
      dataFwMgr->addFirewallEntry(profileId, fwEntry, respCb);

      // [9] Wait for callback - this is optional
      int tmp = future.get();
   } else {
      std::cout << "\n Invalid argument!!! \n\n";
      std::cout << "\n Sample command is: \n";
      std::cout << "\n\t ./fwlEntry_sample_app <configuration file>";
      std::cout << std::endl;
      std::cout << "\n\t\t Configuration File Parameters";
      std::cout << "\n\t\t OPERATION_TYPE  (0-LOCAL, 1-REMOTE)";
      std::cout << "\n\t\t PROFILE_ID      modem profile id to add firewall entry on";
      std::cout << "\n\t\t DIRECTION       (1-Uplink, 2-Downlink)";
      std::cout << "\n\t\t PROTOCOL        (TCP, UDP)";
      std::cout << "\n\t\t IP_FAMILY       (4-IPv4, 6-IPv6)";
      std::cout << "\n\t\t SOURCE_ADDR     ip address in ipv4 or ipv6 format based on ip family";
      std::cout << "\n\t\t DEST_ADDR       ip address in ipv4 or ipv6 format based on ip family";
      std::cout << "\n\t\t IPV4_SRC_SUBNET_MASK    For Ipv4 Only";
      std::cout << "\n\t\t IPV4_DEST_SUBNET_MASK   For Ipv4 Only";
      std::cout << "\n\t\t IPV4_SERVICE_TYPE       For Ipv4 Only";
      std::cout << "\n\t\t IPV4_SERVICE_TYPE_MASK  For Ipv4 Only";
      std::cout << "\n\t\t IPV6_TRAFFIC_CLASS      For Ipv6 Only";
      std::cout << "\n\t\t IPV6_TRAFFIC_CLASS_MASK For Ipv6 Only";
      std::cout << "\n\t\t IPV6_FLOW_LABEL         For Ipv6 Only";
      std::cout << "\n\t\t PROTOCOL_SRC_PORT       Source port number";
      std::cout << "\n\t\t PROTOCOL_SRC_RANGE      Source port range";
      std::cout << "\n\t\t PROTOCOL_DEST_PORT      Destination port number";
      std::cout << "\n\t\t PROTOCOL_DEST_RANGE     Destination port range";
      std::cout << std::endl;
      std::cout << "\n\t   ./fwlEntry_sample_app firewall_config.conf";
   }

   // [7] Cleaning up and exit the application
   std::cout << "\n\nPress ENTER to exit!!! \n\n";
   std::cin.ignore();

   return 0;
}
