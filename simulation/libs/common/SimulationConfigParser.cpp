/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <fstream>
#include <iostream>
#include <regex>
#include <string>

extern "C" {
#include <limits.h>
#include <unistd.h>
}

#include "SimulationConfigParser.hpp"
#include "FileInfo.hpp"

/**
 * Check if a file exists.
 */
inline bool fileExists(const std::string &configFile) {
    std::ifstream f(configFile.c_str());
    return f.good();
}

SimulationConfigParser::SimulationConfigParser() {
    if (configMap_.size() == 0) {
        std::string configFilePath
            = std::string(DEFAULT_SIM_CONFIG_FILE_PATH) + std::string(DEFAULT_SIM_CONFIG_FILE_NAME);

        if (fileExists(configFilePath)) {
            readConfigFile(configFilePath);
        } else {
            configFilePath = std::string(DEFAULT_SIM_FILE_PREFIX)
                             + std::string(DEFAULT_SIM_CONFIG_FILE_PATH)
                             + std::string(DEFAULT_SIM_CONFIG_FILE_NAME);
            if (fileExists(configFilePath)) {
                readConfigFile(configFilePath);
            } else {
                std::cout << "Config file " << DEFAULT_SIM_CONFIG_FILE_NAME
                          << " neither exists in same folder nor at " << configFilePath
                          << std::endl;
            }
        }
    }
}

SimulationConfigParser::SimulationConfigParser(std::string configFile, std::string confFilePath) {
    if (configMap_.size() == 0) {
        std::string configFilePath = getConfigFilePath() + "/" + configFile;
        // Check if the file is present in the same directory where the application is running
        if (fileExists(configFilePath)) {
            readConfigFile(configFilePath);
        } else {
            // Check if the file is present in the provided confFilePath
            configFilePath = confFilePath + "/" + configFile;
            if (fileExists(configFilePath)) {
                readConfigFile(configFilePath);
            } else {
                std::cout << "Config file " << configFile
                          << " neither exists in same folder nor at " << configFilePath
                          << std::endl;
            }
        }
    }
}

SimulationConfigParser::~SimulationConfigParser() {
}

/**
 * Order of search for the key value from config file:
 * Search for key value from user supplied config file which is present under
 * current running app path.
 * else search for key value from default config file which is present under
 * current running app path.
 */
std::string SimulationConfigParser::getValue(std::string key) {
    auto settingsIterator = configMap_.find(key);
    if (settingsIterator != configMap_.end()) {
        return settingsIterator->second;
    } else {
        return std::string("");  // return an empty string when the setting is not configured.
    }
}

/**
 * Get the config file path. Config file is expected to be present in the same
 * location from where application is running.
 */
std::string SimulationConfigParser::getConfigFilePath() {
    char path[PATH_MAX];
    ssize_t count        = readlink("/proc/self/exe", path, PATH_MAX);
    std::string fullPath = std::string(path, (count > 0) ? count : 0);
    auto const pos       = fullPath.find_last_of('/');
    return fullPath.substr(0, pos);
}

/**
 * Utility function to read config file with key value pairs
 * Prepares a map of key value pairs from Key=Value format
 * Discards leading spaces, blank lines and lines starting with #
 * Removes any leading or trailing spaces around Key and Value if any.
 */
void SimulationConfigParser::readConfigFile(std::string configFile) {

    // Create a file stream from the file name
    std::ifstream configFileStream(configFile);

    // Iterate through each parameter in the file and read the key value pairs
    std::string param;
    while (std::getline(configFileStream >> std::ws, param)) {
        std::string key;
        std::istringstream paramStream(param);
        if (std::getline(paramStream, key, '=')) {
            // Ignore lines starting with # character
            if (key[0] == '#') {
                continue;
            }
            key = std::regex_replace(key, std::regex(" +$"), "");
            if (key.length() > 0) {
                std::string value;
                if (std::getline(paramStream, value)) {
                    value           = std::regex_replace(value, std::regex("^ +| +$"), "");
                    configMap_[key] = value;
                }
            }
        }
    }
}
