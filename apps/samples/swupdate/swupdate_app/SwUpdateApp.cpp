/*
Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
*/
/*
 * This application demonstrates how to call performOTA. The steps are as follows:
 *
 *  1. Get a SwUpdateFactory instance.
 *  2. Give command line arguments at package zip location.
 *  3. using Factory Instance call performOTA
 *  4. device should perform the OTA successfully
 *
 * Usage:
 * # ./swupdate_app /data/update_*.zip
 */

#include <errno.h>

#include <iostream>
#include <memory>
#include <cstdlib>
#include <future>
#include <list>
#include <chrono>
#include <thread>

#include <telux/swupdate/SwUpdateFactory.hpp>
#include <telux/common/CommonDefines.hpp>

using namespace telux::swupdate;
int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <data_path>" << std::endl;
        return -1;
    }

    std::string zipPath = argv[1];
    /// Create an instance using the factory method
    SwUpdateFactory &factory = SwUpdateFactory::getInstance();
    std::shared_ptr<ISwUpdateManager> manager = factory.getSwUpdateManager();
    // Call the performOTA function using the factory instance
    manager->performUpdate(zipPath);
    std::cout << "Please check /cache/recovery/ota_status cookie file for OTA status" << std::endl;
    return 0;

}

