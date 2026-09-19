Network Registration Reject with Cell ID {#network_reject}
==========================================================

This sample application demonstrates how to listen for network registration rejection events
using `onNetworkRejection`. The notification provides accurate reject information
(RAT, PLMN, reject cause, and Cell ID) directly from the camped/serving cell.

### 1. Get phone factory and serving system manager instances

   ~~~~~~{.cpp}
   auto &phoneFactory = telux::tel::PhoneFactory::getInstance();
   std::promise<telux::common::ServiceStatus> prom;
   auto networkMgr = phoneFactory.getServingSystemManager(DEFAULT_SLOT_ID,
       [&](telux::common::ServiceStatus status) {
           prom.set_value(status);
   });
   ~~~~~~

### 2. Wait for the serving system subsystem initialization

   ~~~~~~{.cpp}
   telux::common::ServiceStatus subSystemStatus = prom.get_future().get();
   ~~~~~~

### 3. Exit the application, if serving system subsystem can not be initialzed

   ~~~~~~{.cpp}
   if (subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
       std::cout << "Serving System  Manager is ready " << "\n";
   } else {
       std::cout << "ERROR - Unable to initialize,"
           << " Serving System Manager subsystem " << std::endl;
       return 1;
   }
   ~~~~~~

### 4. Implement the listener to receive network rejection events

   ~~~~~~{.cpp}
   class NetworkRejectListener : public telux::tel::IServingSystemListener {
   public:
       void onNetworkRejection(telux::tel::NetworkRejectInfo rejectInfo) override {
           std::cout << "Network rejection occurred:\n"
               << "  RAT          : " << static_cast<int>(rejectInfo.rejectSrvInfo.rat)
               << "\n"
               << "  Service Domain: "
               << static_cast<int>(rejectInfo.rejectSrvInfo.domain) << "\n"
               << "  Reject Cause : " << static_cast<int>(rejectInfo.rejectCause) << "\n"
               << "  MCC          : " << rejectInfo.mcc << "\n"
               << "  MNC          : " << rejectInfo.mnc << "\n"
               << "  Cell ID      : ";
           if (rejectInfo.cellId == 0xFFFFFFFF) {
               std::cout << "UNAVAILABLE" << "\n";
           } else {
               std::cout << rejectInfo.cellId << "\n";
           }
       }
   };
   ~~~~~~

   The `cellId` field contains the identifier of the cell where the registration rejection
   occurred. It is set to "0xFFFFFFFFFFFFFFFF" when the modem does not provide a Cell identifier in
   the indication.

### 5. Register the listener for network rejection information notification

   ~~~~~~{.cpp}
   auto listener = std::make_shared<NetworkRejectListener>();
   telux::tel::ServingSystemNotificationMask mask;
   mask.set(telux::tel::ServingSystemNotificationType::NETWORK_REJ_INFO);
   servingSystemMgr->registerListener(listener, mask);
   ~~~~~~

### 6. Deregister the listener before exit

   ~~~~~~{.cpp}
   servingSystemMgr->deregisterListener(listener, mask);
   ~~~~~~

Now `onNetworkRejection()` will be invoked each time the modem sends a network reject indication,
with all available reject details including the Cell identifier of the rejecting cell.
