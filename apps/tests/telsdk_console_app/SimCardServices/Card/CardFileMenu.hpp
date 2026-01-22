/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef CARD_FILE_MENU_HPP
#define CARD_FILE_MENU_HPP

#include "telux/tel/CardManager.hpp"
#include "telux/tel/CardFileHandler.hpp"
#include "telux/tel/CardDefines.hpp"
#include "console_app_framework/ConsoleApp.hpp"

class CardFileMenu : public ConsoleApp {
 public:
    CardFileMenu(std::string appName, std::string cursor);
    ~CardFileMenu();
    bool init();

 private:
    void getSupportedApps(std::vector<std::string> userInput);
    void readEFLinearFixed(std::vector<std::string> userInput);
    void readEFLinearFixedAll(std::vector<std::string> userInput);
    void readEFTransparent(std::vector<std::string> userInput);
    void writeEFLinearFixed(std::vector<std::string> userInput);
    void writeEFTransparent(std::vector<std::string> userInput);
    void requestEFAttributes(std::vector<std::string> userInput);
    void selectCardSlot(std::vector<std::string> userInput);
    std::string cardStateToString(telux::tel::CardState state);
    std::string appTypeToString(telux::tel::AppType appType);
    std::string appStateToString(telux::tel::AppState appState);
    std::shared_ptr<telux::tel::ICardListener> cardListener_;
    std::shared_ptr<telux::tel::ICardManager> cardManager_;
    int slot_ = DEFAULT_SLOT_ID;
    std::vector<std::shared_ptr<telux::tel::ICard>> cards_;
};

class CardFileHandlerResponseCallback {
 public:
    static void EfReadLinearFixedResponseCb(
        telux::common::ErrorCode error, telux::tel::IccResult result);
    static void EfReadAllRecordsResponseCb(
        telux::common::ErrorCode error, std::vector<telux::tel::IccResult> records);
    static void EfReadTransparentResponseCb(
        telux::common::ErrorCode error, telux::tel::IccResult result);
    static void EfWriteLinearFixedResponseCb(
        telux::common::ErrorCode error, telux::tel::IccResult result);
    static void EfWriteTransparentResponseCb(
        telux::common::ErrorCode error, telux::tel::IccResult result);
    static void EfGetFileAttributesCb(telux::common::ErrorCode error, telux::tel::IccResult result,
        telux::tel::FileAttributes attributes);
};

#endif  // CARD_FILE_MENU_HPP
