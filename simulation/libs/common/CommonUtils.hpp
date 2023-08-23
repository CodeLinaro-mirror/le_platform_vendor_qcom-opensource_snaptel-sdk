/*
 *  Changes from Qualcomm Innovation Center are provided under the following license:
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef COMMONUTILS_HPP
#define COMMONUTILS_HPP

#include <telux/common/CommonDefines.hpp>
#include "JsonParser.hpp"
#include "Logger.hpp"

#define handleApiResponseForMethod(subSystem, manager)                                       \
    telux::common::Status status = Status::FAILED;                                           \
    telux::common::ErrorCode errorCode = ErrorCode::GENERIC_FAILURE;                         \
    uint32_t cbDelay = 100;                                                                  \
    Json::Value rootNode;                                                                    \
    do {                                                                                     \
        ErrorCode err                                                                        \
            = JsonParser::readFromJsonFile(rootNode, "api/" subSystem "/" manager ".json");  \
        if (err != ErrorCode::SUCCESS) {                                                     \
            LOG(ERROR, "Unable to read file: " subSystem "/" manager);                       \
            status = Status::FAILED;                                                         \
            errorCode = ErrorCode::GENERIC_FAILURE;                                          \
            break;                                                                           \
        }                                                                                    \
        CommonUtils::getValues(rootNode, manager, __FUNCTION__, status, errorCode, cbDelay); \
    } while (0);                                                                             \
    if (status != Status::SUCCESS) {                                                         \
        LOG(ERROR, subSystem "/" manager "::", __FUNCTION__,                                 \
            " failed: ", static_cast<int>(status));                                          \
        return status;                                                                       \
    }
namespace telux {

namespace common {

class CommonUtils {
 public:
    static telux::common::Status mapStatus(std::string status);
    static telux::common::ErrorCode mapErrorCode(std::string errorCode);
    static telux::common::ErrorCode toErrorCode(telux::common::Status status);

    static void getValues(Json::Value &values, std::string subsystem,
        std::string method, telux::common::Status &status,
        telux::common::ErrorCode &errorCode, uint32_t &cbDelay);
    static telux::common::ServiceStatus mapServiceStatus(std::string status);
    static std::string readSystemDataValue(
        std::string subsystem, std::string defaultValue, std::vector<std::string> path);
    static ErrorCode writeSystemDataValue(
        std::string subsystem, std::string value, std::vector<std::string> path);

    template<typename T>
    static void updateJsonValue(const std::string& filePath, const std::string& subsystem,
        const std::string& method, const std::string& attribute, T val) {
        Json::Value rootObj;
        ErrorCode error = JsonParser::readFromJsonFile(rootObj, filePath);
        if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! ");
        }
        rootObj[subsystem][method][attribute] = val;
        JsonParser::writeToJsonFile(rootObj, filePath);
    }

 private:
    static std::string readSystemDataValue(
        Json::Value &jsonValue, std::string defaultValue, std::vector<std::string> &path);
    static void writeSystemDataValue(
        Json::Value &node, std::string value, std::vector<std::string> &path);
};

}  // namespace common
}  // namespace telux
#endif  // COMMONUTILS_HPP
