# Using TCU Activity Manager APIs to set the TCU activity state in ACTIVE mode

The below steps need to be followed by the applications that control the TCU-activity state, to change the TCU-activity state in ACTIVE mode.

### 1. Implement a command response function ###
   ~~~~~~{.cpp}
    void commandResponse(ErrorCode error) {
        if (error == ErrorCode::SUCCESS) {
            std::cout << " Command executed successfully" << std::endl;
        } else {
            std::cout << " Command failed, errorCode: " << static_cast<int>(error) << std::endl;
        }
    }
   ~~~~~~

### 2. Implement ITcuActivityListener and IServiceStatusListener interface. ###
   ~~~~~~{.cpp}
    class MyTcuActivityStateListener : public ITcuActivityListener,
                                       public IServiceStatusListener {
    public:
        void onTcuActivityStateUpdate(TcuActivityState state) override;
        void onServiceStatusChange(ServiceStatus status) override;
    };
   ~~~~~~

### 3. Get the Power-Factory instance ###
   ~~~~~~{.cpp}
    auto &powerFactory = PowerFactory::getInstance();
   ~~~~~~

### 4. Get TCU-activity manager instance with ClientType as MASTER ###
   ~~~~~~{.cpp}
    auto tcuActivityManager = powerFactory.getTcuActivityManager(ClientType::MASTER);
   ~~~~~~

### 5. Wait for the TCU-activity management services to be initialized and ready ###
   ~~~~~~{.cpp}
    bool isReady = tcuActivityManager->isReady();
    if(!isReady) {
        std::cout << "TCU-activity management service is not ready" << std::endl;
        std::cout << "Waiting uncondotionally for it to be ready " << std::endl;
        std::future<bool> f = tcuActivityManager->onReady();
        isReady = f.get();
    }
   ~~~~~~

### 6. Exit the application, if SDK is unable to initialize TCU-activity management service ###
   ~~~~~~{.cpp}
    if(isReady) {
        std::cout << " *** TCU-activity management service is Ready *** " << std::endl;
    } else {
        std::cout << " *** ERROR - Unable to initialize TCU-activity management service" << std::endl;
        return 1;
    }
   ~~~~~~

### 7. Instantiate MyTcuActivityStateListener ###
   ~~~~~~{.cpp}
    auto myTcuStateListener = std::make_shared<MyTcuActivityStateListener>();
   ~~~~~~

### 8. Register for updates on TCU-activity state and its management service status ###
   ~~~~~~{.cpp}
    tcuActivityManager->registerListener(myTcuStateListener);
    tcuActivityManager->registerServiceStateListener(myTcuStateListener);
   ~~~~~~

### 9. Set the TCU-activity state ###
   ~~~~~~{.cpp}
    tcuActivityManager->setActivityState(state,&commandResponse);
   ~~~~~~

### 10. Command response callback function is invoked with error code indicating whether the request was SUCCESS or FAILURE ###

### 11. This API on the listener is invoked to notify that the TCU-activity state is changing to the desired state ###
   ~~~~~~{.cpp}
    void MyTcuActivityStateListener::onTcuActivityStateUpdate(TcuActivityState state) {
        std::cout << std::endl << "********* TCU-activity state update *********" << std::endl;
        // Avoid long blocking calls when handling notifications
    }
   ~~~~~~

### 12. On SUSPEND/SHUTDOWN notification, save any required information and send one(despite multiple listeners) acknowledgement ###
   ~~~~~~{.cpp}
    tcuActivityManager->sendActivityStateAck(TcuActivityStateAck);
   ~~~~~~

### 13. When the TCU-activity management service goes down, the below listener API is invoked with status UNAVAILABLE. Any further TCU-activity state commands and notifications will not be served until the status becomes AVAILABLE again ###
   ~~~~~~{.cpp}
    void MyTcuActivityStateListener::onServiceStatusChange(ServiceStatus status) {
        std::cout << std::endl << "****** TCU-activity management service status update ******" << std::endl;
        // Avoid long blocking calls when handling notifications
    }
   ~~~~~~