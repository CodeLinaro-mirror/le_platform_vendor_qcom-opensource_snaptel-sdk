# C-V2X TX Sample App

This Document walks through the cv2x_tx_app sample application.

### 1. Create Callback functions for ICv2xRadio and ICv2xRadioManager methods ###
   ~~~~~~{.cpp}
   // Globals
   static Cv2xStatus gCv2xStatus;
   static promise<ErrorCode> gCallbackPromise;
   static shared_ptr<ICv2xTxFlow> gSpsFlow;
   static array<char, G_BUF_LEN> gBuf;

   // Resets the global callback promise
   static inline void resetCallbackPromise(void) {
       gCallbackPromise = promise<ErrorCode>();
   }

   // Callback function for ICv2xRadioManager->requestCv2xStatus()
   static void cv2xStatusCallback(Cv2xStatus status, ErrorCode error) {
       if (ErrorCode::SUCCESS == error) {
           gCv2xStatus = status;
       }
       gCallbackPromise.set_value(error);
   }

   // Callback function for ICv2xRadio->createTxSpsFlow()
   static void createSpsFlowCallback(shared_ptr<ICv2xTxFlow> txSpsFlow,
                                     shared_ptr<ICv2xTxFlow> unusedFlow,
                                     ErrorCode spsError,
                                     ErrorCode unusedError) {
       if (ErrorCode::SUCCESS == spsError) {
           gSpsFlow = txSpsFlow;
       }
       gCallbackPromise.set_value(spsError);
   }

   // Callback for ICv2xRadio->closeTxFlow()
   static void closeFlowCallback(shared_ptr<ICv2xTxFlow> flow, ErrorCode error) {
       gCallbackPromise.set_value(error);
   }
   ~~~~~~
Note: We can also use Lambda functions instead of defining global scope callback functions.

### 2. Get a handle to the ICv2xRadioManager object ###
   ~~~~~~{.cpp}
   int main {
       // Get handle to Cv2xRadioManager
       auto & cv2xFactory = Cv2xFactory::getInstance();
       auto cv2xRadioManager = cv2xFactory.getCv2xRadioManager();
   ~~~~~~

### 3. Request the C-V2X status ###
We want to verify that the C-V2X TX status is ACTIVE before we try to send data.
   ~~~~~~{.cpp}
       // Get C-V2X status and make sure Rx is enabled
       assert(Status::SUCCESS == cv2xRadioManager->requestCv2xStatus(cv2xStatusCallback));
       assert(ErrorCode::SUCCESS == gCallbackPromise.get_future().get());

       if (Cv2xStatusType::ACTIVE == gCv2xStatus.txStatus) {
           cout << "C-V2X TX status is active" << endl;
       }
       else {
           cerr << "C-V2X TX is inactive" << endl;
           return EXIT_FAILURE;
       }
   ~~~~~~

### 4. Get handle to C-V2X Radio ###
   ~~~~~~{.cpp}
       auto cv2xRadio = cv2xRadioManager->getCv2xRadio(TrafficCategory::SAFETY_TYPE);
   ~~~~~~

### 5. Wait for C-V2X Radio to be ready ###
   ~~~~~~{.cpp}
       if (not cv2xRadio->isReady()) {
           if (Status::SUCCESS == cv2xRadio->onReady().get()) {
               cout << "C-V2X Radio is ready" << endl;
           }
           else {
               cerr << "C-V2X Radio initialization failed." << endl;
               return EXIT_FAILURE;
           }
       }
   ~~~~~~

### 6. Create TX SPS flow and send data using TX socket ###
   ~~~~~~{.cpp}
       // Set SPS parameters
       SpsFlowInfo spsInfo;
       spsInfo.priority = Priority::PRIORITY_2;
       spsInfo.periodicity = Periodicity::PERIODICITY_100MS;
       spsInfo.nbytesReserved = G_BUF_LEN;
       spsInfo.autoRetransEnabledValid = true;
       spsInfo.autoRetransEnabled = true;

       // Create new SPS flow
       resetCallbackPromise();
       assert(Status::SUCCESS == cv2xRadio->createTxSpsFlow(TrafficIpType::TRAFFIC_NON_IP,
                                                            SPS_SERVICE_ID,
                                                            spsInfo,
                                                            SPS_SRC_PORT_NUM,
                                                            false,
                                                            0,
                                                            createSpsFlowCallback));
       assert(ErrorCode::SUCCESS == gCallbackPromise.get_future().get());

       // Send message in a loop
       for (uint16_t i = 0; i < NUM_TEST_ITERATIONS; ++i) {
           fillBuffer();
           sampleSpsTx();
           usleep(100000u);
       }

   ~~~~~~

### 7. Close TX SPS flow ###
   ~~~~~~{.cpp}
       // Deregister SPS flow
       resetCallbackPromise();
       assert(Status::SUCCESS == cv2xRadio->closeTxFlow(gSpsFlow, closeFlowCallback));
       assert(ErrorCode::SUCCESS == gCallbackPromise.get_future().get());

       return EXIT_SUCCESS;
   } // main
   ~~~~~~
