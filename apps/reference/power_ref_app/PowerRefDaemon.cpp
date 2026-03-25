/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <iostream>
#include <csignal>
#include <future>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

extern "C" {
#include <getopt.h>
}

#include "PowerRefDaemon.hpp"
#include <telux/common/DeviceConfig.hpp>

#define WAKELOCK_PATH "/sys/power/wake_lock"
#define WAKEUNLOCK_PATH "/sys/power/wake_unlock"
#define RESUME_TIMER_WAKELOCK "power_ref_resume_timer"

PowerRefDaemon &PowerRefDaemon::getInstance() {
    LOG(DEBUG, __FUNCTION__);
    static PowerRefDaemon instance;
    return instance;
}

PowerRefDaemon::~PowerRefDaemon() {
    if (timerId_ != 0) {
        // Disarm the timer first to prevent callbacks during cleanup
        struct itimerspec its;
        memset(&its, 0, sizeof(struct itimerspec));
        timer_settime(timerId_, 0, &its, NULL);

        // Now safely delete the timer
        if (timer_delete(timerId_) == -1) {
            LOG(ERROR, __FUNCTION__, "Failed to delete timer: ", strerror(errno));
        }
        timerId_ = 0;
    }
}

telux::common::Status PowerRefDaemon::init() {
    LOG(DEBUG, __FUNCTION__);
    telux::common::Status initStatus = telux::common::Status::SUCCESS;
    config_                          = ConfigParser::getInstance();

    do {
        shared_ptr<EventManager> eventManager(EventManager::getInstance());
        if (eventManager && eventManager->init()) {
            LOG(DEBUG, __FUNCTION__, " eventManager init succeed");
            eventManager_ = eventManager;
        } else {
            LOG(ERROR, __FUNCTION__, " eventManager init failed");
            initStatus = telux::common::Status::FAILED;
            break;
        }

        /**
         * By default, the app can register for SMS and CAN triggers. However, when power refd
         * runs in an environment where SATCOM is enabled, we need to check if NTN is enabled.
         * If NTN is enabled, NAOIP trigger shouldn't be allowed.
         */
        bool allowNaoIp = true;

#ifdef TELSDK_FEATURE_SATCOM_ENABLED
        // By default, the app runs in TN mode.
        if (config_->getValue("NTN_CONFIGS", "ENABLE_NTN") == "TRUE") {
            ntnEnabled_ = true;
        }
        // If NTN is enabled, disallow NAOIP and CAN triggers
        if (ntnEnabled_) {
            allowNaoIp = false;
            LOG(DEBUG, __FUNCTION__, " NTN enabled: only SMS trigger will run");
        }
#endif

        if (config_->getValue("TRIGGER", "NAOIP_TRIGGER") == "ENABLE" && allowNaoIp) {
            naoIpTrigger_ = make_shared<NAOIpTrigger>(eventManager);
            if (naoIpTrigger_ && naoIpTrigger_->init()) {
                LOG(DEBUG, __FUNCTION__, " naoIpTrigger init succeed");
            } else {
                LOG(ERROR, __FUNCTION__, " naoIpTrigger init failed");
                initStatus = telux::common::Status::FAILED;
                break;
            }
        } else {
            LOG(DEBUG, __FUNCTION__, " naoIpTrigger ",
                config_->getValue("TRIGGER", "NAOIP_TRIGGER"));
        }

        // Register for SMS trigger regardless of NTN enabled/disabled.
        if (config_->getValue("TRIGGER", "SMS_TRIGGER") == "ENABLE") {
            smsTrigger_ = make_shared<SMSTrigger>(eventManager);
            if (smsTrigger_ && smsTrigger_->init()) {
                LOG(DEBUG, __FUNCTION__, " smsTrigger init succeeded");
            } else {
                LOG(ERROR, __FUNCTION__, " smsTrigger init failed");
                initStatus = telux::common::Status::FAILED;
                break;
            }
        } else {
            LOG(DEBUG, __FUNCTION__, " smsTrigger ", config_->getValue("TRIGGER", "SMS_TRIGGER"));
        }

        if (config_->getValue("TRIGGER", "CAN_TRIGGER") == "ENABLE") {
#ifdef CAN_TRIGGER_SUPPORTED
            canTrigger_ = CANTrigger::getInstance(eventManager);
            if (canTrigger_ && canTrigger_->init()) {
                LOG(DEBUG, __FUNCTION__, " canTrigger init succeeded");
            } else {
                LOG(ERROR, __FUNCTION__, " canTrigger init failed");
                initStatus = telux::common::Status::FAILED;
                break;
            }
#else  // CAN_TRIGGER_SUPPORTED
            LOG(ERROR, " CAN trigger is not supported");
#endif  // CAN_TRIGGER_SUPPORTED

        } else {
            LOG(DEBUG, __FUNCTION__, " CAN trigger ", config_->getValue("TRIGGER", "CAN_TRIGGER"));
        }

#ifdef TELSDK_FEATURE_SATCOM_ENABLED
        // Perform ntn enablement.
        if (ntnEnabled_) {
            ntnClient_                      = std::make_shared<NtnClient>();
            telux::common::Status retStatus = ntnClient_->init();
            if (retStatus == telux::common::Status::SUCCESS) {
                ntnClient_->registerForUpdates();
                // Enable NTN
                telux::common::ErrorCode err = ntnClient_->enableNtn();
                if (err == telux::common::ErrorCode::SUCCESS) {
                    LOG(DEBUG, __FUNCTION__, " ntn enable success");
                    if (smsTrigger_) {
                        // Needed for SMS trigger
                        smsTrigger_->setNtnClientInstance(ntnClient_);
                    }
                } else {
                    std::string ec = Utils::getErrorCodeAsString(err);
                    LOG(ERROR, __FUNCTION__, " ntn enable failed, ec: ", ec);
                }
            } else {
                LOG(ERROR, __FUNCTION__, " ntn init failed");
            }
        }
#endif

    } while (0);

    return initStatus;
}

