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

#include <chrono>
#include <future>
#include <iostream>
#include <memory>
#include <sstream>
#include <algorithm>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include <telux/loc/LocationFactory.hpp>
#include "ConfigParser.hpp"
#include "DgnssMenu.hpp"

#define RESP_BUFFER_SIZE    1032
#define ACK_STRING "ICY 200 OK\r\n"

using namespace telux::common;

DgnssMenu::DgnssMenu(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
}

DgnssMenu::~DgnssMenu() {
   if(dgnssManager_) {
      dgnssManager_->deRegisterListener();
      dgnssManager_ = nullptr;
   }
}

telux::common::Status DgnssMenu::initDgnssManager(std::shared_ptr<IDgnssManager>
        &dgnssManager) {
    if(dgnssManager == nullptr) {
        std::promise<ServiceStatus> prom{};
        auto &locationFactory = LocationFactory::getInstance();
        dgnssManager = locationFactory.getDgnssManager(DgnssDataFormat::DATA_FORMAT_RTCM_3,
            [&](ServiceStatus status) {
                if (status == ServiceStatus::SERVICE_AVAILABLE) {
                    prom.set_value(ServiceStatus::SERVICE_AVAILABLE);
                } else {
                    prom.set_value(ServiceStatus::SERVICE_FAILED);
                }
            });
        if (!dgnssManager) {
            std::cout << "Failed to get Gnss manager object" << std::endl;
            return Status::FAILED;
        }
        // The dgnssManager object is associated with a default source which support
        // injection of RCTM3 format data.
        std::chrono::time_point<std::chrono::system_clock> startTime, endTime;
        startTime = std::chrono::system_clock::now();
        ServiceStatus dgnssMgrStatus = dgnssManager->getServiceStatus();
        if(dgnssMgrStatus != ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "Dgnss subsystem is not ready, Please wait" << std::endl;
        }
        dgnssMgrStatus = prom.get_future().get();
        if(dgnssMgrStatus == ServiceStatus::SERVICE_AVAILABLE) {
            endTime = std::chrono::system_clock::now();
            std::chrono::duration<double> elapsedTime = endTime - startTime;
            std::cout << "Elapsed Time for Dgnss subsystems to ready : "
                << elapsedTime.count() << "s\n"  << std::endl;
        } else {
            std::cout << "ERROR - Unable to initialize Dgnss subsystem" << std::endl;
            return telux::common::Status::NOTREADY;
        }
   } else {
       std::cout<< "Dgnss manager is already initialized" << std::endl;
   }
   return telux::common::Status::SUCCESS;
}

int DgnssMenu::init() {
   std::shared_ptr<ConsoleAppCommand> injectFromFileCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "1", "Inject_From_File", {},
         std::bind(&DgnssMenu::injectFromFile, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> injectFromServerCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "2", "Inject_From_Server", {},
         std::bind(&DgnssMenu::injectFromServer, this, std::placeholders::_1)));

   std::vector<std::shared_ptr<ConsoleAppCommand>> commandsListDgnssSubMenu
      = {injectFromFileCommand, injectFromServerCommand};
   addCommands(commandsListDgnssSubMenu);
   ConsoleApp::displayMenu();

   telux::common::Status status = telux::common::Status::FAILED;
   int rc = 0;
   status = initDgnssManager(dgnssManager_);
   if (status != telux::common::Status::SUCCESS) {
       rc = -1;
   }
   return rc;
}

