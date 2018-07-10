/*
 *  Copyright (c) 2017-2018, The Linux Foundation. All rights reserved.
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

/**
 * @file       TelSdkConsoleApp.cpp
 *
 * @brief      This is entry class for console application for Telematics SDK,
 *             It allows one to interactively invoke most of the public APIs in the Telematics SDK.
 */

#include <iostream>
#include <cxxabi.h>

extern "C" {
#include <execinfo.h>
#include <signal.h>
}

#include "Call/CallMenu.hpp"
#include "ECall/ECallMenu.hpp"
#include "Location/LocationMenu.hpp"
#include "Phone/PhoneMenu.hpp"
#include "Sms/SmsMenu.hpp"
#include "SimCardServices/SimCardServicesMenu.hpp"

#include "TelSdkConsoleApp.hpp"

TelSdkConsoleApp::TelSdkConsoleApp(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
}

TelSdkConsoleApp::~TelSdkConsoleApp() {
}

/**
 * Used for creating a menus of high level features
 */
void TelSdkConsoleApp::init() {
   std::shared_ptr<ConsoleAppCommand> phoneMenuCommand = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("1", "Phone_Status", {},
                        std::bind(&TelSdkConsoleApp::phoneMenu, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> callMenuCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "2", "Dialer", {}, std::bind(&TelSdkConsoleApp::callMenu, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> eCallMenuCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "3", "eCall", {}, std::bind(&TelSdkConsoleApp::eCallMenu, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> smsMenuCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "4", "SMS", {}, std::bind(&TelSdkConsoleApp::smsMenu, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> simCardMenuCommand = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("5", "Card_Services", {},
                        std::bind(&TelSdkConsoleApp::simCardMenu, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> locationMenuCommand = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("6", "Location", {},
                        std::bind(&TelSdkConsoleApp::locationMenu, this, std::placeholders::_1)));
   std::vector<std::shared_ptr<ConsoleAppCommand>> mainMenuCommands
      = {phoneMenuCommand, callMenuCommand,    eCallMenuCommand,
         smsMenuCommand,   simCardMenuCommand, locationMenuCommand};

   addCommands(mainMenuCommands);
   ConsoleApp::displayMenu();
}

void TelSdkConsoleApp::phoneMenu(std::vector<std::string> userInput) {
   PhoneMenu phoneMenu("Phone Menu", "phone> ", 1);
   phoneMenu.init();
   phoneMenu.mainLoop();
}

void TelSdkConsoleApp::callMenu(std::vector<std::string> userInput) {
   CallMenu callMenu("Dialer Menu", "dialer> ");
   callMenu.init();
   callMenu.mainLoop();
}

void TelSdkConsoleApp::eCallMenu(std::vector<std::string> userInput) {
   ECallMenu eCallMenu("eCall Menu", "eCall> ");
   eCallMenu.init();
   eCallMenu.mainLoop();
}

void TelSdkConsoleApp::simCardMenu(std::vector<std::string> userInput) {
   SimCardServicesMenu simCardServicesMenu("SIM Card Services Menu", "card_services> ");
   simCardServicesMenu.init();
   simCardServicesMenu.mainLoop();
}

void TelSdkConsoleApp::smsMenu(std::vector<std::string> userInput) {
   SmsMenu smsMenu("SMS Menu", "sms> ", 1);
   smsMenu.init();
   smsMenu.mainLoop();
}

void TelSdkConsoleApp::locationMenu(std::vector<std::string> userInput) {
   LocationMenu locationMenu("Location Menu", "location> ");
   locationMenu.init();
   locationMenu.mainLoop();
}

void signalHandler(int sig) {
   void *array[120];
   size_t size;

   // get void*'s for all entries on the stack
   size = backtrace(array, 120);

   // print out all the frames to stderr
   fprintf(stdout, "Error: signal %d:\n", sig);
   // NOTE: C style symbols are printed by backtrace_symbols_fd, need to demangle manually for c++
   // backtrace_symbols_fd(array, size, STDERR_FILENO);

   if(size == 0) {
      fprintf(stdout, "  <empty, possibly corrupt>\n");
      return;
   }

   // resolve addresses into strings containing "filename(function+address)",
   // this array must be free()-ed
   char **symbolList = backtrace_symbols(array, size);

   // iterate over the returned symbol lines. skip the first, it is the
   // address of this function.
   for(size_t i = 1; i < size; i++) {

      char *beginName = 0, *beginOffset = 0, *endOffset = 0;

      // find parentheses and +address offset surrounding the mangled name:
      // ./module(function+0x15c) [0x8048a6d]
      for(char *p = symbolList[i]; *p; ++p) {
         if(*p == '(')
            beginName = p;
         else if(*p == '+')
            beginOffset = p;
         else if(*p == ')' && beginOffset) {
            endOffset = p;
            break;
         }
      }

      if(beginName && beginOffset && endOffset && beginName < beginOffset) {
         *beginName++ = '\0';
         *beginOffset++ = '\0';
         *endOffset++ = '\0';

         // mangled name is now in [beginName, beginOffset) and caller
         // offset in [beginOffset, endOffset). now apply
         // __cxa_demangle():
         int status;
         size_t funcNameSize = 1024;
         char *funcName = (char *)malloc(funcNameSize);
         char *ret = abi::__cxa_demangle(beginName, funcName, &funcNameSize, &status);
         if(status == 0) {
            funcName = ret;  // use possibly realloc()-ed string
            fprintf(stdout, "  %s : %s+%s%s\n", symbolList[i], funcName, beginOffset, endOffset);
         } else {
            // demangling failed. Output function name as a C function with
            // no arguments.
            fprintf(stdout, "  %s : %s()+%s%s\n", symbolList[i], beginName, beginOffset, endOffset);
         }
         free(funcName);
      } else {
         // couldn't parse the line? print the whole line.
         fprintf(stdout, "  %s\n", symbolList[i]);
      }
   }
   free(symbolList);
   exit(1);
}

void setupSignal() {
   signal(SIGSEGV, signalHandler);
   signal(SIGABRT, signalHandler);
   signal(SIGBUS, signalHandler);
   signal(SIGILL, signalHandler);
   signal(SIGFPE, signalHandler);
   signal(SIGPIPE, signalHandler);
}

// Main function that displays the console and processes user input
int main(int argc, char **argv) {

   setupSignal();

   TelSdkConsoleApp telsdkConsoleApp("Telematics SDK features", "tel_sdk> ");

   telsdkConsoleApp.init();  // initialize commands and display

   return telsdkConsoleApp.mainLoop();  // Main loop to continuously read and execute commands
}
