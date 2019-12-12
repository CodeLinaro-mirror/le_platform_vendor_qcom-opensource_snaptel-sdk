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
 * @file       MsdProvider.cpp
 *
 * @brief      MsdProvider class fetches the MSD data and will cache the MSD parameters.
 *             It provides utility functions to read the configured values.
 */

#include <fstream>
#include <iostream>
#include <regex>
#include <string>

#include "MsdProvider.hpp"
#include "ConfigParser.hpp"

telux::tel::ECallMsdData MsdProvider::msdData_ = {};

/**
 * Reads MSD data from file and caches it
 */
telux::tel::ECallMsdData MsdProvider::getMsd() {
    return msdData_;
}

/**
 * Function to read MSD config file containing key value pairs
 */
void MsdProvider::init(std::string filename, std::string filePath) {
    std::shared_ptr<ConfigParser> msdSettings = std::make_shared<ConfigParser>(filename, filePath);

    // RECENT_LOCATION_N1_PRESENT
    auto recentVehicleLocationN1PresentAsString
      = msdSettings->getValue("RECENT_LOCATION_N1_PRESENT");
    bool recentVehicleLocationN1PresentAsBool
      = atoi(recentVehicleLocationN1PresentAsString.c_str()) ? true : false;
    msdData_.optionals.recentVehicleLocationN1Present = recentVehicleLocationN1PresentAsBool;

    // RECENT_LOCATION_N2_PRESENT
    auto recentVehicleLocationN2PresentAsString
      = msdSettings->getValue("RECENT_LOCATION_N2_PRESENT");
    bool recentVehicleLocationN2PresentAsBool
      = atoi(recentVehicleLocationN2PresentAsString.c_str()) ? true : false;
    msdData_.optionals.recentVehicleLocationN2Present = recentVehicleLocationN2PresentAsBool;

    // NUMBER_OF_PASSENGERS_PRESENT
    auto numberOfPassengersPresentAsString = msdSettings->getValue("NUMBER_OF_PASSENGERS_PRESENT");
    bool numberOfPassengersPresentAsBool
      = atoi(numberOfPassengersPresentAsString.c_str()) ? true : false;
    msdData_.optionals.numberOfPassengersPresent = numberOfPassengersPresentAsBool;

    // MESSAGE_IDENTIFIER
    msdData_.messageIdentifier = atoi(msdSettings->getValue("MESSAGE_IDENTIFIER").c_str());

    // AUTOMATIC_ACTIVATION
    auto automaticActivationtAsString = msdSettings->getValue("AUTOMATIC_ACTIVATION");
    bool automaticActivationAsBool = atoi(automaticActivationtAsString.c_str()) ? true : false;
    msdData_.control.automaticActivation = automaticActivationAsBool;

    // TEST_CALL
    auto testCallAsString = msdSettings->getValue("TEST_CALL");
    bool testCallAsBool = atoi(testCallAsString.c_str()) ? true : false;
    msdData_.control.testCall = testCallAsBool;

    // POSITION_CAN_BE_TRUSTED
    auto positionCanBeTrustedAsString = msdSettings->getValue("POSITION_CAN_BE_TRUSTED");
    bool positionCanBeTrustedAsBool = atoi(positionCanBeTrustedAsString.c_str()) ? true : false;
    msdData_.control.positionCanBeTrusted = positionCanBeTrustedAsBool;

    // VEHICLE_TYPE
    auto vehicleTypeAsString = msdSettings->getValue("VEHICLE_TYPE");
    msdData_.control.vehicleType
      = static_cast<telux::tel::ECallVehicleType>(atoi(vehicleTypeAsString.c_str()));

    // ISO_WMI
    msdData_.vehicleIdentificationNumber.isowmi = msdSettings->getValue("ISO_WMI");

    // ISO_VDS
    msdData_.vehicleIdentificationNumber.isovds = msdSettings->getValue("ISO_VDS");

    // ISO_VIS_MODEL_YEAR
    msdData_.vehicleIdentificationNumber.isovisModelyear
      = msdSettings->getValue("ISO_VIS_MODEL_YEAR");

    // ISO_VIS_SEQ_PLANT
    msdData_.vehicleIdentificationNumber.isovisSeqPlant =
                                            msdSettings->getValue("ISO_VIS_SEQ_PLANT");

    // GASOLINE_TANK_PRESENT
    auto gasolineTankPresentAsString = msdSettings->getValue("GASOLINE_TANK_PRESENT");
    bool gasolineTankPresentAsBool = atoi(gasolineTankPresentAsString.c_str()) ? true : false;
    msdData_.vehiclePropulsionStorage.gasolineTankPresent = gasolineTankPresentAsBool;

    // DIESEL_TANK_PRESENT
    auto dieselTankPresentAsString = msdSettings->getValue("DIESEL_TANK_PRESENT");
    bool dieselTankPresentAsBool = atoi(dieselTankPresentAsString.c_str()) ? true : false;
    msdData_.vehiclePropulsionStorage.dieselTankPresent = dieselTankPresentAsBool;

    // COMPRESSED_NATURALGAS
    auto compressedNaturalGasAsString = msdSettings->getValue("COMPRESSED_NATURALGAS");
    bool compressedNaturalGasAsBool = atoi(compressedNaturalGasAsString.c_str()) ? true : false;
    msdData_.vehiclePropulsionStorage.compressedNaturalGas = compressedNaturalGasAsBool;

    // LIQUID_PROPANE_GAS
    auto liquidPropaneGasAsString = msdSettings->getValue("LIQUID_PROPANE_GAS");
    bool liquidPropaneGasAsBool = atoi(liquidPropaneGasAsString.c_str()) ? true : false;
    msdData_.vehiclePropulsionStorage.liquidPropaneGas = liquidPropaneGasAsBool;

    // ELECTRIC_ENERGY_STORAGE
    auto electricEnergyStorageAsString = msdSettings->getValue("ELECTRIC_ENERGY_STORAGE");
    bool electricEnergyStorageAsAsBool = atoi(electricEnergyStorageAsString.c_str()) ? true : false;
    msdData_.vehiclePropulsionStorage.electricEnergyStorage = electricEnergyStorageAsAsBool;

    // HYDROGEN_STORAGE
    auto hydrogenStorageAsString = msdSettings->getValue("HYDROGEN_STORAGE");
    bool hydrogenStorageAsBool = atoi(hydrogenStorageAsString.c_str()) ? true : false;
    msdData_.vehiclePropulsionStorage.hydrogenStorage = hydrogenStorageAsBool;

    // OTHER_STORAGE
    auto otherStorageAsString = msdSettings->getValue("OTHER_STORAGE");
    bool otherStorageAsBool = atoi(otherStorageAsString.c_str()) ? true : false;
    msdData_.vehiclePropulsionStorage.otherStorage = otherStorageAsBool;

    // TIMESTAMP
    msdData_.timestamp = atoi(msdSettings->getValue("TIMESTAMP").c_str());

    // VEHICLE_POSITION_LATITUDE
    msdData_.vehicleLocation.positionLatitude
      = atoi(msdSettings->getValue("VEHICLE_POSITION_LATITUDE").c_str());

    // VEHICLE_POSITION_LONGITUDE
    msdData_.vehicleLocation.positionLongitude
      = atoi(msdSettings->getValue("VEHICLE_POSITION_LONGITUDE").c_str());

    // VEHICLE_DIRECTION
    msdData_.vehicleDirection = atoi(msdSettings->getValue("VEHICLE_DIRECTION").c_str());

    // RECENT_N1_LATITUDE_DELTA
    msdData_.recentVehicleLocationN1.latitudeDelta
      = atoi(msdSettings->getValue("RECENT_N1_LATITUDE_DELTA").c_str());

    // RECENT_N1_LONGITUDE_DELTA
    msdData_.recentVehicleLocationN1.longitudeDelta
      = atoi(msdSettings->getValue("RECENT_N1_LONGITUDE_DELTA").c_str());

    // RECENT_N2_LATITUDE_DELTA
    msdData_.recentVehicleLocationN2.latitudeDelta
      = atoi(msdSettings->getValue("RECENT_N2_LATITUDE_DELTA").c_str());

    // RECENT_N2_LONGITUDE_DELTA
    msdData_.recentVehicleLocationN2.longitudeDelta
      = atoi(msdSettings->getValue("RECENT_N2_LONGITUDE_DELTA").c_str());

    // NUMBER_OF_PASSENGERS
    msdData_.numberOfPassengers = atoi(msdSettings->getValue("NUMBER_OF_PASSENGERS").c_str());

}
