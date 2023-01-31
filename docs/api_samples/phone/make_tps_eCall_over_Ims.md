Make Third Party Service (TPS) eCall over IMS {#make_eCall_Over_Ims}
========================

This sample application demonstrates how to make a TPS eCall over IMS.

### 1. Get the PhoneFactory

   ~~~~~~{.cpp}
   auto &phoneFactory = PhoneFactory::getInstance();


### 2. Instantiate call manager

   ~~~~~~{.cpp}
   std::shared_ptr<ICallManager> callManager = phoneFactory.getCallManager();
   ~~~~~~


### 3. Initialize phoneId with default value

   ~~~~~~{.cpp}
   int phoneId = DEFAULT_PHONE_ID;
   ~~~~~~


### 4. Optionally, instantiate dial call instance

   ~~~~~~{.cpp}
   std::shared_ptr<DialCallback> dialCb = std::make_shared<DialCallback> ();
   ~~~~~~


##### 4.1. Optionally, implement IMakeCallCallback interface to receive response for the dial
#####      request

   ~~~~~~{.cpp}
   class DialCallback : public IMakeCallCallback {
   public:
      void makeCallResponse(ErrorCode error, std::shared_ptr<ICall> call) override;
   };

   void DialCallback::makeCallResponse(ErrorCode error, std::shared_ptr<ICall> call) {
      // will be invoked with response of makeECall operation
   }
   ~~~~~~


### 5. Optionally, instantiate dial call instance

    ~~~~~~{.cpp}
    CustomSipHeader header;
    std::string contentTypeHeader;
    std::string acceptInfoHeader;
    char delimiter = '\n';
    std::string temp = "";
    std::cout << "Enter Custom SIP Header for contentType (uses default for no input): ";
    std::getline(std::cin, temp, delimiter);
    if (!temp.empty()) {
        contentTypeHeader = temp;
    } else {
        std::cout << "No input, proceeding with default contentType: " << std::endl;
    }
    temp = "";
    std::cout << "Enter Custom SIP Header for acceptInfo (uses default for no input): ";
    std::getline(std::cin, temp, delimiter);
    if (!temp.empty()) {
        acceptInfoHeader = temp;
    } else {
        std::cout << "No input, proceeding with default acceptInfo: " << std::endl;
    }
    if (contentTypeHeader != "") {
        header.contentType = contentType;
    } else {
        header.contentType = telux::tel::CONTENT_HEADER;
    }
    if (acceptInfoHeader != "") {
        header.acceptInfo = acceptInfo;
    } else  {
        header.acceptInfo = "";
    }
    ~~~~~~


### 6. Initialize the data required for eCall such as dialnumber, msdData

   ~~~~~~{.cpp}
   std::string dialNumber = 77777777;
   std::vector<uint8_t> rawData;
   rawData = { 2, 41, 68, 6, 128, 227, 10, 81, 67, 158, 41, 85, 212, 56, 0, 128, 4, 52, 10, 140,
              65, 89, 164, 56, 119, 207, 131, 54, 210, 63, 65, 104, 16, 24, 8, 32, 19, 198, 68, 0,
              0, 48, 20 };
   ~~~~~~


### 7. Send a eCall request

   ~~~~~~{.cpp}
   if(callManager) {
      auto makeCallStatus = callManager->makeECall(phoneId, dialNumber, rawData, header, dialCb);
      std::cout << "Dial ECall Status:" << (int)makeCallStatus << std::endl;
   }
   ~~~~~~
