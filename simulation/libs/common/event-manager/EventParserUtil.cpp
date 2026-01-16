/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "EventParserUtil.hpp"

/* INPUT: -f <filter> -e <event> <event-args-1> <event-args-2> */
std::string EventParserUtil::getNextToken(std::string &inputString, std::string delimiter) {
    unsigned int position = 0;
    std::string token;
    if ((position = inputString.find(delimiter)) != std::string::npos) {
        token = inputString.substr(0, position);
        inputString.erase(0, position + delimiter.length());
    }
    return token;
}

/* INPUT: -f <filter> -e <event> <event-args-1> <event-args-2> */
void EventParserUtil::parseEventHeaderOptions(std::string flagsString, EventMessage &parsedMsg) {
    std::string delimiter = " ";
    std::string filter    = " ";

    /** INPUT: -f <filter> -e <event> <event-args-1> <event-args-2>
     *  OPUTPUT: -f
     **/
    if ((EventParserUtil::getNextToken(flagsString, delimiter)) == FILTER_FLAG) {
        parsedMsg.filter = EventParserUtil::getNextToken(flagsString, delimiter);
    }

    /* INPUT: -e <event> <event-args-1> <event-args-2> */
    parsedMsg.event = flagsString;

    return;
}
