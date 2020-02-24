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

/**
 * Utility helper class
 * @brief Utils class performs common error code conversions
 */

#ifndef UTILS_HPP
#define UTILS_HPP

#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include <telux/common/CommonDefines.hpp>


class Utils {
public:
   // Validate the input and in case of invalid input request
   // for proper input from user.
   template <typename T>
   static void validateInput(T &input) {
      bool valid = false;
      do {
         if(std::cin.good()) {
            valid = true;
         } else {
            // If an error occurs then an error flag is set and future attempts to get
            // input will fail. Cear the error flag on cin.
            std::cin.clear();
            // Extracts characters from the previous input sequence and discards them,
            // until entire stream have been extracted, or one compares equal to newline.
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "ERROR: Invalid input, please re-enter." << std::endl;
            std::cin >> input;
         }
      } while(!valid);
   }

   // Validate input string(Ex: 1, 2, 3) which should contain
   // atleast one number or numbers seperated by either comma, space or both.
   static void validateNumericString(std::string &input);
   /**
    * Get error description for given ErrorCode
    */
   static std::string getErrorCodeAsString(telux::common::ErrorCode error);
};

#endif
