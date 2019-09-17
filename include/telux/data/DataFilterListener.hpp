/*
 *  Copyright (c) 2019, The Linux Foundation. All rights reserved.
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
 * @file       DataFilterListener.hpp
 *
 * @brief      DataFilterListener provides callback methods for listening to Data powersave
 *             filtering mode notifications, like Data Filter filtering mode change.
 *             Client need to implement these methods.
 *             The methods in listener can be invoked from multiple threads. So the client needs to
 *             make sure that the implementation is thread-safe.
 *
 * @note       Eval: This is a new API and is being evaluated. It is subject to change and could
 *             break backwards compatibility.
 */

#ifndef DATAFILTERLISTENER_HPP
#define DATAFILTERLISTENER_HPP

#include <memory>

#include <telux/common/CommonDefines.hpp>
#include <telux/data/DataDefines.hpp>
namespace telux {
namespace data {

/** @addtogroup telematics_data
 * @{ */

/**
 * @brief Listener class for listening to filtering mode notifications,
 *        like Data filtering mode change. Client need to implement these methods.
 *        The methods in listener can be invoked from multiple threads. So the client needs to
 *        make sure that the implementation is thread-safe.
 *
 * @note  Eval: This is a new API and is being evaluated. It is subject to change
 *        and could break backwards compatibility.
 */
class IDataFilterListener : public common::IServiceStatusListener {
public:
    /**
     * This function is called when the data filtering mode is changed
     * for the packet data session.
     *
     * @param [in] state the current data filter mode
     *
     * @note     Eval: This is a new API and is being evaluated. It is subject to change and could
     *           break backwards compatibility.
     */
    virtual void onDataRestrictModeChange(DataRestrictMode mode) {}

    /**
     * Destructor of IDataFilterListener
     */
    virtual ~IDataFilterListener() {}
};

/** @} */ /* end_addtogroup telematics_data */

} // end of namespace data
} // end of namespace telux

#endif // DATAFILTERLISTENER_HPP
