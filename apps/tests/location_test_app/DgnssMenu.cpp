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
 * Changes from Qualcomm Technologies, Inc. are provided under the following license:
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <chrono>
#include <future>
#include <iostream>
#include <memory>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/select.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>
#include <cstring>
#include <csignal>
#include <telux/loc/LocationFactory.hpp>
#include "ConfigParser.hpp"
#include "DgnssMenu.hpp"

#define RESP_BUFFER_SIZE    1032
#define SOCKET_READ_TO    5
#define RETRY_COUNT    5
#define ACK_STRING "ICY 200 OK\r\n"
#define ACK_ICY_PREFIX "ICY 200 OK"
#define ACK_HTTP_PREFIX "HTTP/1.1 200 OK"
#define ACK_HTTP10_PREFIX "HTTP/1.0 200 OK"

uint8_t truncBuffer[RESP_BUFFER_SIZE];
bool append = false;
int appendOffset = 0;

using namespace telux::common;

static inline void printSysErr(const char *what) {
    std::cout << what << " failed: errno=" << errno
              << " (" << std::strerror(errno) << ")" << std::endl;
}

// RAII fd wrapper and socket connect helpers
namespace {
class UniqueFd {
    int fd_ = -1;
public:
    UniqueFd() = default;
    explicit UniqueFd(int fd) : fd_(fd) {}

    UniqueFd(const UniqueFd&) = delete;
    UniqueFd &operator=(const UniqueFd&) = delete;

    UniqueFd(UniqueFd &&other) noexcept : fd_(other.fd_) { other.fd_ = -1; }
    UniqueFd &operator=(UniqueFd &&other) noexcept {
        if (this != &other) {
            if (fd_ >= 0) ::close(fd_);
            fd_ = other.fd_;
            other.fd_ = -1;
        }
        return *this;
    }

    ~UniqueFd() { if (fd_ >= 0) ::close(fd_); }

    int get() const { return fd_; }
    explicit operator bool() const { return fd_ >= 0; }

    int release() {
        int out = fd_;
        fd_ = -1;
        return out;
    }

    void reset(int fd = -1) {
        if (fd_ >= 0) ::close(fd_);
        fd_ = fd;
    }
};

static UniqueFd connectIpv4(const sockaddr_in &server) {
    UniqueFd s(::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));
    if (!s) {
        printSysErr("socket");
        return {};
    }
    int ret = ::connect(s.get(), reinterpret_cast<const sockaddr *>(&server), sizeof(server));
    if (ret != 0) {
        printSysErr("connect");
        return {};
    }
    return s;
}

static UniqueFd connectAnyAddrinfo(struct addrinfo *res) {
    for (auto *rp = res; rp != nullptr; rp = rp->ai_next) {
        UniqueFd s(::socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol));
        if (!s) {
            continue;
        }
        if (::connect(s.get(), rp->ai_addr, rp->ai_addrlen) == 0) {
            return s;
        }
        printSysErr("connect");
        // s auto-closes on destruction
    }
    return {};
}
} // namespace

