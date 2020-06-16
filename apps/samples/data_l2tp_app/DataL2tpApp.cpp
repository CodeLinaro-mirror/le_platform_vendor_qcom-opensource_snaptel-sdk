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
#include <telux/data/net/L2tpManager.hpp>
#include "ConfigParser.hpp"


/**
 * @file: DataL2tpApp.cpp
 *
 * @brief: Simple application to enable/disable L2TP un-managed tunnels and add new tunnel
 *         ./l2tp_sample_app <configuration file>
 * @note: Please follow instructions in readme.txt file located in same folder as this app
 *        to setup L2TP mode and associated VLAN before running this app.
 */

#define OPERATION_SUCCESS 1

int main(int argc, char *argv[]) {
   std::promise<int> promise;

   if(argc == 2) {
      // [1] Get the DataFactory and L2tp Manager instance
      auto &dataFactory = telux::data::DataFactory::getInstance();
      auto dataL2tpMgr  = dataFactory.getL2tpManager();

      // [2] Check if data subsystem is ready
      bool subSystemStatus = dataL2tpMgr->isSubsystemReady();

      // [2.1] If data subsystem is not ready, wait for it to be ready
      if(!subSystemStatus) {
         std::cout << "L2tp subsystem is not ready" << std::endl;
         std::cout << "wait unconditionally for it to be ready " << std::endl;
         std::future<bool> f = dataL2tpMgr->onSubsystemReady();
         // If we want to wait unconditionally for data subsystem to be ready
         subSystemStatus = f.get();
      }

      // [3] Exit the application, if SDK is unable to initialize L2tp subsystems
      if(subSystemStatus) {
         std::cout << " *** L2tp Subsystem is Ready *** " << std::endl;
      } else {
         std::cout << " *** ERROR - Unable to initialize L2tp subsystem *** " << std::endl;
         return 1;
      }

      std::string configFile = argv[1];
      std::shared_ptr<ConfigParser> configParser = std::make_shared<ConfigParser>(configFile);
      bool setConfigPass = false;
      bool enable = false;
      int param = std::atoi(configParser->getValue(std::string("L2TP_ENABLE")).c_str());
      if (param) {
         enable = true;
      }
      bool enableMss = false;
      param = std::atoi(configParser->getValue(std::string("TCP_MSS_ENABLE")).c_str());
      if (param) {
         enableMss = true;
      }
      bool enableMtu = false;
      param = std::atoi(configParser->getValue(std::string("MTU_SIZE_ENABLE")).c_str());
      if (param) {
         enableMtu = true;
      }
      int mtuSize = std::atoi(configParser->getValue(std::string("MTU_SIZE_BYTES")).c_str());

      // [4] Instantiate setConfig callback instance - this is optional
      auto setConfigCb = [&setConfigPass, &promise](telux::common::ErrorCode error) {
         std::cout << std::endl << std::endl;
         std::cout << "CALLBACK: "
                   << "setConfig Response"
                   << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                   << ". ErrorCode: " << static_cast<int>(error) << "\n";
         if (error == telux::common::ErrorCode::SUCCESS) {setConfigPass = true;}
         promise.set_value(OPERATION_SUCCESS);
      };

      std::future<int> future = promise.get_future();
      // [5] Set L2TP Configuration
      dataL2tpMgr->setConfig(enable, enableMss, enableMtu, setConfigCb, mtuSize);

      // [6] Wait for setConfig callback - this is optional
      int ret = future.get();

      // [7] Configure L2TP Tunnel
      if (setConfigPass) {
         std::cout << "L2TP Set Configuration succeeded ... Adding Tunnel" << std::endl;
         telux::data::net::L2tpTunnelConfig l2tpTunnelConfig;
         l2tpTunnelConfig.locIface = configParser->getValue(std::string("HW_IF_NAME"));
         l2tpTunnelConfig.prot = static_cast<telux::data::net::L2tpProtocol>(
            std::atoi(configParser->getValue(std::string("ENCAP_PROTOCOL")).c_str()));
         l2tpTunnelConfig.locId =
             std::atoi(configParser->getValue(std::string("LOCAL_TUNNEL_ID")).c_str());
         l2tpTunnelConfig.peerId =
             std::atoi(configParser->getValue(std::string("PEER_TUNNEL_ID")).c_str());
         l2tpTunnelConfig.localUdpPort =
             std::atoi(configParser->getValue(std::string("LOCAL_UDP_PORT")).c_str());
         l2tpTunnelConfig.peerUdpPort =
             std::atoi(configParser->getValue(std::string("PEER_UDP_PORT")).c_str());
         l2tpTunnelConfig.ipType =  static_cast<telux::data::IpFamilyType>(
             std::atoi(configParser->getValue(std::string("PEER_IP_FAMILY")).c_str()));
         l2tpTunnelConfig.peerIpv6Addr =  configParser->getValue(std::string("PEER_IP_ADDRESS"));
         telux::data::net::L2tpSessionConfig l2tpSessionConfig;
         l2tpSessionConfig.locId = 1;
         l2tpSessionConfig.peerId = 1;
         l2tpTunnelConfig.sessionConfig.emplace_back(l2tpSessionConfig);

         promise = std::promise<int>();
         // [8] Instantiate addTunnel callback instance - this is optional
         auto addTunnelCb = [&setConfigPass, &promise](telux::common::ErrorCode error) {
            std::cout << std::endl << std::endl;
            std::cout << "CALLBACK: "
                     << "addTunnel Response"
                     << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                     << ". ErrorCode: " << static_cast<int>(error) << "\n";
            if (error == telux::common::ErrorCode::SUCCESS) {setConfigPass = true;}
            promise.set_value(OPERATION_SUCCESS);
         };

         // [9] addTunnel to L2TP
         std::future<int> future = promise.get_future();
         dataL2tpMgr->addTunnel(l2tpTunnelConfig, addTunnelCb);

         // [10] Wait for addTunnel callback - this is optional
         ret = future.get();
      }
      else {
         std::cout << "L2TP Set Configuration failed ..." << std::endl;
      }
   } else {
      std::cout << "\n Invalid argument!!! \n\n";
      std::cout << "\n Sample command is: \n";
      std::cout << "\n\t ./l2tp_sample_app <configuration file>";
      std::cout << std::endl;
      std::cout << "\n\t\t Configuration File Parameters";
      std::cout << "\n\t\t L2TP_ENABLE        (0-DISABLE, 1-ENABLE)";
      std::cout << "\n\t\t TCP_MSS_ENABLE     (0-DISABLE, 1-ENABLE)";
      std::cout << "\n\t\t MTU_SIZE_ENABLE    (0-DISABLE, 1-ENABLE)";
      std::cout << "\n\t\t MTU_SIZE_BYTE      MTU size in bytes, if MTU size is enabled";
      std::cout << "\n\t\t HW_IF_NAME         eth0.x where x is Vlan Id";
      std::cout << "\n\t\t ENCAP_PROTOCOL     (0-IP, 1-UDP)";
      std::cout << "\n\t\t PEER_IP_FAMILY     (4-IPv4, 6-IPv6)";
      std::cout << "\n\t\t PEER_IP_ADDRESS    ip address in ipv4 or ipv6 format based on ip family";
      std::cout << "\n\t\t LOCAL_TUNNEL_ID    Local Tunnel Id";
      std::cout << "\n\t\t PEER_TUNNEL_ID     Peer Tunnel Id";
      std::cout << "\n\t\t LOCAL_UDP_PORT     Local UDP port if UDP encapsulation protocol is used";
      std::cout << "\n\t\t PEER_UDP_PORT      Peer UDP port if UDP encapsulation protocol is used";
      std::cout << std::endl;
      std::cout << "\n\t ./l2tp_sample_app DataL2tpApp.conf";
   }

   // [7] Cleaning up and exit the application
   std::cout << "\n\nPress ENTER to exit!!! \n\n";
   std::cin.ignore();

   return 0;
}
