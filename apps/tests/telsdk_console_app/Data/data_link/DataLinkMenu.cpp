/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include <future>
#include "DataLinkMenu.hpp"
#include "../DataUtils.hpp"

using namespace std;
using namespace telux::data;
using namespace telux::common;

DataLinkMenu::DataLinkMenu(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
    dataLinkManager_ = nullptr;
    initComplete_ = false;
    addMenuCmds_ = false;
    dataLinkListener_ = std::make_shared<DataLinkListener>();
}

DataLinkMenu::~DataLinkMenu() {
    if (dataLinkManager_ && dataLinkListener_) {
        dataLinkManager_->deregisterListener(dataLinkListener_);
    }
    dataLinkManager_ = nullptr;
    dataLinkListener_ = nullptr;
    initComplete_ = false;
}

bool DataLinkMenu::init() {
    bool initStat = initDataLinkManagerAndListener();

    if (addMenuCmds_ == false) {
        addMenuCmds_ = true;
        std::shared_ptr<ConsoleAppCommand> registerListener =
            std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("1", "register_listener", {},
            std::bind(&DataLinkMenu::registerListener, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> deregisterListener =
            std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("2", "deregister_listener", {},
            std::bind(&DataLinkMenu::deregisterListener, this, std::placeholders::_1)));

        std::vector<std::shared_ptr<ConsoleAppCommand>> commandsList = {registerListener,
            deregisterListener};
        addCommands(commandsList);
    }

    return initStat;
}

bool DataLinkMenu::initDataLinkManagerAndListener() {
    if (initComplete_) {
        std::cout << "\nDataLinkMenu is already initialized." << std::endl;
        return true;
    }

    auto &dataFactory = telux::data::DataFactory::getInstance();
    dataLinkManager_ = dataFactory.getDataLinkManager(telux::data::OperationType::DATA_LOCAL);

    if (dataLinkManager_ == nullptr) {
        std::cout << "\nUnable to initialize DataLink Manager." << std::endl;
        return false;
    }

    bool isReady = dataLinkManager_->isSubsystemReady();
    if (!isReady) {
        std::cout << "\nInitializing DataLink Manager subsystem, please wait..." << std::endl;
        std::future<bool> readyFuture = dataLinkManager_->onSubsystemReady();
        isReady = readyFuture.get();
    }

    if (isReady) {
        std::cout << "\nDataLink Manager is ready." << std::endl;
        initComplete_ = true;
        dataLinkManager_->registerListener(dataLinkListener_);
    } else {
        std::cout << "\nDataLink Manager is not ready." << std::endl;
        return false;
    }

    return true;
}

void DataLinkMenu::registerListener(std::vector<std::string> inputCommand) {
    std::cout << " register data link listener " << std::endl;
    dataLinkManager_->registerListener(dataLinkListener_);
}

void DataLinkMenu::deregisterListener(std::vector<std::string> inputCommand) {
    std::cout << " deregister data link listener " << std::endl;
    dataLinkManager_->deregisterListener(dataLinkListener_);
}