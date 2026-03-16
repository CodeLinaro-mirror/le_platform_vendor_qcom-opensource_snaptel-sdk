/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @brief ConfigParser class reads config file and caches the app config
 * settings. It provides utility functions to read the config values (key=value pair).
 */

#ifndef CONFIGPARSER_HPP
#define CONFIGPARSER_HPP

#include <map>
#include <vector>
#include <string>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <regex>
#include <telux/common/Log.hpp>

extern "C" {
#include <limits.h>
#include <unistd.h>
}
#include "define.hpp"

#ifndef DEFAULT_CONFIG_FILE_NAME
#define DEFAULT_CONFIG_FILE_NAME "/etc/telux_power_refd.conf"
#endif

/*
 * Reference app specific config
 * ConfigParser class caches the config settings from conf file
 * It provides utility methods to get value from configuration file in key,value form.
 *
 * The ConfigParser provide section based key/value pair segregation.
 * This makes co-existing of multiple section with similar name, but different key/value pair.
 */
class ConfigParser {
    static ConfigParser *instance;

 public:
    static ConfigParser *getInstance(std::string configFile = DEFAULT_CONFIG_FILE_NAME);
    ~ConfigParser();
    // Get the user defined value for configured key
    std::string getValue(std::string section, std::string key);
    std::map<std::string, std::string> getSectionValue(std::string section);

    // Added to support cases where the same section name is provided multiple times,
    // for example, multiple socket parameters to enable multiple socket connections
    std::vector<std::map<std::string, std::string>> getDuplicateSectionValue(std::string section);

    std::map<std::string, std::map<std::string, std::string>> getAllConfig();

 private:
    ConfigParser(std::string configFile = DEFAULT_CONFIG_FILE_NAME);
    // Function to read config file containing key value pairs
    void readConfigFile(std::string configFile);
    // Get the path where config file is located
    std::string getConfigFilePath();
    // Hashmap to store all settings as key-value pairs
    std::map<std::string, std::map<std::string, std::string>> configMap_;
    std::string configFile_ = DEFAULT_CONFIG_FILE_NAME;
};

#endif  // CONFIGPARSER_HPP
