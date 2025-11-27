/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       EventParserUtil.hpp
 *
 * @brief      Utils class that helps with parsing the event string.
 *
 */

#ifndef EVENT_PARSER_UTIL_HPP
#define EVENT_PARSER_UTIL_HPP

#include <telux/common/CommonDefines.hpp>
#include <string>

using namespace telux::common;
using namespace std;

namespace telux {
namespace common {

/* Flags received for different operations. */
const string FILTER_FLAG = "-f";
const string EVENT_FLAG  = "-e";

struct EventMessage {
    std::string filter;
    std::string event;
};

class EventParserUtil {
 public:
    static void parseEventHeaderOptions(std::string flagsString, EventMessage &parsedMsg);
    static std::string getNextToken(std::string &inputString, std::string delimiter);
};

}  // end of namespace common

}  // end of namespace telux

#endif  // EVENT_PARSER_UTIL_HPP