DgnssMenu::DgnssMenu(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor),
     dgnssSourceType_(DgnssSourceType::FILE_SOURCE),
     dataFormat_(DgnssDataFormat::DATA_FORMAT_UNKNOWN) {
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
        std::promise<ServiceStatus> prom = std::promise<ServiceStatus>();
        auto &locationFactory = LocationFactory::getInstance();
        dgnssManager = locationFactory.getDgnssManager(dataFormat_,
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

    while (true) {
        std::cout
            << "Enter Dgnss Data Format:\n"
            << "  1 - DATA_FORMAT_RTCM_3\n"
            << "  2 - DATA_FORMAT_3GPP_RTK_R15\n"
            << "  3 - DATA_FORMAT_RTX\n"
            << "Selection: ";

        std::string usrInput;
        std::getline(std::cin, usrInput);

        if (usrInput == "1") {
            dataFormat_ = DgnssDataFormat::DATA_FORMAT_RTCM_3;
            break;
        } else if (usrInput == "2") {
            dataFormat_ = DgnssDataFormat::DATA_FORMAT_3GPP_RTK_R15;
            break;
        } else if (usrInput == "3") {
            dataFormat_ = DgnssDataFormat::DATA_FORMAT_RTX;
            break;
        } else {
            std::cout << "Invalid input. Please enter 1, 2, or 3.\n\n";
        }
    }

    telux::common::Status status = initDgnssManager(dgnssManager_);
    if (status != telux::common::Status::SUCCESS) {
        return -1;
    }

    std::shared_ptr<ConsoleAppCommand> injectFromFileCommand =
        std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
            "1", "Inject_From_File", {},
            std::bind(&DgnssMenu::injectFromFile, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> injectFromServerCommand =
        std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
            "2", "Inject_From_Server", {},
            std::bind(&DgnssMenu::injectFromServer, this, std::placeholders::_1)));

    std::vector<std::shared_ptr<ConsoleAppCommand>> commandsListDgnssSubMenu =
        {injectFromFileCommand, injectFromServerCommand};

    addCommands(commandsListDgnssSubMenu);
    ConsoleApp::displayMenu();

    return 0;
}
int DgnssMenu::processRtxFromServer(void) {
   uint8_t buffer[RESP_BUFFER_SIZE];
   int ret;
   // Blocking recv with EINTR handling
   do {
       ret = recv(ntcSocketFd_, buffer, sizeof(buffer), 0);
   } while (ret < 0 && errno == EINTR);

   if (ret < 0) {
       printSysErr("processRtxFromServer recv");
       return ret;
   }
   if (ret == 0) {
       std::cout << "processRtxFromServer: peer closed connection (EOF)" << std::endl;
       reconnect_ = true;
       return 0;
   }
   std::cout << "Injecting RTX length=" << ret << std::endl;
   return (dgnssManager_->injectCorrectionData(buffer, ret) == telux::common::Status::SUCCESS)
          ? ret : -1;
}