int DgnssMenu::processRtcmFromServer(void) {
   int i;
   int length, msg_type;
   uint8_t buffer[RESP_BUFFER_SIZE];
   int bufferSize = RESP_BUFFER_SIZE;
   ssize_t ret;

   memset(buffer, 0, bufferSize);
   ret = recv(ntcSocketFd_, buffer, (size_t)bufferSize, 0);
   if (ret < 0) {
      std::cout << "ReadRtcmPacket: recv failed " << ret << std::endl;
      return ret;
   }
   bufferSize = (int)ret;
   for (i = 0; i < (bufferSize - 4);) {
     if ((buffer[i] == 0xD3) && ((buffer[i+1] & 0xFC) == 0x00)) {
       //Found RTCM preamble.
       length = buffer[i + 2];
       length |= (buffer[i + 1] & 0x03) << 8;
       msg_type = buffer[i + 3] << 4;
       msg_type |= buffer[i + 4] >> 4;

       std::cout << "Injecting msg_type=" << msg_type << " length=" << length+6 << std::endl;
       if (telux::common::Status::SUCCESS !=
               dgnssManager_->injectCorrectionData(buffer + i, length + 6)) {
         return -1;
       }
       i += length + 6;
     } else {
       i += 1;
     }
   }
   return 0;

}

int DgnssMenu::processRtcmFromFile(void) {
   int size = 0;
   int ret;
   uint8_t temp[2];
   uint8_t buffer[RESP_BUFFER_SIZE];
   uint8_t *p = buffer;
   memset(buffer, 0, sizeof(buffer));
   do {
      ret = read(dgnssSourceFd_, temp, 2);
      if (temp[0] == '\r' && temp[1] == '\n') {
         break;
      } else {
         *p = temp[0];
         *(p+1) = temp[1];
         p += 2;
         size += 2;
      }
   } while(size < RESP_BUFFER_SIZE);
   if (ret < 2) {
      std::cout << "End of file reached" << std::endl;
      close(dgnssSourceFd_);
      return -1;
   }
   std::cout << "Injecting data.." << std::endl;
   if (telux::common::Status::SUCCESS == dgnssManager_->injectCorrectionData(buffer, size)) {
       ret = 0;
   } else {
       ret = -1;
   }

   return ret;
}
/* This funciton is invoked asynchronously in a seperate thread */
void DgnssMenu::onDgnssStatusUpdate(DgnssStatus status) {
   switch(status) {
       case DgnssStatus::DATA_SOURCE_NOT_SUPPORTED:
         std::cout << "RTCM data soure is not supported" << std::endl;
         dgnssManager_->releaseSource();
         break;
       case DgnssStatus::DATA_FORMAT_NOT_SUPPORTED:
         std::cout << "RTCM data format is not supported" << std::endl;
         dgnssManager_->releaseSource();
         break;
       case DgnssStatus::OTHER_SOURCE_IN_USE:
         std::cout << "RTCM other source is in use" << std::endl;
         dgnssManager_->releaseSource();
         break;
       case DgnssStatus::MESSAGE_PARSE_ERROR:
         std::cout << "RTCM message parsing error" << std::endl;
         dgnssManager_->releaseSource();
         break;
       case DgnssStatus::DATA_SOURCE_NOT_USABLE:
         std::cout << "RTCM data source is not usable" << std::endl;
         // Demonstrate "source switching" requirement. If current source's data
         // is not usable anymore, another source is picked, but we must call releaseSource()
         // to release current source and createSource() to create a new one.
         dgnssManager_->releaseSource();
         if (dgnssManager_->createSource(DgnssDataFormat::DATA_FORMAT_RTCM_3) !=
                telux::common::Status::SUCCESS) {
            std::cout << "Failed to create RTCM source" << std::endl;
         }
         break;
      default:
         std::cout << "Unknown RTCM status" << std::endl;
   }
}

