/*
 *  Copyright (c) 2018-2019, The Linux Foundation. All rights reserved.
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
 * @file       DataFactory.hpp
 *
 * @brief      DataFactory is the central factory to create all data instances
 *
 */

#ifndef DATAFACTORY_HPP
#define DATAFACTORY_HPP

#include <map>
#include <memory>
#include <mutex>

#include <telux/common/CommonDefines.hpp>

#include <telux/data/DataConnectionManager.hpp>
#include <telux/data/DataProfileManager.hpp>
#include <telux/data/DataFilterManager.hpp>
#include <telux/data/IpFilter.hpp>

namespace telux {
namespace data {

/** @addtogroup telematics_data
 * @{ */

/**
 *@brief DataFactory is the central factory to create all data classes
 *
 */
class DataFactory {
 public:
    /**
     * Get Data Factory instance.
     */
    static DataFactory &getInstance();

    /**
     * Get Data Connection Manager
     *
     * @param [in] slotId    Unique identifier for the SIM slot
     *
     * @returns instance of IDataConnectionManager
     *
     */
    std::shared_ptr<IDataConnectionManager> getDataConnectionManager(int slotId = DEFAULT_SLOT_ID);

    /**
     * Get Data Profile Manager
     *
     * @param [in] slotId    Unique identifier for the SIM slot
     *
     * @returns instance of IDataProfileManager
     *
     */
    std::shared_ptr<IDataProfileManager> getDataProfileManager(int slotId = DEFAULT_SLOT_ID);

    /**
     * Get Data Filter Manager instance
     *
     * @param [in] slotId    Unique identifier for the SIM slot
     *
     * @returns instance of IDataFilterManager.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to
     * change and could break backwards compatibility.
     */
    std::shared_ptr<IDataFilterManager> getDataFilterManager(int slotId = DEFAULT_SLOT_ID);

    /**
    * Get IIpFilter instance based on IP Protocol
    *
    * @param [in] proto    @ref telux::data::IpProtocol
    *                      Some sample protocol values are
    *                      ICMP = 1    # Internet Control Message Protocol - RFC 792
    *                      IGMP = 2    # Internet Group Management Protocol - RFC 1112
    *                      TCP = 6     # Transmission Control Protocol - RFC 793
    *                      UDP = 17    # User Datagram Protocol - RFC 768
    *                      ESP = 50    # Encapsulating Security Payload - RFC 4303
    *
    * @returns instance of IIpFilter based on IpProtocol filter (i.e TCP, UDP)
    *
    * @note     Eval: This is a new API and is being evaluated.It is subject to change and could
    *           break backwards compatibility.
    */
    std::shared_ptr<IIpFilter> getNewIpFilter(IpProtocol proto);

 private:
    // mutex to protect member variables
    std::mutex dataMutex_;
    std::shared_ptr<IDataConnectionManager> dataConnectionManager_;
    std::shared_ptr<IDataProfileManager> dataProfileManager_;

    DataFactory();
    ~DataFactory();
    DataFactory(const DataFactory &) = delete;
    DataFactory &operator=(const DataFactory &) = delete;
};

/** @} */ /* end_addtogroup telematics_data */
}
}

#endif
