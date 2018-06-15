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

#include <iostream>
#include <memory>

#include <telux/tel/PhoneFactory.hpp>

using namespace telux::tel;
using namespace telux::common;

// ### 7. Initialize the Minimum Set of Data(MSD) data as shown

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
#define OPTIONAL_DATA_PRESENT 1
#define OPTIONALS_OPTIONAL_DATA_TYPE 1

// ##### 6.1. implement IMakeCallCallback interface to receive response for the dial request -
// optional
class DialCallback : public IMakeCallCallback {
public:
   void makeCallResponse(ErrorCode error, std::shared_ptr<ICall> call) override;
};

void DialCallback::makeCallResponse(ErrorCode error, std::shared_ptr<ICall> call) {
   std::cout << "DialCallback::makeCallResponse" << std::endl;
   std::cout << "makeCallResponse ErrorCode: " << int(error) << std::endl;
   if(call) {
      std::cout << "makeCallResponse::onCallInfoChange: "
                << " Call Index: " << (int)call->getCallIndex()
                << " Call Direction: " << (int)call->getCallDirection()
                << " Phone Number: " << call->getRemotePartyNumber() << std::endl;
   }
}

/**
 * Main routine
 */
int main(int, char **) {

   // ### 1. Get the PhoneFactory and PhoneManager instances.
   auto &phoneFactory = PhoneFactory::getInstance();
   auto phoneManager = phoneFactory.getPhoneManager();

   // ### 2. Check if telephony subsystem is ready
   bool subSystemsStatus = phoneManager->isSubsystemReady();

   // #### 2.1 If telephony subsystem is not ready, wait for it to be ready
   if(!subSystemsStatus) {
      std::cout << "Telephony subsystem is not ready" << std::endl;
      std::cout << "wait unconditionally for it to be ready " << std::endl;
      std::future<bool> f = phoneManager->onSubsystemReady();
      // If we want to wait unconditionally for telephony subsystem to be ready
      subSystemsStatus = f.get();
   }

   // Exit the application, if SDK is unable to initialize telephony subsystems
   if(subSystemsStatus) {
      std::cout << " *** Sub Systems Ready *** " << std::endl;
   } else {
      std::cout << " *** ERROR - Unable to initialize telephony subsystem" << std::endl;
      return 1;
   }

   // ### 4. Instantiate Phone and call manager
   auto phone = phoneManager->getPhone();
   std::shared_ptr<ICallManager> callManager = phoneFactory.getCallManager();

   // ### 5. Get unique id of the phone
   int phoneId = DEFAULT_PHONE_ID;

   // ### 6. Instantiate dial callback instance - this is optional
   std::shared_ptr<DialCallback> dialCb = std::make_shared<DialCallback>();

   // ### 8. Create details required to make emergency call(eCall) like eCallMsdData,
   // emergencyCategory and eCallVariant
   int emergencyCategory = 64;
   ECallMsdData eCallMsdData;
   int eCallVariant = 1;
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
   eCallMsdData.optionals.optionalDataType = (ECallOptionalDataType)OPTIONALS_OPTIONAL_DATA_TYPE;
   eCallMsdData.optionals.optionalDataPresent = OPTIONAL_DATA_PRESENT;
   eCallMsdData.recentVehicleLocationN1.latitudeDelta = RECENT_N1_LATITUDE_DELTA;
   eCallMsdData.recentVehicleLocationN1.longitudeDelta = RECENT_N1_LONGITUDE_DELTA;
   eCallMsdData.recentVehicleLocationN2.latitudeDelta = RECENT_N2_LATITUDE_DELTA;
   eCallMsdData.recentVehicleLocationN2.longitudeDelta = RECENT_N2_LONGITUDE_DELTA;
   eCallMsdData.numberOfPassengers = NUMBER_OF_PASSENGERS;

   // ### 9. Send a eCall request
   if(callManager) {
      auto makeCallStatus
         = callManager->makeECall(phoneId, eCallMsdData, emergencyCategory, eCallVariant, dialCb);
      std::cout << "Dial ECall Status:" << (int)makeCallStatus << std::endl;
   }

   // ### 10. Exit logic is specific to an application
   std::cout << "Press enter to exit" << std::endl;
   std::string input;
   std::getline(std::cin, input);
   std::cout << "Exiting application..." << std::endl;
   return 0;
}
