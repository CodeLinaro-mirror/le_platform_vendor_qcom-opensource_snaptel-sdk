/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "CsvHandler.hpp"
#include "Logger.hpp"
#include <fstream>

#define DELIMETER ','

namespace telux {

namespace common {

CsvHandler::CsvHandler(std::string filename) {
    filename_ = filename;
}

CsvHandler::~CsvHandler() {
    filename_ = "";
}

Status CsvHandler::readCsv(csvData &data) {
    LOG(DEBUG, __FUNCTION__);
    std::lock_guard<std::mutex> lk(CsvHandler::fileMutex_);
    std::string line, colname, val  = "";
    std::vector<std::string> headers;

    std::ifstream ifs(filename_);
    if(!ifs.is_open()) {
        LOG(ERROR, __FUNCTION__, "Could not open the file: ", filename_);
        return Status::FAILED;
    }

    if(ifs.good()) {
        LOG(DEBUG, "Starting to read csv");
        while(std::getline(ifs, line)) {
            //skipping empty lines & lines that contains license text
            if((line.size() != 0) && (line.find('*') == string::npos))
            {
                break;
            }
        }

        /* Extracting the data header part */
        std::stringstream colStream(line);

        LOG(DEBUG, "Extracting Headers");
        // Extract each column name
        while(std::getline(colStream, colname, DELIMETER)) {
            headers.emplace_back(colname);
        }

        LOG(DEBUG, "Extracting data");
        // Extracting the row data
        while(std::getline(ifs, line))
        {
            // Create a stringstream of the current line
            std::stringstream rowStream(line);
            int row = 0;

            while (std::getline(rowStream, val, DELIMETER)) {
                /**additional handling for data within double-quotes.
                 For example: nmea_sentence could hold values like below
                 "$GLGSV,1,1,03,82,11,169,35,69,26,340,35,68,14,032,36,1*4C"

                 if case will be executed for the data that doesn't exist
                 within double-quotes.
                 **/
                if(val.front() != '"') {
                    data[headers[row]].emplace_back(val);
                } else {
                    //skipping ',' as delimeter if value inside double-quotes
                    string str = val;
                    std::getline(rowStream, val, '"');
                    str += ", " + val;
                    str.erase(0,1);
                    data[headers[row]].emplace_back(str);
                    std::getline(rowStream, val, ',');
                }
            }
        }
    }
    LOG(DEBUG, "File read complete");
    // Close file
    ifs.close();
    return Status::SUCCESS;
}

Status CsvHandler::writeCsv(const std::vector<std::string>& headers, csvData &data,
    const LicenseHeader &license) {

    LOG(DEBUG, __FUNCTION__);
    std::lock_guard<std::mutex> lck(CsvHandler::fileMutex_);
    // Create an output filestream object
    std::ofstream ofs(filename_);
    /* Print the license header to the file.*/
    std::stringstream writeStream;

    if (license.isAvailable) {
        LOG(DEBUG, "Writing license content");
        for (auto val : license.license)
        {
            writeStream << val << "\n";
        }
        ofs << writeStream.str();
    }

    auto itr = data.begin();
    int columnSize = headers.size();
    long int rowSize = data[itr->first].size();
    int columnIdx = 0;
    writeStream.str(std::string());

    LOG(DEBUG, "Starting to write headers");
    for (auto headerVal: headers)
    {
        writeStream << headerVal;
        if(columnIdx != columnSize - 1) {
            writeStream << ","; // No comma at end of line
        }
        columnIdx++;
    }

    writeStream << "\n";
    ofs << writeStream.str();
    writeStream.str(std::string());

    LOG(DEBUG, "Starting to write data");
    for (long int idx=0;idx<rowSize;idx++)
    { //Writing data row-by-row
        columnIdx = 0;
        for (auto headerVal: headers)
        {
            if(data.find(headerVal) != data.end()) {
                writeStream << data[headerVal][idx];
            } else {
                writeStream << ""; //if header not matches
            }

            if(columnIdx != columnSize - 1) {
                writeStream << ","; // No comma at end of line
            }
            columnIdx++;
        }
        writeStream << "\n";
        ofs << writeStream.str();
        writeStream.str(std::string());
    }

    LOG(DEBUG, "Writing csv completed");
    ofs.close();
    return Status::SUCCESS;
}

} // end of namespace common

} // end of namespace telux