void DgnssMenu::injectFromFile(std::vector<std::string> userInput) {
    std::string sourceFile;
    char delimiter = '\n';
    int ret = 0;

    if (dgnssManager_) {
        dgnssSourceType_ = DgnssSourceType::FILE_SOURCE;
        std::cout << "Input source file name::" << std::endl;
        while (sourceFile.empty()) {
            std::getline(std::cin, sourceFile, delimiter);
        }

        dgnssSourceFd_ = open(sourceFile.c_str(), S_IRUSR, S_IRUSR);
        if (dgnssSourceFd_ < 0) {
            std::cout << "failed to open file " << sourceFile << std::endl;
            return;
        }
        std::cout << "File opened" << std::endl;
        // register status listener
        dgnssManager_->registerListener(shared_from_this());
        std::cout << "listener registered" << std::endl;
        // a default source(with RTCM3 format) has been created in initDgnssManager()
        // and dgnss subsystem is ready.
        //
        while(1) {
            while(!ret) {
                ret = processRtcmFromFile();
                sleep(1);
            }
            // If error returned and subSystemReady() is false, that means current source
            // has been released from listening function and new source may have been
            // created but not ready to accept data, we wait for it to become ready before
            // injecting data.
            // NOTE that in real case, if this happened the data should come from new source.
            bool subSystemStatus = dgnssManager_->isSubsystemReady();
            if (false == subSystemStatus) {
                std::future<bool> f = dgnssManager_->onSubsystemReady();
                subSystemStatus = f.get();
                if (false == subSystemStatus) {
                    break;
                }
            }
        }
    }
}
/**
 * Config file is needed if injecting from Ntrip caster. The format is:
 *
 * hostName = (IP or host name)
 * Port = (port number)
 * userNamePwdInBase64Format = username and password in Base64 format
 * mountPoint = /mountpoint
 */
void DgnssMenu::injectFromServer(std::vector<std::string> userInput) {
   struct sockaddr_in server_addr;
   int ret;
   std::string con_request;
   std::string configFile;
   char response[RESP_BUFFER_SIZE];
   char delimiter = '\n';

   if(dgnssManager_) {
      dgnssSourceType_ = DgnssSourceType::SERVER_SOURCE;
      std::cout << "Input config file name::" << std::endl;
      while (configFile.empty()) {
          std::getline(std::cin, configFile, delimiter);
      }

      // parse the config file
      ConfigParser config(configFile);
      ntcSocketFd_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
      if (ntcSocketFd_ < 0) {
          std::cout << "Socket create failed" << std::endl;
          return;
      } else {
          std::cout << "Socket create success" << std::endl;
      }
      memset(&server_addr, 0, sizeof(server_addr));
      server_addr.sin_family = AF_INET;
      server_addr.sin_addr.s_addr = inet_addr(config.getValue("hostName").c_str());
      server_addr.sin_port = htons(std::stoi(config.getValue("Port")));

      std::cout << "Connecting to server..." << std::endl;
      ret = connect(ntcSocketFd_, (struct sockaddr *)&server_addr, sizeof(server_addr));
      if (ret < 0) {
          std::cout << "connection failed" << std::endl;
      } else {
          std::cout << "connection success" << std::endl;
      }
      memset(&response, 0 , sizeof(response));
      con_request += "GET /" + config.getValue("mountPoint") +
              " HTTP/1.1\r\n" + "User-Agent: NTRIP GNR/1.0.0 (Win32)\r\n" +
              "Authorization: Basic " +
              config.getValue("userNamePwdInBase64Format") +
              "\r\nConnection: close\r\n";

      ret = send(ntcSocketFd_, con_request.c_str(), con_request.size(), 0);
      std::cout << "Sending request: " << con_request << std::endl;
      if (ret < 0) {
          std::cout << "send failed: " << ret << std::endl;
          return;
      }
      ret = recv(ntcSocketFd_, response, sizeof(response), 0);
      if(ret < 0 ) {
          std::cout << "recv failed" << std::endl;
          return;
      } else if (ret > 0 && !strncmp(ACK_STRING, (char*)response, 12)) {
          // register status listener
          dgnssManager_->registerListener(shared_from_this());

          // Please refer to injectFromFile() for alternative use case sample.
          while (!ret) {
            ret = processRtcmFromServer();
          }
       } else {
          std::cout << "Initial response invalid: " << response << std::endl;
          return;
       }
   }
}
