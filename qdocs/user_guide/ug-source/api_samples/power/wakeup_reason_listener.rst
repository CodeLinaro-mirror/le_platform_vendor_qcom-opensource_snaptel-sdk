.. _wakeup-reason-listener:

Wakeup Reason Listener
======================

This sample application demonstrates how to register a listener to receive wakeup event notifications. The application shows how to handle both QMI transaction wakeups and Wake-on-Wireless (WoW) events.


1. Implement IWakeupListener and IServiceStatusListener interfaces

.. code-block::

   class WakeupReasonListener : public telux::power::IWakeupListener,
                                public telux::common::IServiceStatusListener {
   public:
       void onWakeup(const telux::power::WakeupEventInfo &eventInfo) override {
           // eventInfo.type: Indicates wakeup event type (QMI or WoW)
           // eventInfo.qmiWakeupInfo: QMI transaction details (serviceId, nodeIds, msgId, pid, processName)
           // eventInfo.wowWakeupInfo: Wireless event details (timestamp, wakeupReason, macAddress, interfaceName, pbmBuffer)
           // Avoid long blocking calls when handling notifications
       }

       void onServiceStatusChange(telux::common::ServiceStatus status) override {
           // status: Indicates service availability (SERVICE_AVAILABLE or SERVICE_UNAVAILABLE)
           // Process service status transitions accordingly
       }
   };


2. Get an instance of PowerFactory and obtain a WakeupManager instance

.. code-block::

   auto &powerFactory = telux::power::PowerFactory::getInstance();
   std::promise<telux::common::ServiceStatus> prom = std::promise<telux::common::ServiceStatus>();
   auto wakeupMgr = powerFactory.getWakeupManager(
       [&](telux::common::ServiceStatus status) {
           std::cout << " Init Callback called " << std::endl;
           prom.set_value(status);
       });


3. Wait for the wakeup management services to be initialized and ready

.. code-block::

   if (wakeupMgr == nullptr) {
       std::cout << " ERROR - Failed to get manager instance" << std::endl;
   }
   std::cout << "  Waiting for Wakeup Manager to be ready" << std::endl;
   telux::common::ServiceStatus serviceStatus = prom.get_future().get();


4. Exit the application, if SDK is unable to initialize wakeup management service

.. code-block::

   if (serviceStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
       std::cout << " *** Wakeup management service is Ready *** " << std::endl;
   } else {
       std::cout << " *** ERROR - Unable to initialize wakeup management service" << std::endl;
       return 1;
   }


5. Instantiate WakeupReasonListener and register for wakeup event notifications

.. code-block::

   auto listener = std::make_shared<WakeupReasonListener>();
   telux::power::WakeupIndications indications;
   indications.set(telux::power::WakeupIndicationsType::DEFAULT);
   indications.set(telux::power::WakeupIndicationsType::QMI_WAKEUP);
   indications.set(telux::power::WakeupIndicationsType::WOW_WAKEUP);
   
   auto ec = wakeupMgr->registerListener(listener, indications);
   if (ec != telux::common::ErrorCode::SUCCESS) {
       std::cout << "Can't register listener" << std::endl;
       return -1;
   }


6. Wait for wakeup event notifications

   The listener will receive wakeup events through the onWakeup callback. The application waits for events to be processed:

.. code-block::

   std::cout << "Listener registered, waiting for wakeup events..." << std::endl;
   // The onWakeup callback will be invoked when wakeup events occur
   // Avoid long blocking calls when handling notifications
   std::this_thread::sleep_for(std::chrono::seconds(60));


7. Deregister the listener when finished

.. code-block::

   wakeupMgr->deRegisterListener(listener);
   std::cout << "Listener deregistered" << std::endl;


Wakeup Event Types

The application can receive two types of wakeup events:


1. QMI Wakeup Events - Triggered by QMI transactions
   - Contains service ID, node IDs, message ID
   - Includes process information (PID, process name)


2. Wake-on-Wireless (WoW) Events - Triggered by wireless events
   - Contains timestamp and wakeup reason code
   - Includes MAC address and interface name
   - Includes pattern buffer (pbmBuffer) information
   - Supports various WoW reasons
