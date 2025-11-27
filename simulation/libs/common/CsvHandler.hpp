/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       CsvHandler.hpp
 *
 * @brief      Declares the CsvHandler class that handles the CSV file read/write
 *             operations.
 *
 */

#ifndef CSV_HANDLER_HPP
#define CSV_HANDLER_HPP

#include <telux/common/CommonDefines.hpp>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <mutex>

using namespace std;

namespace telux {

namespace common {

/**
 * @brief: Handles the content of the csv file in the following format.
 *       For Ex: If csv contains
 *       A,B,C,D,E
 *       1,2,3,4,5
 *       6,7,8,9,10
 *       11,12,,14,15
 *
 *       Data is stored as:
 *       Key->Value
 *
 *       A -> 1,6,11
 *       B -> 2,7,12
 *       C -> 3,8,0
 *       D -> 4,9,14
 *       E -> 5,10,15
 *
 *       To access data:
 *       data[A][0]=1  data[B][0]=2
 *       data[A][1]=6  data[B][1]=7
 *
 */
typedef std::unordered_map<std::string, std::vector<string>> csvData;
typedef std::vector<std::string> License;

struct LicenseHeader {
    bool isAvailable;
    License license;
};

class CsvHandler {
 public:
    /**
     * @brief: Open the file
     */
    CsvHandler(std::string filename);
    ~CsvHandler();

    /**
     * @brief:   Reads the complete CSV file
     * @param:   data     -    Reference to the data structure where the parsed content is
     *                         stored.

     */
    Status readCsv(csvData &data);

    /**
     *  @brief: Writes to the CSV file as a fresh file.
     *
     *  @param: headers       -   The data header content to be written to the file.
     *  @param: data          -   The content to be written to the file.
     *  @param: license       -   Lisence related properties are stored.
     */
    Status writeCsv(
        const std::vector<std::string> &headers, csvData &data, const LicenseHeader &license);

 private:
    std::mutex fileMutex_;
    std::string filename_;
};

}  // end of namespace common

}  // end of namespace telux

#endif  // CSV_HANDLER_HPP