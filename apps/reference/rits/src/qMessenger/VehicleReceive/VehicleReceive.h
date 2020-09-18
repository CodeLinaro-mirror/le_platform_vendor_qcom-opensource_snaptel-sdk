/*
 *  Copyright (c) 2018-2020, The Linux Foundation. All rights reserved.
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
  * @file: VehicleReceive.h
  *
  * @brief: Public API and definitions of the Vehicle Receive. It uses
  * v2x_vehicle_api.h to get CAN data that will later be added to BSMs
  * or others.
  *
  */
#ifndef __VEHICLE_RECEIVE_H__
#define __VEHICLE_RECEIVE_H__

#include <v2x_vehicle_api.h>
#include <iostream>

using std::cout;

class VehicleReceive {

private:
    static void onVehicleDataChanges(current_dynamic_vehicle_state_t* vehicleData, void* context);
    /**
    * Constructor that registers listener to v2x_vehicle_api
    */
    VehicleReceive();

    static VehicleReceive* instance;

public:
    /**
     * Method that extracts singleton instance pointer of Vehicle Receive.
     * @ return a VehicleReceive* that has access to CAN data to populate BSMs, CAMs, DENMs, etc.
     */
    static VehicleReceive* Instance();

    /**
    * Holds the most up to data CAN data from the vehicle
    * as current_dynamic_state_t.
    * @see v2x_vehicle_api
    */
    current_dynamic_vehicle_state_t* vehicleData;
};
#endif