int PowerRefDaemon::startDaemon(int argc, char **argv) {
    LOG(DEBUG, __FUNCTION__);

    struct sigaction sigAction = {};
    sigAction.sa_handler       = signalHandler;
    sigemptyset(&sigAction.sa_mask);
    sigAction.sa_flags = 0;

    sigaction(SIGHUP, &sigAction, NULL);
    sigaction(SIGINT, &sigAction, NULL);
    sigaction(SIGTERM, &sigAction, NULL);
    sigaction(SIGTSTP, &sigAction, NULL);

    if (init() != telux::common::Status::SUCCESS) {

        if (eventManager_) {
            eventManager_ = nullptr;
        }
        if (naoIpTrigger_) {
            naoIpTrigger_ = nullptr;
        }
        if (smsTrigger_) {
            smsTrigger_ = nullptr;
        }
#ifdef TELSDK_FEATURE_SATCOM_ENABLED
        if (ntnClient_) {
            ntnClient_->cleanup();
            ntnClient_ = nullptr;
        }
#endif
        return EXIT_FAILURE;
    }

    if (consoleMode_) {
        initConsole();
        mainLoop();
    } else {
        // block current thread, till we get signal
        std::unique_lock<std::mutex> lock(mtx_);
        cv_.wait(lock, [this] { return exiting_.load(); });
    }
    return EXIT_SUCCESS;
}

void PowerRefDaemon::stopDaemon() {
    LOG(DEBUG, __FUNCTION__);
    exiting_ = true;
    if (naoIpTrigger_)
        naoIpTrigger_.reset();
    if (smsTrigger_)
        smsTrigger_.reset();
    if (eventManager_) {
        eventManager_->cleanup();
        eventManager_.reset();
    }
#ifdef TELSDK_FEATURE_SATCOM_ENABLED
    if (ntnClient_) {
        ntnClient_.reset();
    }
#endif
    fflush(stdout);
    cv_.notify_all();
}

void PowerRefDaemon::signalHandler(int signum) {
    LOG(DEBUG, __FUNCTION__, "Received signal = ", signum, " terminating program.");
    PowerRefDaemon::getInstance().stopDaemon();
}

