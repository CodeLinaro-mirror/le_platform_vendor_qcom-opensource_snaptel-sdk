Serving system information, notification filter and domain preference {#serving_system}
=========================================================================================

This sample application demonstrates how to:
- Register/deregister a listener for serving system notifications.
- Configure and query the system information notification filter using
  setSystemInfoNotificationFilter() and getSystemInfoNotificationFilter().
- Request the current service domain preference.

### 1. Get phone factory and serving system manager instances

   ~~~~~~{.cpp}
   auto &phoneFactory = telux::tel::PhoneFactory::getInstance();
   std::promise<telux::common::ServiceStatus> p{};
   auto servingSystemMgr = phoneFactory.getServingSystemManager(DEFAULT_SLOT_ID,
      [&p](telux::common::ServiceStatus status) { p.set_value(status); });
   ~~~~~~

### 2. Wait for the serving system manager to become available

   ~~~~~~{.cpp}
   telux::common::ServiceStatus serviceStatus = p.get_future().get();
   if(serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
      std::cout << "Serving system service unavailable" << std::endl;
      return -EIO;
   }
   ~~~~~~

### 3. Implement a listener to receive notifications that are affected by the system information notification filter

   ~~~~~~{.cpp}
   class ServingSystemListener : public telux::tel::IServingSystemListener {
   public:
      void onSystemInfoChanged(telux::tel::ServingSystemInfo sysInfo) override {
         std::cout << "System info changed - RAT: " << static_cast<int>(sysInfo.rat)
                   << ", domain: " << static_cast<int>(sysInfo.domain)
                   << ", state: " << static_cast<int>(sysInfo.state) << std::endl;
      }

      void onCallBarringInfoChanged(
         std::vector<telux::tel::CallBarringInfo> barringInfo) override {
         std::cout << "Call barring info changed, count: " << barringInfo.size() << std::endl;
      }

      void onSmsCapabilityChanged(telux::tel::SmsCapability smsCapability) override {
         std::cout << "SMS capability changed - domain: "
                   << static_cast<int>(smsCapability.domain) << std::endl;
      }
   };
   ~~~~~~

### 4. Register the listener for SYSTEM_INFO notifications

Filtering configured through setSystemInfoNotificationFilter() only takes effect when the
SYSTEM_INFO notification type is enabled through registerListener().

   ~~~~~~{.cpp}
   auto listener = std::make_shared<ServingSystemListener>();

   telux::tel::ServingSystemNotificationMask mask;
   mask.set(telux::tel::ServingSystemNotificationType::SYSTEM_INFO);

   telux::common::Status status = servingSystemMgr->registerListener(listener, mask);
   if(status != telux::common::Status::SUCCESS) {
      std::cout << "Can't register listener, err " << static_cast<int>(status) << std::endl;
   }
   ~~~~~~

### 5. Set the system information notification filter

Restrict onSystemInfoChanged(), onCallBarringInfoChanged() and onSmsCapabilityChanged()
notifications to only the attributes selected in the filter mask, e.g. service registration
state transitions and serving RAT changes.

   ~~~~~~{.cpp}
   telux::tel::SystemInfoNotificationFilterMask filterMask;
   filterMask.set(telux::tel::SystemInfoNotificationFilter::SRV_STATUS);
   filterMask.set(telux::tel::SystemInfoNotificationFilter::SRV_RAT_TECH_CHANGE);

   telux::common::ErrorCode errorCode
      = servingSystemMgr->setSystemInfoNotificationFilter(filterMask);
   if(errorCode != telux::common::ErrorCode::SUCCESS) {
      std::cout << "Can't set system info notification filter, err "
                << static_cast<int>(errorCode) << std::endl;
   }
   ~~~~~~

An empty mask (the default) disables filtering, so all system information changes are
reported.

### 6. Get the currently configured system information notification filter

   ~~~~~~{.cpp}
   telux::tel::SystemInfoNotificationFilterMask currentMask;
   telux::common::ErrorCode errorCode
      = servingSystemMgr->getSystemInfoNotificationFilter(currentMask);
   if(errorCode == telux::common::ErrorCode::SUCCESS) {
      std::cout << "System info notification filter: " << currentMask.to_string() << std::endl;
   }
   ~~~~~~

### 7. Implement response callback to receive response for request service domain preference

   ~~~~~~{.cpp}
   class ServiceDomainResponseCallback {
   public:
      void serviceDomainResponse(telux::tel::ServiceDomainPreference preference,
                                 telux::common::ErrorCode errorCode) {
         if(errorCode == telux::common::ErrorCode::SUCCESS) {
            std::cout << "Service domain preference: "
                      << static_cast<int>(preference)
                      << std::endl;
         } else {
            std::cout << "\n requestServiceDomainPreference failed, ErrorCode: "
                      << static_cast<int>(errorCode)
                      << std::endl;
         }
      }
   };
   ~~~~~~

### 8. Send request service domain preference request along with required function object

   ~~~~~~{.cpp}
   if(servingSystemMgr) {
      ServiceDomainResponseCallback cb;
      telux::common::Status status = servingSystemMgr->requestServiceDomainPreference(
          std::bind(&ServiceDomainResponseCallback::serviceDomainResponse, &cb,
                  std::placeholders::_1, std::placeholders::_2));
      std::cout << static_cast<int>(status) << std::endl;
   }
   ~~~~~~

Now serviceDomainResponse() method will be invoked with current service domain preference.

### 9. Deregister the listener before the application exits

   ~~~~~~{.cpp}
   telux::tel::ServingSystemNotificationMask mask;
   mask.set(telux::tel::ServingSystemNotificationType::SYSTEM_INFO);

   telux::common::Status status = servingSystemMgr->deregisterListener(listener, mask);
   if(status != telux::common::Status::SUCCESS) {
      std::cout << "Can't deregister listener, err " << static_cast<int>(status) << std::endl;
   }
   ~~~~~~
