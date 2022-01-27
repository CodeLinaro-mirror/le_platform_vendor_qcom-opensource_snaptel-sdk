Set TCU activity state in passive mode {#set_tcu_activity_state_passive_mode}
===========================================================================

This sample app demonstrates how to change the TCU-activity power state in passive mode.

### 1. Implement a command response callback method

   ~~~~~~{.cpp}
   // This is invoked with error code indicating whether the request
   // was SUCCESS or FAILURE
   void commandResponse(ErrorCode error) {
       if (error == ErrorCode::SUCCESS) {
           std::cout << " Command executed successfully" << std::endl;
       } else {
           std::cout << " Command failed, errorCode: " << static_cast<int>(error) << std::endl;
       }
   }
   ~~~~~~

### 2. Implement ITcuActivityListener and IServiceStatusListener interface

   ~~~~~~{.cpp}
   class MyTcuActivityStateListener : public ITcuActivityListener,
                                       public IServiceStatusListener {
   public:
       void onTcuActivityStateUpdate(TcuActivityState state) override;
       void onSlaveAckStatusUpdate(telux::common::Status status);
       void onServiceStatusChange(ServiceStatus status) override;
   };
   ~~~~~~

### 3. Get an instance of PowerFactory

   ~~~~~~{.cpp}
   auto &powerFactory = PowerFactory::getInstance();
   ~~~~~~

### 4. Get TCU-activity manager instance with ClientType as MASTER

   ~~~~~~{.cpp}
   auto tcuActivityManager = powerFactory.getTcuActivityManager(ClientType::MASTER);
   ~~~~~~

### 5. Wait for the TCU-activity management services to be initialized and ready

   ~~~~~~{.cpp}
   bool isReady = tcuActivityManager->isReady();
   if(!isReady) {
      std::cout << "TCU-activity management service is not ready" << std::endl;
      std::cout << "Waiting uncondotionally for it to be ready " << std::endl;
      std::future<bool> f = tcuActivityManager->onReady();
      isReady = f.get();
   }
   ~~~~~~

### 6. Exit the application, if SDK is unable to initialize TCU-activity management service

   ~~~~~~{.cpp}
   if(isReady) {
      std::cout << " *** TCU-activity management service is Ready *** " << std::endl;
   } else {
      std::cout << " *** ERROR - Unable to initialize TCU-activity management service" << std::endl;
      return 1;
   }
   ~~~~~~

### 7. Instantiate MyTcuActivityStateListener

   ~~~~~~{.cpp}
   auto myTcuStateListener = std::make_shared<MyTcuActivityStateListener>();
   ~~~~~~

### 8. Register for updates on TCU-activity state and its management service status

   ~~~~~~{.cpp}
   tcuActivityManager->registerListener(myTcuStateListener);
   tcuActivityManager->registerServiceStateListener(myTcuStateListener);
   ~~~~~~

### 9. Set the TCU-activity state

   ~~~~~~{.cpp}
    tcuActivityManager->setActivityState(state,&commandResponse);
   ~~~~~~

### 10. Implement onTcuActivityStateUpdate which is invoked to notify that the TCU-activity state is changing to the desired state

   ~~~~~~{.cpp}
   void MyTcuActivityStateListener::onTcuActivityStateUpdate(TcuActivityState state) {
        std::cout << std::endl << "********* TCU-activity state update *********" << std::endl;
        // Avoid long blocking calls when handling notifications
   }
   ~~~~~~

### 11. On SUSPEND/SHUTDOWN notification, save any required information and send one(despite multiple listeners) acknowledgement

   ~~~~~~{.cpp}
   tcuActivityManager->sendActivityStateAck(TcuActivityStateAck);
   ~~~~~~

### 12. The below listener API is invoked to notify the acknowledgement status of all the clients in the system

   ~~~~~~{.cpp}
   void MyTcuActivityStateListener::onSlaveAckStatusUpdate(telux::common::Status status) {
        if (status == telux::common::Status::SUCCESS) {
            std::cout << " All clients acknowledged successfully" << std::endl;
        } else if (status == telux::common::Status::EXPIRED) {
            std::cout << " Timeout occured while waiting for all acknowledgements" << std::endl;
        } else {
            std::cout << " Failed to receive all acknowledgements, status: " << static_cast<int>(status) << std::endl;
        }
   }
   ~~~~~~

### 13. Implement onServiceStatusChange callback to know when TCU-activity management service goes down

   ~~~~~~{.cpp}
   // When the TCU-activity management service goes down, this API is invoked
   // with status UNAVAILABLE. All TCU-activity state notifications will be
   // stopped until the status becomes AVAILABLE again.
   void MyTcuActivityStateListener::onServiceStatusChange(ServiceStatus status) {
        std::cout << std::endl << "****** TCU-activity management service status update ******" << std::endl;
        // Avoid long blocking calls when handling notifications
   }
   ~~~~~~