void PowerRefDaemon::printUsage(char **argv) {
    std::cout << "Usage: " << std::string(argv[0]) << " [options] " << std::endl;
    std::cout << "Options: " << std::endl;
    std::cout << "\t -h --help        Print helpful information" << std::endl;
    std::cout << "\t -s --slave       Run in slave mode" << std::endl;
    std::cout << "\t -c --console     Run in console mode" << std::endl;
    std::cout << "\t -k --kpi         Enable KPI logging" << std::endl;
    std::cout << "Example: " << std::endl;
    std::cout << "   ./telux_power_refd " << std::endl;
    std::cout << "   ./telux_power_refd -s     To run in slave mode" << std::endl;
    std::cout << "   ./telux_power_refd -c     To run in console mode" << std::endl;
    std::cout << "   ./telux_power_refd -k     To enable KPI logging" << std::endl;
    std::cout << std::endl;
}

telux::common::Status PowerRefDaemon::parseArguments(
    int argc, char **argv, bool &isSlave, bool &isConsole) {
    LOG(DEBUG, __FUNCTION__);
    int c;
    struct option long_options[] = {{"help", no_argument, 0, 'h'}, {"slave", no_argument, 0, 's'},
        {"console", no_argument, 0, 'c'}, {"kpi", no_argument, 0, 'k'}, {0, 0, 0, 0}};

    while (1) {
        int option_index = 0;
        c                = getopt_long(argc, argv, "hsck", long_options, &option_index);
        /* Detect the end of the options. */
        if (c == -1) {
            break;
        }
        switch (c) {
            case 's':
                isSlave = true;
                break;
            case 'c':
                isConsole = true;
                break;
            case 'k':
                // KPI logging is handled in PowerRefDaemonMain.cpp
                break;
            case 'h':
            default:
                printUsage(argv);
                return telux::common::Status::INVALIDPARAM;
                break;
        }
    }
    return telux::common::Status::SUCCESS;
}

void PowerRefDaemon::initConsole() {
    LOG(DEBUG, __FUNCTION__);
    // Initialize console commands
    std::shared_ptr<ConsoleAppCommand> suspendCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("1", "Suspend_System", {},
            std::bind(&PowerRefDaemon::triggerActivityState, this, TcuActivityState::SUSPEND)));

    std::shared_ptr<ConsoleAppCommand> resumeCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("2", "Resume_System", {},
            std::bind(&PowerRefDaemon::triggerActivityState, this, TcuActivityState::RESUME)));

    std::shared_ptr<ConsoleAppCommand> shutdownCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("3", "Shutdown_System", {},
            std::bind(&PowerRefDaemon::triggerActivityState, this, TcuActivityState::SHUTDOWN)));

    std::shared_ptr<ConsoleAppCommand> setTimerCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
            "4", "Set_Resume_Timer", {}, std::bind(&PowerRefDaemon::configureResumeTimer, this)));

    std::vector<std::shared_ptr<ConsoleAppCommand>> commandsList
        = {suspendCommand, resumeCommand, shutdownCommand, setTimerCommand};

    ConsoleApp::addCommands(commandsList);
    ConsoleApp::displayMenu();
}

void PowerRefDaemon::triggerActivityState(TcuActivityState state) {
    LOG(DEBUG, __FUNCTION__);

    std::string machineName = ALL_MACHINES;
    std::cout << "Enter machine name (or leave empty for ALL_MACHINES): ";
    std::string input;
    std::getline(std::cin, input);

    if (!input.empty()) {
        machineName = input;
    }

    std::shared_ptr<Event> event
        = std::make_shared<Event>(state, machineName, TriggerType::CONSOLE_TRIGGER);

    if (event) {
        if (eventManager_) {
            RefAppUtils::logKpiFile(event);
            eventManager_->pushEvent(event);
            std::cout << "Event triggered: " << event->toString() << std::endl;
        } else {
            LOG(ERROR, __FUNCTION__, "Event manager is not available");
        }
    } else {
        LOG(ERROR, __FUNCTION__, "Unable to create event");
    }
}

void PowerRefDaemon::setConsoleMode(bool enable) {
    consoleMode_ = enable;
}

void PowerRefDaemon::configureResumeTimer() {
    LOG(DEBUG, __FUNCTION__);

    // Get timer duration from user
    std::cout << "Enter resume timer duration in seconds: ";
    std::string input;
    std::getline(std::cin, input);

    int seconds = 0;
    try {
        seconds = std::stoi(input);
        if (seconds <= 0) {
            std::cout << "Invalid timer duration. Please enter a positive number." << std::endl;
            return;
        }
    } catch (const std::exception &e) {
        std::cout << "Invalid input. Please enter a valid number." << std::endl;
        return;
    }

    // Get machine name from user
    std::string machineName = "";
    std::cout << "Enter machine name (or leave empty for ALL_MACHINES): ";
    std::getline(std::cin, input);

    if (!input.empty()) {
        machineName = input;
    }

    // Create and configure the timer
    if (createResumeTimer(seconds, machineName)) {
        std::cout << "Resume timer set for " << seconds << " seconds" << std::endl;
        if (!machineName.empty()) {
            std::cout << "Target machine: " << machineName << std::endl;
        }
    } else {
        std::cout << "Failed to set resume timer" << std::endl;
    }
}

