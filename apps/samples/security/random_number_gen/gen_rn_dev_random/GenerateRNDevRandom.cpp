/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * Sample application to demonstrate how to:
 * 1. How to generate random number and random data bytes using TRNG.
 */

#include <iostream>

#include <telux/sec/RandomNumberManager.hpp>
#include <telux/sec/SecurityFactory.hpp>

int main(int argc, char **argv) {

    size_t numBytes    = 0;
    uint8_t *data      = nullptr;
    uint32_t randNum32 = 0;
    uint64_t randNum64 = 0;
    telux::common::ErrorCode ec;
    std::vector<uint8_t> generatedData(16, 0);

    std::shared_ptr<telux::sec::IRandomNumberManager> rngMgr;

    /* Get SecurityFactory instance */
    auto &secFact = telux::sec::SecurityFactory::getInstance();

    /* Get CryptoManager instance with TRNG as source */
    rngMgr = secFact.getRandomNumberManager(telux::sec::RNGSource::DEV_RANDOM, ec);
    if (!rngMgr) {
        std::cout << "Can't allocate IRandomNumberManager, err: " << static_cast<int>(ec)
                  << std::endl;
        return -1;
    }

    /* Generate 32 bit random number */
    ec = rngMgr->getRandomNumber(randNum32);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "failed 32 bit number generation, err: " << static_cast<int>(ec) << std::endl;
        return -1;
    }

    std::cout << "32 bit random number generated: " << randNum32 << std::endl;

    /* Generate 64 bit random number */
    ec = rngMgr->getRandomNumber(randNum64);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "failed 64 bit number generation, err: " << static_cast<int>(ec) << std::endl;
        return -1;
    }

    std::cout << "64 bit random number generated: " << randNum64 << std::endl;

    /* Generate 16 random data bytes */
    ec = rngMgr->getRandomData(generatedData, numBytes);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "failed data generation, err: " << static_cast<int>(ec) << std::endl;
        return -1;
    }

    printf("numBytes: %zu\n", numBytes);
    printf("random data generated: ");
    data = generatedData.data();
    for (size_t x = 0; x < numBytes; x++) {
        printf("%02x", data[x] & 0xffU);
    }
    printf("\n");
    fflush(stdout);

    return 0;
}
