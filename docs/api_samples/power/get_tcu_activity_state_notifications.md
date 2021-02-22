Using TCU Activity Manager APIs to get TCU activity state updates {#get_tcu_activity_state_notifications}
=========================================================================================================

# Using TCU Activity Manager APIs to get TCU activity state updates

The below steps need to be followed by applications to listen to TCU-activity state notifications, for performing any tasks before the state transition.
These are valid in both ACTIVE and PASSIVE modes.

### 1. Implement ITcuActivityListener and IServiceStatusListener interface ###
   ~~~~~~{.cpp}
    class MyTcuActivityStateListener : public ITcuActivityListener,
                                       public IServiceStatusListener {
    public:
        void onTcuActivityStateUpdate(TcuActivityState state) override;
        void onServiceStatusChange(ServiceStatus status) override;
    };
   ~~~~~~

### 2. Get the Power-Factory instance ###
   ~~~~~~{.cpp}
    auto &powerFactory = PowerFactory::getInstance();
   ~~~~~~

### 3. Get TCU-activity manager instance with clientType as SLAVE ###
   ~~~~~~{.cpp}
    auto tcuActivityManager = powerFactory.getTcuActivityManager(ClientType::SLAVE);
   ~~~~~~

### 4. Wait for the TCU-activity management services to be initialized and ready ###
   ~~~~~~{.cpp}
    bool isReady = tcuActivityManager->isReady();
    if(!isReady) {
        std::cout << "TCU-activity management service is not ready" << std::endl;
        std::cout << "Waiting uncondotionally for it to be ready " << std::endl;
        std::future<bool> f = tcuActivityManager->onReady();
        isReady = f.get();
    }
   ~~~~~~

### 5. Exit the application, if SDK is unable to initialize TCU-activity management service ###
   ~~~~~~{.cpp}
    if(isReady) {
        std::cout << " *** TCU-activity management service is Ready *** " << std::endl;
    } else {
        std::cout << " *** ERROR - Unable to initialize TCU-activity management service" << std::endl;
        return 1;
    }
   ~~~~~~

### 6. Instantiate MyTcuActivityStateListener ###
   ~~~~~~{.cpp}
    auto myTcuStateListener = std::make_shared<MyTcuActivityStateListener>();
   ~~~~~~

### 7. Register for updates on TCU-activity state and its management service status ###
   ~~~~~~{.cpp}
    tcuActivityManager->registerListener(myTcuStateListener);
    tcuActivityManager->registerServiceStateListener(myTcuStateListener);
   ~~~~~~

### 8. Wait for the TCU-activity state updates ###
   ~~~~~~{.cpp}
    void MyTcuActivityStateListener::onTcuActivityStateUpdate(TcuActivityState state) {
        std::cout << std::endl << "********* TCU-activity state update *********" << std::endl;
        // Avoid long blocking calls when handling notifications
    }
   ~~~~~~

### 9. On SUSPEND/SHUTDOWN notification, save any required information and send one(despite multiple listeners) acknowledgement ###
   ~~~~~~{.cpp}
    tcuActivityManager->sendActivityStateAck(TcuActivityStateAck);
   ~~~~~~

### 10. When the TCU-activity management service goes down, this API is invoked with status UNAVAILABLE. All TCU-activity state notifications will be stopped until the status becomes AVAILABLE again ###
   ~~~~~~{.cpp}
    void MyTcuActivityStateListener::onServiceStatusChange(ServiceStatus status) {
        std::cout << std::endl << "****** TCU-activity management service status update ******" << std::endl;
        // Avoid long blocking calls when handling notifications
    }
   ~~~~~~