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

/*
 * Changes from Qualcomm Technologies, Inc. are provided under the following license:
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * This application demonstrates how to make an ecall. The steps are as follows:
 *
 * 1. Get a PhoneFactory instance.
 * 2. Get a ICallManager instance from the PhoneFactory.
 * 3. Wait for the call manager service to become available.
 * 4. Trigger an ecall.
 * 5. Receive status of the ecall in callback.
 * 6. Wait while the call is in progress.
 * 7. Finally, when the use case is over, hangup the call.
 *
 * Usage:
 * # ./make_ecall_app
 * # ./make_ecall_app default_sdn_uri
 * # ./make_ecall_app custom_sdn_uri
 * # ./make_ecall_app dialing_number <number>
 */

#include <errno.h>
#include <chrono>
#include <iostream>
#include <memory>
#include <thread>

#include <telux/tel/PhoneFactory.hpp>

using namespace telux::tel;
using namespace telux::common;

// Define macros to populate eCallMsdData
#define MSD_VERSION 2
#define MESSAGE_IDENTIFIER 60
#define AUTOMATIC_ACTIVATION 1
#define TEST_CALL 0
#define POSITION_CAN_BE_TRUSTED 1
#define VEHICLE_TYPE 0
#define ISO_WMI "ECA"
#define ISO_VDS "LLEXAM"
#define ISO_VIS_MODEL_YEAR "P"
#define ISO_VIS_SEQ_PLANT "LE02013"
#define GASOLINE_TANK_PRESENT 1
#define DIESEL_TANK_PRESENT 0
#define COMPRESSED_NATURALGAS 0
#define LIQUID_PROPANE_GAS 0
#define ELECTRIC_ENERGY_STORAGE 0
#define HYDROGEN_STORAGE 0
#define OTHER_STORAGE 0
#define TIMESTAMP 1367878452
#define VEHICLE_POSITION_LATITUDE 123
#define VEHICLE_POSITION_LONGITUDE 1234
#define VEHICLE_DIRECTION 4
#define RECENT_LOCATION_N1_PRESENT 1
#define RECENT_N1_LATITUDE_DELTA -1
#define RECENT_N1_LONGITUDE_DELTA -10
#define RECENT_LOCATION_N2_PRESENT 1
#define RECENT_N2_LATITUDE_DELTA -1
#define RECENT_N2_LONGITUDE_DELTA -30
#define NUMBER_OF_PASSENGERS_PRESENT 1
#define NUMBER_OF_PASSENGERS 2
#define VIN "ECALLEXAMPLE02013"
#define OPTIONAL_ADDITIONAL_DATA_PRESENT 1
#define OID_DATA "8.1"
/* If already encoded optional additional data content is available, fill "OAD_DATA",
otherwise fill Euro NCAP optional additional data content fields.
# For example, OAD_DATA = "0832D28480" */
static const std::string OAD_DATA = "";
/* Below are the Euro NCAP optional additional data content fields. */
#define EURONCAP_LOCATION_OF_IMPACT 2
/* Possible LOCATION_OF_IMPACT values are 0 to 6
  0 = unknown
  1 = none,
  2 = front,
  3 = rear,
  4 = driver_side,
  5 = non_driver_side,
  6 = other */
#define EURONCAP_ROLL_OVER_DETECTED_PRESENT 0
#define EURONCAP_ROLL_OVER_DETECTED 0
// range limit is 100 to 255
#define EURONCAP_DELTAV_RANGELIMIT 125
// delta VX range is -255 to 255
#define EURONCAP_DELTAV_DELTAVX -45
// delta VY range is -255 to 255
#define EURONCAP_DELTAV_DELTAVY 10

std::shared_ptr<ICall> dialedCall = nullptr;

// ##### 6.1. implement IMakeCallCallback interface to receive response for the dial request -
// optional
class DialCallback : public IMakeCallCallback {
public:
    void makeCallResponse(ErrorCode error, std::shared_ptr<ICall> call) {
       std::cout << "DialCallback::makeCallResponse" << std::endl;
       std::cout << "makeCallResponse ErrorCode: " << int(error) << std::endl;
       if(call) {
          std::cout << "makeCallResponse::onCallInfoChange: "
                    << " Call Index: " << (int)call->getCallIndex()
                    << " Call Direction: " << (int)call->getCallDirection()
                    << " Phone Number: " << call->getRemotePartyNumber() << std::endl;
          dialedCall = call;
       }
    }
};

/**
 * Main routine
 */
