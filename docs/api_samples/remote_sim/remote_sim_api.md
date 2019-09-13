# Remote SIM Manager API Sample Reference

This section demonstrates how to use the Remote SIM Manager API for remote SIM card operations.

### 1. Get the PhoneFactory and RemoteSimManager instances

   ~~~~~~{.cpp}
   #include <telux/tel/PhoneFactory.hpp>

   using namespace telux::common;
   using namespace telux::tel;

   PhoneFactory &phoneFactory = PhoneFactory::getInstance();
   std::shared_ptr<IRemoteSimManager> remoteSimMgr =
       phoneFactory.getRemoteSimManager(DEFAULT_SLOT_ID);
   ~~~~~~

### 2. Instantiate and register RemoteSimListener

   ~~~~~~{.cpp}
   std::shared_ptr<IRemoteSimListener> listener = std::make_shared<RemoteSimListener>();
   remoteSimMgr.registerListener(listener);
   ~~~~~~

###### 2.1 Implementation of IRemoteSimListener interface for receiving Remote SIM notifications

   ~~~~~~{.cpp}
   class RemoteSimListener : public IRemoteSimListener {
   public:
       void onApduTransfer(const unsigned int id, const std::vector<uint8_t> &apdu) override {
           // Send APDU to SIM card
       }
       void onCardConnect() override {
           // Connect to SIM card and request AtR
       }
       void onCardDisconnect() override {
           // Disconnect from SIM card
       }
       void onCardPowerUp() override {
           // Power up SIM card and request AtR
       }
       void onCardPowerDown() override {
           // Power down SIM card
       }
       void onCardReset() override {
           // Reset SIM card
       }
       void onServiceStatusChange(ServiceStatus status) {
           // Handle case where modem goes down or comes up
       }
   };
   ~~~~~~

###### 2.1 Implementation of event callback for asynchronous requests

   ~~~~~~{.cpp}
   void eventCallback(ErrorCode errorCode) {
       std::cout << "Received event response with errorcode " << static_cast<int>(errorCode)
           << std::endl;
   }
   ~~~~~~

### 3. Wait for Remote SIM subsystem initialization

   ~~~~~~{.cpp}
   int timeoutSec = 5;
   if (!(remoteSimMgr->isSubsystemReady())) {
       auto f = remoteSimMgr->onSubsystemReady();
       if (f.wait_for(std::chrono::seconds(timeoutSec)) != std::future_status::ready) {
           std::cout << "Remote SIM subsystem did not initialize!" << std::endl;
       }
   }
   ~~~~~~

### 4. Send connection available event request

   When the remote card is available and ready, make it available to the modem by sending a
   connection available request.

   ~~~~~~{.cpp}
   if (remoteSimMgr->sendConnectionAvailable(eventCallback) != Status::SUCCESS) {
       std::cout << "Failed to send connection available request!" << std::endl;
   }
   ~~~~~~

### 5. Send card reset request after receiving onCardConnect() notification from listener

   You will receive an onCardConnect notification on the listener when the modem accepts the
   connection.

   ~~~~~~{.cpp}
   // After connecting to SIM card, requesting AtR, and receiving response with AtR bytes

   if (remoteSimMgr->sendCardReset(atr, eventCallback) != Status::SUCCESS) {
       std::cout << "Failed to send card reset request!" << std::endl;
   }
   ~~~~~~

### 6. Send response APDU after receiving onTransmitApdu() notification from listener

   ~~~~~~{.cpp}
   // After sending command APDU to SIM and receiving the response

   if (remoteSimMgr->sendApdu(id, apdu, true, apdu.size(), 0, eventCallback) != Status::SUCCESS) {
       std::cout << "Failed to send response APDU!" << std::endl;
   }
   ~~~~~~

### 7. Send connection unavailable request before exiting

   When the card becomes unavailable (or before you exit), tear down the connection with the modem.

   ~~~~~~{.cpp}
   if (remoteSimMgr->sendConnectionUnavailable() != Status::SUCCESS) {
       std::cout << "Failed to send connection unavailable request!" << std::endl;
   }
   ~~~~~~
