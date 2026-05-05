Request Carrier Aggregation Info {#carrier_aggregation_info}
==================================================================

This sample application demonstrates how to request Carrier Aggregation information, comprising
the Primary Cell (PCell) and configured Secondary Cell(s) (SCell), for a given Radio Access
Technology (RAT).

### 1. Implement ResponseCallback interface to receive subsystem initialization status

   ~~~~~~{.cpp}
   std::promise<telux::common::ServiceStatus> cbProm = std::promise<telux::common::ServiceStatus>();
   void initResponseCb(telux::common::ServiceStatus status) {
      cbProm.set_value(status);
   }
   ~~~~~~

### 2. Get the PhoneFactory and PhoneManager instance

   ~~~~~~{.cpp}
   auto &phoneFactory = telux::tel::PhoneFactory::getInstance();
   auto phoneManager = phoneFactory.getPhoneManager(initResponseCb);
   if(phoneManager == NULL) {
      std::cout << " Failed to get Phone Manager instance" << std::endl;
      return -1;
   }
   ~~~~~~

### 3. Wait for Phone Manager subsystem to be ready

   ~~~~~~{.cpp}
   telux::common::ServiceStatus status = cbProm.get_future().get();
   if(status != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
      std::cout << Unable to initialize Phone Manager subsystem << std::endl;
      return -1;
   }
   ~~~~~~

### 4. Get the phone object corresponding to a phone ID

   ~~~~~~{.cpp}
   auto phone = phoneManager->getPhone(DEFAULT_PHONE_ID);
   ~~~~~~

### 5. Implement CaInfoCallback and request Carrier Aggregation information

   ~~~~~~{.cpp}
   auto caInfoCallback = [](const telux::tel::CarrierAggregationInfo &info,
         telux::common::ErrorCode error) {
      if(error == telux::common::ErrorCode::SUCCESS && info.lteInfo) {
         std::cout << "PCell PCI: " << info.lteInfo->pCell.cell.pci << std::endl;
         std::cout << "Number of SCells: " << info.lteInfo->sCell.size() << std::endl;
      } else {
         std::cout << "requestCarrierAggregationCellInfo failed, errorCode: "
                   << static_cast<int>(error) << std::endl;
      }
   };

   telux::common::Status status = phone->requestCarrierAggregationCellInfo(
      telux::tel::CarrierAggregationType::LteCa, caInfoCallback);
   if(status != telux::common::Status::SUCCESS) {
      std::cout << "Can't request Carrier Aggregation info, err " << static_cast<int>(status)
                << std::endl;
      return -1;
   }
   ~~~~~~

After receiving the response in caInfoCallback, the Primary Cell and Secondary Cell(s) information
can be accessed through the CarrierAggregationInfo structure. The application can also register an
IPhoneListener and override onCarrierAggregationInfoChanged() to receive a notification whenever the
Carrier Aggregation configuration changes.
