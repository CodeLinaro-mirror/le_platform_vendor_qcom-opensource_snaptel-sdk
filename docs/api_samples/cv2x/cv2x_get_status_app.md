Get CV2X service status {#cv2x_get_status_app}
==================================================

This sample app demonstrates how to use the C-V2X Radio Manager API to get the C-V2X status.

### 1. Create a RequestCv2xStatusCallback method

   ~~~~~~{.cpp}
   static Cv2xStatus gCv2xStatus;
   static promise<ErrorCode> gCallbackPromise;

   static map<Cv2xStatusType, string> gCv2xStatusToString = {
       {Cv2xStatusType::INACTIVE, "Inactive"},
       {Cv2xStatusType::ACTIVE, "Active"},
       {Cv2xStatusType::SUSPENDED, "SUSPENDED"},
       {Cv2xStatusType::UNKNOWN, "UNKNOWN"},
   };

   // Callback method for Cv2xRadioManager->requestCv2xStatus
   static void cv2xStatusCallback(Cv2xStatus status, ErrorCode error) {
       if (ErrorCode::SUCCESS == error) {
           gCv2xStatus = status;
       }
       gCallbackPromise.set_value(error);
   }
   ~~~~~~
Note: as an alternative, we can use a Lambda function which would eliminate the need for this global scope.

### 2. Get a handle to the ICv2xRadioManager instance

   ~~~~~~{.cpp}
   auto & cv2xFactory = Cv2xFactory::getInstance();
   auto cv2xRadioManager = cv2xFactory.getCv2xRadioManager();
   ~~~~~~

### 3. Request the C-V2X status

   ~~~~~~{.cpp}
   if (Status::SUCCESS != cv2xRadioManager->requestCv2xStatus(cv2xStatusCallback)) {
       cout << "Error : request for C-V2X status failed." << endl;
       return EXIT_FAILURE;
   }
   if (ErrorCode::SUCCESS != gCallbackPromise.get_future().get()) {
       cout << "Error : failed to retrieve C-V2X status." << endl;
           return EXIT_FAILURE;
   }

   if (Cv2xStatusType::ACTIVE == gCv2xStatus.rxStatus) {
       cout << "C-V2X Status:" << endl
            << "  RX : " << gCv2xStatusToString[gCv2xStatus.rxStatus] << endl
            << "  TX : " << gCv2xStatusToString[gCv2xStatus.txStatus] << endl;
   }
   ~~~~~~
