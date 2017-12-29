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

#include <chrono>
#include <iostream>
#include <iterator>
#include <sstream>

#include <telux/tel/PhoneFactory.hpp>
#include "ConsoleApp.hpp"
#include "ConsoleAppCommand.hpp"

using namespace telux::tel;

ConsoleApp::ConsoleApp(std::string appName, std::string cursor)
   : appName_(appName)
   , cursor_(cursor) {
}

/**
 * Displaying Menu of Applications Requested
 */
void ConsoleApp::displayMenu() {
   displayBanner();
   // Iterate through the supportedCommands_ list and display all the commands
   for(auto command : supportedCommands_) {
      command->displayCommand();
   }
   std::cout << std::endl;
   std::cout << "************** Misc. Commands ******************" << std::endl << std::endl;
   std::cout << "   ? - help" << std::endl;
   std::cout << "   q / 0 - exit" << std::endl << std::endl;
   std::cout << "------------------------------------------------" << std::endl << std::endl;
}

/**
 * Display Cursor to Read User Input
 */
void ConsoleApp::displayCursor() {
   std::cout << cursor_;
}

/**
 * Display the title banner
 */
void ConsoleApp::displayBanner() {
   std::cout << "\n";
   std::cout << "------------------------------------------------" << std::endl;
   std::cout << "                   " << appName_ << std::endl;
   std::cout << "------------------------------------------------" << std::endl << std::endl;
   std::cout << "************** Supported Commands **************" << std::endl << std::endl;
}

/**
 * Read User Request From Command Line
 */
std::vector<std::string> ConsoleApp::readCommand() {
   ConsoleApp::displayCursor();
   std::string command;
   std::getline(std::cin, command);
   std::istringstream iss(command);
   std::vector<std::string> userInput{std::istream_iterator<std::string>{iss},
                                      std::istream_iterator<std::string>{}};
   return userInput;
}

/**
 * Check Whether the request is present in the menu and possible commands
 */
bool ConsoleApp::isValidChoice(std::vector<std::string> inputCommand) {

   for(auto command : supportedCommands_) {
      if(command->getId() == inputCommand[0] || command->getName() == inputCommand[0]) {
         if(command->getArguments().size() == (inputCommand.size() - 1)) {
            return true;
         }
      }
   }
   return false;
}

/**
 * Add  commands into supportCommands_ list
 */
void ConsoleApp::addCommands(std::vector<std::shared_ptr<ConsoleAppCommand>> supportedCommandsList) {
   for(auto command : supportedCommandsList) {
      supportedCommands_.emplace_back(command);
   }
}

/**
 * Initialize sub-systems of SDK
 */
bool ConsoleApp::initializeSDK() {
   //  Get the PhoneFactory and PhoneManager instances.
   auto &phoneFactory = PhoneFactory::getInstance();
   auto phoneManager = phoneFactory.getPhoneManager();

   //  Check if telephony subsystem is ready
   bool subSystemStatus = phoneManager->isSubsystemReady();

   //  If telephony subsystem is not ready, wait for it to be ready
   if(!subSystemStatus) {
      std::future<bool> f = phoneManager->onSubsystemReady();
      // If we want to wait unconditionally for telephony subsystem to be ready
      subSystemStatus = f.get();
   }

   //  Exit the application, if SDK is unable to initialize telephony subsystems
   if(subSystemStatus) {
      return true;
   } else {
      std::cout << " *** ERROR - Unable to initialize telephony subsystem" << std::endl;
      return false;
   }
}

int ConsoleApp::mainLoop() {
   while(true) {
      std::vector<std::string> userInput = readCommand();
      if(userInput.size() == 0)
         continue;

      if(userInput[0] == "0" || userInput[0] == "exit" || userInput[0] == "q") {
         break;
      } else if(userInput[0] == "?" || userInput[0] == "help") {
         displayMenu();
         continue;
      }

      bool choiceIsValid = isValidChoice(userInput);
      if(!choiceIsValid) {  // check the user request is valid or not
         std::cout << "Invalid command: " << userInput[0] << " entered." << std::endl;
         std::cout << "Please enter valid command and arguments." << std::endl;
      } else {  // valid request...
         for(auto command : supportedCommands_) {
            if((command->getId() == userInput[0] || command->getName() == userInput[0])
               && (command->getArguments().size() == (userInput.size() - 1))) {
               // std::cout << "valid operation: " << command->getName()
               //           << std::endl;  // extract operation name ...
               command->executeCommand(userInput);
            }
         }
      }
   }
   return 0;
}
