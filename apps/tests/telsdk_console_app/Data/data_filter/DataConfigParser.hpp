/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @brief DataConfigParser class reads config file and caches the app config
 * settings. It provides utility functions to read the config values (key=value pair).
 */

#ifndef DATACONFIGPARSER_HPP
#define DATACONFIGPARSER_HPP

#include <map>
#include <vector>
#include <string>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <regex>

extern "C" {
#include <limits.h>
#include <unistd.h>
}

#define DEFAULT_DATA_CONFIG_FILE_NAME "/etc/Datafilter.conf"

/*
 * DataConfigParser class caches the config settings from conf file
 * It provides utility methods to get value from configuration file in key,value form.
 *
 * The DataConfigParser provide section based key/value pair segregation.
 * This makes co-existing of muliple section with similar name, but different key/value pair.
 */
class DataConfigParser {
 public:
    DataConfigParser(std::string section, std::string configFile = DEFAULT_DATA_CONFIG_FILE_NAME);
    ~DataConfigParser();
    // Get the user defined value for configured key
    std::string getValue(std::map<std::string, std::string> pairMap_, std::string key);
    std::vector<std::map<std::string, std::string>> getFilters();

 private:
    std::string section_;
    // Function to read config file containing key value pairs
    void readConfigFile(std::string configFile);
    // Get the path where config file is located
    std::string getConfigFilePath();
    // Hashmap to store all settings as key-value pairs
    std::vector<std::map<std::string, std::string>> configVector_;
};

#endif  // DATACONFIGPARSER_HPP
