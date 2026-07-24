/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef USERUTILS_HPP
#define USERUTILS_HPP

#include <string>

class UserUtils {
 public:
    bool getYesNoFromUser(std::string choiceToDisplay);
    bool getLocalRemoteFromUser();
    std::string getStringFromUser(std::string prompt);
};

#endif  // USERUTILS_HPP