int main(int argc, char *argv[]) {
   // ### 1. Get the PhoneFactory and PhoneManager instances.
   auto &phoneFactory = PhoneFactory::getInstance();
   std::promise<telux::common::ServiceStatus> cbProm = std::promise<telux::common::ServiceStatus>();
   auto callManager = phoneFactory.getCallManager([&](telux::common::ServiceStatus status) {
            cbProm.set_value(status);});
   if(callManager == nullptr) {
      std::cout << " *** ERROR - Unable to get Call Manager instance" << std::endl;
      return 1;
   }

   // ### 2. Wait for the Call Manager subsystem to be ready.
   telux::common::ServiceStatus status = cbProm.get_future().get();
   if(status == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
      std::cout << "Call Manager subsystem is ready" << std::endl;
   } else {
      std::cout << " *** ERROR - Unable to initialize Call Manager subsystem" << std::endl;
      return 1;
   }

   telux::tel::TestECallConfig testECallConfig{
        telux::tel::TestECallConfigType::DEFAULT_SDN_URI, ""};

    std::string configType;
    if (argc > 1 && argv[1] != nullptr) {
        configType = argv[1];
    } else {
        configType = "default_sdn_uri";  // sensible default
    }

    if (configType == "default" || configType == "default_sdn_uri") {
        testECallConfig.type = telux::tel::TestECallConfigType::DEFAULT_SDN_URI;
    } else if (configType == "custom" || configType == "custom_sdn_uri") {
        // User need to configure SDN or SDN URI in the EFS file
        // Create efsprofiles folder inside root(/) folder of EFS explorer in PCAT
        // and copy overrideconfig file to efsprofiles folder.
        testECallConfig.type = telux::tel::TestECallConfigType::CUSTOM_SDN_URI;
    } else if (configType == "dialing" || configType == "dialing_number") {
        testECallConfig.type = telux::tel::TestECallConfigType::DIALING_NUMBER;
        if (argc > 2) {
            testECallConfig.dialNumber = argv[2];
        } else {
            std::cout << "Missing dialing number for dialing_number config. "
                << "Using empty dial number." << std::endl;
        }
   } else {
        std::cout << "Unknown test config type '" << configType << "'. Using DEFAULT_SDN_URI."
             << std::endl;
        testECallConfig.type = telux::tel::TestECallConfigType::DEFAULT_SDN_URI;
   }
   // ### 3. Instantiate dial callback instance - this is optional
   std::shared_ptr<DialCallback> dialCb = std::make_shared<DialCallback>();

   // ### 4. Create details required to make emergency call(eCall) like eCallMsdData,
   // emergencyCategory and eCallVariant
   int emergencyCategory = 64;
   ECallMsdData eCallMsdData;
   int eCallVariant = 1;
   int phoneId = DEFAULT_PHONE_ID;
   // Populate eCallMsdData with valid information
   eCallMsdData.optionals.recentVehicleLocationN1Present = RECENT_LOCATION_N1_PRESENT;
   eCallMsdData.optionals.recentVehicleLocationN2Present = RECENT_LOCATION_N2_PRESENT;
   eCallMsdData.optionals.numberOfPassengersPresent = NUMBER_OF_PASSENGERS_PRESENT;
   eCallMsdData.messageIdentifier = MESSAGE_IDENTIFIER;
   eCallMsdData.control.automaticActivation = AUTOMATIC_ACTIVATION;
   eCallMsdData.control.testCall = TEST_CALL;
   eCallMsdData.control.positionCanBeTrusted = POSITION_CAN_BE_TRUSTED;
   eCallMsdData.control.vehicleType = static_cast<ECallVehicleType>(VEHICLE_TYPE);
   eCallMsdData.vehicleIdentificationNumber.isowmi = ISO_WMI;
   eCallMsdData.vehicleIdentificationNumber.isovds = ISO_VDS;
   eCallMsdData.vehicleIdentificationNumber.isovisModelyear = ISO_VIS_MODEL_YEAR;
   eCallMsdData.vehicleIdentificationNumber.isovisSeqPlant = ISO_VIS_SEQ_PLANT;
   eCallMsdData.vehiclePropulsionStorage.gasolineTankPresent = GASOLINE_TANK_PRESENT;
   eCallMsdData.vehiclePropulsionStorage.dieselTankPresent = DIESEL_TANK_PRESENT;
   eCallMsdData.vehiclePropulsionStorage.compressedNaturalGas = COMPRESSED_NATURALGAS;
   eCallMsdData.vehiclePropulsionStorage.liquidPropaneGas = LIQUID_PROPANE_GAS;
   eCallMsdData.vehiclePropulsionStorage.electricEnergyStorage = ELECTRIC_ENERGY_STORAGE;
   eCallMsdData.vehiclePropulsionStorage.hydrogenStorage = HYDROGEN_STORAGE;
   eCallMsdData.vehiclePropulsionStorage.otherStorage = OTHER_STORAGE;
   eCallMsdData.timestamp = TIMESTAMP;
   eCallMsdData.vehicleLocation.positionLatitude = VEHICLE_POSITION_LATITUDE;
   eCallMsdData.vehicleLocation.positionLongitude = VEHICLE_POSITION_LONGITUDE;
   eCallMsdData.vehicleDirection = VEHICLE_DIRECTION;
   eCallMsdData.optionals.optionalDataPresent = OPTIONAL_ADDITIONAL_DATA_PRESENT;
   eCallMsdData.recentVehicleLocationN1.latitudeDelta = RECENT_N1_LATITUDE_DELTA;
   eCallMsdData.recentVehicleLocationN1.longitudeDelta = RECENT_N1_LONGITUDE_DELTA;
   eCallMsdData.recentVehicleLocationN2.latitudeDelta = RECENT_N2_LATITUDE_DELTA;
   eCallMsdData.recentVehicleLocationN2.longitudeDelta = RECENT_N2_LONGITUDE_DELTA;
   eCallMsdData.numberOfPassengers = NUMBER_OF_PASSENGERS;
   eCallMsdData.optionalPdu.oid = OID_DATA;
   if (!OAD_DATA.empty()) {
       std::vector<uint8_t> data(OAD_DATA.begin(), OAD_DATA.end());
       eCallMsdData.optionalPdu.data = data;
   } else {
       std::vector<uint8_t> data;
       // get encoded optional additional data content
       ECallOptionalEuroNcapData optionalEuroNcapData = {};
       // refer ECallLocationOfImpact for more values
       optionalEuroNcapData.locationOfImpact =
           static_cast<ECallLocationOfImpact>(EURONCAP_LOCATION_OF_IMPACT);
       optionalEuroNcapData.rollOverDetectedPresent = EURONCAP_ROLL_OVER_DETECTED_PRESENT;
       optionalEuroNcapData.rollOverDetected = EURONCAP_ROLL_OVER_DETECTED;
       // deltav range limit is 100 to 255
       optionalEuroNcapData.deltaV.rangeLimit = EURONCAP_DELTAV_RANGELIMIT;
       // deltav VX range is -255 to 255
       optionalEuroNcapData.deltaV.deltaVX = EURONCAP_DELTAV_DELTAVX;
       // deltav VY range is -255 to 255
       optionalEuroNcapData.deltaV.deltaVY = EURONCAP_DELTAV_DELTAVY;
       auto encodeOADContentStatus = callManager->encodeEuroNcapOptionalAdditionalData(
           optionalEuroNcapData, data);
       if (encodeOADContentStatus != telux::common::Status::SUCCESS) {
           std::cout << " Optional additional data content encoding is failed" << std::endl;
           return 1;
       }
       eCallMsdData.optionalPdu.data = data;
   }

   // ### 5. Send an eCall request
   auto makeCallStatus
      = callManager->makeECall(phoneId, eCallMsdData, emergencyCategory, eCallVariant, dialCb,
      testECallConfig);
   std::cout << "Dial ECall Status:" << (int)makeCallStatus << std::endl;

   std::cout << "TestECallConfig type: ";
   switch (testECallConfig.type) {
       case telux::tel::TestECallConfigType::DEFAULT_SDN_URI:
          std::cout << "DEFAULT_SDN_URI";
          break;
       case telux::tel::TestECallConfigType::CUSTOM_SDN_URI:
           std::cout << "CUSTOM_SDN_URI";
           break;
       case telux::tel::TestECallConfigType::DIALING_NUMBER:
           std::cout << "DIALING_NUMBER";
            break;
       default:
           std::cout << "UNKNOWN";
           break;
   }
   if (!testECallConfig.dialNumber.empty()) {
       std::cout << ", dialNumber=" << testECallConfig.dialNumber;
   }
   std::cout << std::endl;
   // ### 6. Wait for the call state to become active and hang-up the call after conversation
   std::this_thread::sleep_for(std::chrono::seconds(10));
   if(dialedCall) {
      dialedCall->hangup();
   }

   // ### 7. Exit logic is specific to an application
   std::cout << "Press enter to exit" << std::endl;
   std::string input;
   std::getline(std::cin, input);
   std::cout << "Exiting application..." << std::endl;
   return 0;
}