int DgnssMenu::processRtcmFromServer(void) {
   int i, length;
   uint8_t buffer[RESP_BUFFER_SIZE];
   static int msg_type;
   int ret;

   memset(buffer, 0, sizeof(buffer));
   // Blocking recv with EINTR handling
   do {
       ret = recv(ntcSocketFd_, buffer, sizeof(buffer), 0);
   } while (ret < 0 && errno == EINTR);

   if (ret < 0) {
       printSysErr("processRtcmFromServer recv");
       stop_ = true;
       return ret;
   }

   if (ret == 0) {
       std::cout << "processRtcmFromServer: peer closed connection (EOF)" << std::endl;
       reconnect_ = true;
       return 0;
   }

   if (append) {
       int totalSize = appendOffset + ret;
       // Avoid overflow of truncBuffer
       if (totalSize > (int)sizeof(truncBuffer)) {
           std::cout << "RTCM truncated buffer overflow, total=" << totalSize
                     << " > " << sizeof(truncBuffer) << std::endl;
           append = false;
           appendOffset = 0;
           return -1;
       }
       memcpy(truncBuffer + appendOffset, buffer, ret);
       std::cout << "Injecting msg_type=" << msg_type << " length=" << totalSize << std::endl;
       if (telux::common::Status::SUCCESS !=
                   dgnssManager_->injectCorrectionData(truncBuffer, totalSize)) {
           // Reset append state on error to avoid stale accumulation
           append = false;
           appendOffset = 0;
           return -1;
       }
       append = false;
       appendOffset = 0;
   } else {
       for (i = 0; i < (ret - 4);) {
           if ((buffer[i] == 0xD3) && ((buffer[i+1] & 0xFC) == 0x00)) {
               //Found RTCM preamble.
               length = buffer[i + 2];
               length |= (buffer[i + 1] & 0x03) << 8;
               // Validate RTCM payload length (10-bit, max 1023)
               if (length > 1023) {
                   std::cout << "Invalid RTCM length=" << length << " at offset " << i << std::endl;
                   i += 1;
                   continue;
               }
               msg_type = buffer[i + 3] << 4;
               msg_type |= buffer[i + 4] >> 4;

               if (ret < length + i + 6) {
                   //didn't read the whole packet, portion will be available for next recv call
                   //save current packet to truncBuffer and append it to next recv'd bytes.
                   memset(truncBuffer, 0, sizeof(truncBuffer));
                   memcpy(truncBuffer, buffer + i, ret - i);
                   append = true;
                   appendOffset = ret - i;
                   i += appendOffset;
               } else {
                   std::cout << "Injecting msg_type=" << msg_type << " length=" << length+6 << std::endl;
                   if (telux::common::Status::SUCCESS !=
                           dgnssManager_->injectCorrectionData(buffer + i, length + 6)) {
                       // Reset append state on error to avoid stale accumulation
                       append = false;
                       appendOffset = 0;
                       return -1;
                   }
                   i += length + 6;
               }
           } else {
               i += 1;
           }
       }
   }
   return ret;
}
/* process input file
 * @returns: 0 succesfully read and injected one line.
 *           1 EOF reached.
 *           -1 injection failed.
 */
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
      return 1;
   }
   std::cout << "Injecting data.." << std::endl;
   if (telux::common::Status::SUCCESS == dgnssManager_->injectCorrectionData(buffer, size)) {
       ret = 0;
   } else {
       std::cout << "Injection failed from file" << std::endl;
       close(dgnssSourceFd_);
       ret = -1;
   }

   return ret;
}
int DgnssMenu::processRtxFromFile(void) {
   uint8_t buffer[RESP_BUFFER_SIZE];
   ssize_t n = read(dgnssSourceFd_, buffer, sizeof(buffer));
   if (n == 0) {
      std::cout << "End of file reached" << std::endl;
      close(dgnssSourceFd_);
      return 1;
   }
   if (n < 0) {
      std::cout << "Read failed from file, errno=" << errno
                << " (" << std::strerror(errno) << ")" << std::endl;
      close(dgnssSourceFd_);
      return -1;
   }
   std::cout << "Injecting RTX (file) length=" << n << std::endl;
   return (dgnssManager_->injectCorrectionData(buffer, n) == telux::common::Status::SUCCESS)
          ? 0 : -1;
}
/* This funciton is invoked asynchronously in a seperate thread */
void DgnssMenu::onDgnssStatusUpdate(DgnssStatus status) {
   switch(status) {
       case DgnssStatus::DATA_SOURCE_NOT_SUPPORTED:
         std::cout << "RTCM data soure is not supported" << std::endl;
         break;
       case DgnssStatus::DATA_FORMAT_NOT_SUPPORTED:
         std::cout << "RTCM data format is not supported" << std::endl;
         break;
       case DgnssStatus::OTHER_SOURCE_IN_USE:
         std::cout << "RTCM other source is in use" << std::endl;
         break;
       case DgnssStatus::MESSAGE_PARSE_ERROR:
         std::cout << "RTCM message parsing error" << std::endl;
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
                ret = (dataFormat_ == DgnssDataFormat::DATA_FORMAT_RTX)
                      ? processRtxFromFile()
                      : processRtcmFromFile();
                sleep(1);
            }
            if (ret > 0) {
                break;
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
                ret = 0;
            } else {
                break;  //injection failure due to other reason, quit.
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
   int ret;
   std::string con_request;
   std::string configFile;
   char response[RESP_BUFFER_SIZE];
   char delimiter = '\n';
   std::signal(SIGPIPE, SIG_IGN);

   if(dgnssManager_) {
      dgnssSourceType_ = DgnssSourceType::SERVER_SOURCE;
      std::cout << "Input config file name::" << std::endl;
      while (configFile.empty()) {
          std::getline(std::cin, configFile, delimiter);
      }

      // parse the config file
      ConfigParser config(configFile);

      while(stop_ == false) {
          struct addrinfo hints;
          struct addrinfo *res = nullptr;
          memset(&hints, 0, sizeof(hints));
          hints.ai_family = AF_INET;
          hints.ai_socktype = SOCK_STREAM;
          hints.ai_protocol = IPPROTO_TCP;

          std::string host = config.getValue("hostName");
          std::string port = config.getValue("Port");

          int gaiErr = getaddrinfo(host.c_str(), port.c_str(), &hints, &res);
          if (gaiErr != 0) {
              std::cout << "getaddrinfo failed: " << gai_strerror(gaiErr) << std::endl;
              // Fallback: resolve via gethostbyname or inet_aton for IPv4
              struct sockaddr_in server;
              memset(&server, 0, sizeof(server));
              server.sin_family = AF_INET;
              server.sin_port = htons(static_cast<uint16_t>(std::stoi(port)));
              struct hostent *he = gethostbyname(host.c_str());
              if (he != nullptr && he->h_addr_list && he->h_addr_list[0]) {
                  memcpy(&server.sin_addr, he->h_addr_list[0], sizeof(server.sin_addr));
              } else if (inet_aton(host.c_str(), &server.sin_addr) == 0) {
                  std::cout << "DNS resolution failed for " << host << std::endl;
                  std::cout << "connection failed, retry after " << RETRY_COUNT << "sec" << std::endl;
                  usleep(RETRY_COUNT * 1000000);
                  continue;
              }
              // Attempt to connect using fallback IPv4 address (centralized RAII)
              std::cout << "Connecting to server..." << std::endl;
              auto s = connectIpv4(server);
              if (!s) {
                  std::cout << "connection failed, retry after " << RETRY_COUNT << "sec" << std::endl;
                  usleep(RETRY_COUNT * 1000000);
                  continue;
              } else {
                  std::cout << "connection success" << std::endl;
                  ntcSocketFd_ = s.release(); // transfer ownership
              }
          } else {
              ntcSocketFd_ = -1;
              std::cout << "Connecting to server..." << std::endl;
              auto s = connectAnyAddrinfo(res);
              if (s) {
                  std::cout << "connection success" << std::endl;
                  ntcSocketFd_ = s.release(); // transfer ownership
              }
          }

          if (res) {
              freeaddrinfo(res);
          }

          if (ntcSocketFd_ < 0) {
              std::cout << "connection failed, retry after " << RETRY_COUNT << "sec" << std::endl;
              usleep(RETRY_COUNT * 1000000);
              continue;
          }

          memset(&response, 0 , sizeof(response));
          con_request = "GET /" + config.getValue("mountPoint") +
              " HTTP/1.1\r\n" +
              "Host: " + host + ":" + port + "\r\n" +
              "User-Agent: NTRIP GNR/1.0.0 (Win32)\r\n" +
              "Ntrip-Version: Ntrip/2.0\r\n" +
              "Authorization: Basic " +
              config.getValue("userNamePwdInBase64Format") +
              "\r\nConnection: keep-alive\r\n\r\n";
          ret = send(ntcSocketFd_, con_request.c_str(), con_request.size(), MSG_NOSIGNAL);
          std::cout << "Sending request: " << con_request << std::endl;
          if (ret < 0) {
              printSysErr("send");
              close(ntcSocketFd_);
              ntcSocketFd_ = -1;
              continue;
          }

          // Blocking recv with EINTR handling for initial response
          do {
              ret = recv(ntcSocketFd_, response, sizeof(response), 0);
          } while (ret < 0 && errno == EINTR);

          if (ret < 0) {
              printSysErr("recv");
              close(ntcSocketFd_);
              ntcSocketFd_ = -1;
              continue;
          } else if (ret == 0) {
              std::cout << "recv: peer closed connection (EOF) after request" << std::endl;
              close(ntcSocketFd_);
              ntcSocketFd_ = -1;
              continue;
          } else if (ret > 0 && (
                     !strncmp(ACK_STRING, (char*)response, 12) ||
                     strncmp((char*)response, ACK_HTTP_PREFIX, strlen(ACK_HTTP_PREFIX)) == 0 ||
                     strncmp((char*)response, ACK_HTTP10_PREFIX, strlen(ACK_HTTP10_PREFIX)) == 0)) {
              // register status listener
              dgnssManager_->registerListener(shared_from_this());

              // Please refer to injectFromFile() for alternative use case sample.
              // Keep streaming: do not break on a single timeout/EAGAIN.
              for (;;) {
                  ret = (dataFormat_ == DgnssDataFormat::DATA_FORMAT_RTX)
                        ? processRtxFromServer()
                        : processRtcmFromServer();
                  if (reconnect_) {
                      close(ntcSocketFd_);
                      ntcSocketFd_ = -1;
                      reconnect_ = false;
                      break; // reconnect
                  }
                  if (ret < 0) {
                      // error while reading; reconnect
                      close(ntcSocketFd_);
                      ntcSocketFd_ = -1;
                      break;
                  }
                  // ret == 0 => timeout/no data; continue waiting
              }
              continue;
          } else {
              if (std::strstr(response, "SOURCETABLE") != nullptr) {
                  std::cout << "Received SOURCETABLE – check mountPoint/credentials.\n" << response << std::endl;
              } else {
                  std::cout << "Initial response invalid: " << response << std::endl;
              }
              close(ntcSocketFd_);
              ntcSocketFd_ = -1;
              return;
          }
      }
   }
}
