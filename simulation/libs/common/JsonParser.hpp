/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       JsonParser.hpp
 *
 * @brief      This class provides utilities to parse a JSON file.
 *
 */

#ifndef JSON_PARSER_HPP
#define JSON_PARSER_HPP

#include <jsoncpp/json/json.h>
#include <telux/common/CommonDefines.hpp>

#include <mutex>

class JsonParser {
 public:
    /**
     * @brief:   Reads the json file
     * @param:   rootNode - where the parsed Json root object is stored.
     * @param:   path     - relative path to the Json file.
     *                      For ex: data json may be stored in
     *                      /data/telux/json/data/IDataConnectionManager.json, in this
     *                      case path would be /data/IDataConnectionManager.json
     */
    static telux::common::ErrorCode readFromJsonFile(Json::Value &rootNode, std::string path);

    /**
     * @brief:   write the json file
     * @param:   rootNode - Json root object to be written.
     * @param:   path     - relative path to the Json file.
     *                      For ex: data json may be stored in
     *                      /data/telux/json/data/IDataConnectionManager.json, in this
     *                      case path would be /data/IDataConnectionManager.json
     */
    static telux::common::ErrorCode writeToJsonFile(Json::Value rootNode, std::string path);

 private:
    static std::mutex fileMutex_;
};

#endif  // JSON_PARSER_HPP