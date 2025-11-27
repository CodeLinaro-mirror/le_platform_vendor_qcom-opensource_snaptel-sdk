/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @brief SimulationConfigParser class reads config file and caches the app config
 * settings. It provides utility functions to read the config values.
 */

#ifndef SIMULATIONCONFIGPARSER_HPP
#define SIMUALTIONCONFIGPARSER_HPP

#include <map>
#include <string>

/*
 * SimulationConfigParser class caches the config settings from conf file
 * It provides utility methods to get value of a configured settings
 */
class SimulationConfigParser {
 public:
    SimulationConfigParser();
    SimulationConfigParser(std::string configFile, std::string configFilePath);
    ~SimulationConfigParser();
    // Get the user defined value for configured key
    std::string getValue(std::string key);

 private:
    // Function to read config file containing key value pairs
    void readConfigFile(std::string configFile);

    // Get the path where config file is located
    std::string getConfigFilePath();

    // Hashmap to store all settings as key-value pairs
    std::map<std::string, std::string> configMap_;
};

#endif  // SIMULATIONCONFIGPARSER_HPP
