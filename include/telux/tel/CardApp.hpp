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

/**
 * @file       CardApp.hpp
 * @brief      CardApp class is used to represent a single card Application on a SIM card.
 *
 */

#ifndef CARDAPP_HPP
#define CARDAPP_HPP

#include <string>

#include <telux/tel/CardDefines.hpp>

namespace telux {

namespace tel {

/** @addtogroup telematics_card
 * @{ */

/**
 * @brief Represents a single card application.
 */

class ICardApp {
public:
   /**
    * Get Application type like SIM, USIM, RUIM, CSIM or ISIM.
    *
    * @returns @ref AppType.
    */
   virtual AppType getAppType() = 0;

   /**
    * Get Application state like PIN1, PUK required and others.
    *
    * @returns @ref AppState.
    */
   virtual AppState getAppState() = 0;

   /**
    * Get application identifier.
    *
    * @returns Application Id.
    */
   virtual std::string getAppId() = 0;

   virtual ~ICardApp(){};
};
/** @} */ /* end_addtogroup telematics_card */

}  // End of namespace tel

}  // End of namespace telux

#endif  // CARDAPP_HPP