bool PowerRefDaemon::createResumeTimer(int seconds, const std::string &machineName) {
    LOG(DEBUG, __FUNCTION__);

    // Store the machine name for use in the timer callback
    timerMachineName_ = machineName;

    // Create timer
    struct sigevent sev;
    memset(&sev, 0, sizeof(struct sigevent));

    sev.sigev_notify          = SIGEV_THREAD;
    sev.sigev_notify_function = &PowerRefDaemon::timerCallback;
    sev.sigev_value.sival_ptr = this;  // Pass this pointer to the callback

    // Delete existing timer if any
    if (timerId_ != 0) {
        timer_delete(timerId_);
        timerId_ = 0;
    }

    // Create a new timer
    if (timer_create(CLOCK_BOOTTIME_ALARM, &sev, &timerId_) == -1) {
        LOG(ERROR, __FUNCTION__, "Failed to create timer: ", strerror(errno));
        return false;
    }

    // Configure timer
    struct itimerspec its;
    memset(&its, 0, sizeof(struct itimerspec));

    its.it_value.tv_sec     = seconds;
    its.it_value.tv_nsec    = 0;
    its.it_interval.tv_sec  = 0;  // One-shot timer
    its.it_interval.tv_nsec = 0;

    if (timer_settime(timerId_, 0, &its, NULL) == -1) {
        LOG(ERROR, __FUNCTION__, "Failed to set timer: ", strerror(errno));
        timer_delete(timerId_);
        timerId_ = 0;
        return false;
    }

    LOG(DEBUG, __FUNCTION__, "Resume timer set for ", seconds, " seconds");
    return true;
}

void PowerRefDaemon::timerCallback(union sigval sv) {
    // Get the PowerRefDaemon instance from the sigval
    PowerRefDaemon *daemon = static_cast<PowerRefDaemon *>(sv.sival_ptr);
    if (daemon) {
        daemon->handleTimerExpiry();
    }
}

void PowerRefDaemon::handleTimerExpiry() {
    LOG(DEBUG, __FUNCTION__, "Resume timer expired");

    // Acquire wake lock to prevent the system from going back to sleep
    writeToSystemNode(WAKELOCK_PATH, RESUME_TIMER_WAKELOCK, strlen(RESUME_TIMER_WAKELOCK));

    // Create and trigger a resume event
    std::shared_ptr<Event> event = std::make_shared<Event>(TcuActivityState::RESUME,
        timerMachineName_.empty() ? ALL_MACHINES : timerMachineName_, TriggerType::TIMER_TRIGGER);

    if (event && eventManager_) {
        LOG(DEBUG, __FUNCTION__, "Triggering resume event");
        RefAppUtils::logKpiFile(event);
        eventManager_->pushEvent(event);
    } else {
        LOG(ERROR, __FUNCTION__, "Failed to create or push resume event");
    }

    // Release the wake lock after a short delay to ensure the event is processed
    std::this_thread::sleep_for(std::chrono::seconds(2));
    writeToSystemNode(WAKEUNLOCK_PATH, RESUME_TIMER_WAKELOCK, strlen(RESUME_TIMER_WAKELOCK));
    std::cout << "** Resume timer expired **" << std::endl;
}

void PowerRefDaemon::writeToSystemNode(const char *nodepath, const char *value, size_t length) {
    int fd = open(nodepath, O_WRONLY | O_APPEND | O_NONBLOCK);
    if (fd < 0) {
        LOG(ERROR, __FUNCTION__, "Opening of ", nodepath, " node failed: ", strerror(errno));
    } else {
        if (write(fd, value, length) == -1) {
            LOG(ERROR, __FUNCTION__, "Writing to ", nodepath, " node failed: ", strerror(errno));
        }
        close(fd);
    }
}
