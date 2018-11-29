# C-V2X RX Sample App

This Document walks through the cv2x_rx_app sample application.

### 1. Create Callback functions for ICv2xRadio and ICv2xRadioManager methods ###
   ~~~~~~{.cpp}
   // Globals
   static Cv2xStatus gCv2xStatus;
   static promise<ErrorCode> gCallbackPromise;
   static shared_ptr<ICv2xRxSubscription> gRxSub;
   static uint32_t gPacketsReceived = 0u;
   static array<char, G_BUF_LEN> gBuf;

   // Resets the global callback promise
   static inline void resetCallbackPromise(void) {
       gCallbackPromise = promise<ErrorCode>();
   }

   // Callback function for Cv2xRadioManager->requestCv2xStatus()
   static void cv2xStatusCallback(Cv2xStatus status, ErrorCode error) {
       if (ErrorCode::SUCCESS == error) {
           gCv2xStatus = status;
       }
       gCallbackPromise.set_value(error);
   }

   // Callback function for Cv2xRadio->createRxSubscription() and Cv2xRadio->closeRxSubscription()
   static void rxSubCallback(shared_ptr<ICv2xRxSubscription> rxSub, ErrorCode error) {
       if (ErrorCode::SUCCESS == error) {
           gRxSub = rxSub;
       }
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
We want to verify that the C-V2X RX status is ACTIVE before we try to receive data.
   ~~~~~~{.cpp}
       // Get C-V2X status and make sure Rx is enabled
       assert(Status::SUCCESS == cv2xRadioManager->requestCv2xStatus(cv2xStatusCallback));
       assert(ErrorCode::SUCCESS == gCallbackPromise.get_future().get());

       if (Cv2xStatusType::ACTIVE == gCv2xStatus.rxStatus) {
           cout << "C-V2X RX status is active" << endl;
       }
       else {
           cerr << "C-V2X RX is inactive" << endl;
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

### 6. Create RX Subscription and receive data using RX socket ###
   ~~~~~~{.cpp}
       resetCallbackPromise();
       assert(Status::SUCCESS == cv2xRadio->createRxSubscription(TrafficIpType::TRAFFIC_NON_IP,
                                                                 RX_PORT_NUM,
                                                                 rxSubCallback));
       assert(ErrorCode::SUCCESS == gCallbackPromise.get_future().get());

       // Read from the RX socket in a loop
       for (uint32_t i = 0; i < NUM_TEST_ITERATIONS; ++i) {
           // Receive from RX socket
           sampleRx();
       }
   ~~~~~~

### 7. Close RX Subcription
We supply the callback in this sample and check its status, but note that it is optional.
~~~~~~{.cpp}
       resetCallbackPromise();
       assert(Status::SUCCESS == cv2xRadio->closeRxSubscription(gRxSub,
                                                                rxSubCallback));
       assert(ErrorCode::SUCCESS == gCallbackPromise.get_future().get());

       return EXIT_SUCCESS;
   } // main
~~~~~~
