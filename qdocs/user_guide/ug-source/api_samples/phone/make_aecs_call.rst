..
   *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
   *  SPDX-License-Identifier: BSD-3-Clause-Clear

.. _make-aecs-call:

Make AECS Call
========================

This sample application demonstrates how to make an Automotive Accident Emergency Call System
(AECS) call.

1. Implement ResponseCallback interface to receive subsystem initialization status

.. code-block::

   std::promise<telux::common::ServiceStatus> cbProm = std::promise<telux::common::ServiceStatus>();
   void initResponseCb(telux::common::ServiceStatus status) {
      if(status == SERVICE_AVAILABLE) {
         std::cout << "Call Manager subsystem is ready" << std::endl;
      } else if(status == SERVICE_FAILED) {
         std::cout << "Call Manager subsystem initialization failed" << std::endl;
      }
      cbProm.set_value(status);
   }


2. Get the PhoneFactory and Call Manager instance

.. code-block::

   auto &phoneFactory = PhoneFactory::getInstance();
   auto callManager = phoneFactory.getCallManager(initResponseCb);
   if(callManager == NULL) {
      std::cout << " Failed to get Call Manager instance" << std::endl;
      return -1;
   }


3. Wait for Call Manager subsystem to be ready

.. code-block::

   telux::common::ServiceStatus status = cbProm.get_future().get();
   if(status != SERVICE_AVAILABLE) {
      std::cout << "Unable to initialize Call Manager subsystem" << std::endl;
      return -1;
   }


4. Enable emergency mode before initiating the AECS call

.. code-block::

   int phoneId = 1;
   bool antennaSwitchEnabled = false;
   auto emergencyModeStatus = callManager->setEmergencyMode(phoneId, true, antennaSwitchEnabled);
   std::cout << "Set Emergency Mode Status:" << (int)emergencyModeStatus << std::endl;


5. Optionally, implement MakeCallCallback to receive response for the dial request

.. code-block::

   telux::tel::MakeCallCallback aecsCallCb = [](telux::common::ErrorCode error,
                                                std::shared_ptr<telux::tel::ICall> call) {
      // will be invoked with response of makeAecsCall operation
      std::cout << "AECS Call Response Error Code:" << (int)error << std::endl;
   };


6. Send an AECS call request

.. code-block::

   std::string eCallIdentifier = "112";
   if(callManager) {
      auto makeCallStatus = callManager->makeAecsCall(phoneId, eCallIdentifier, aecsCallCb);
      std::cout << "Dial AECS Call Status:" << (int)makeCallStatus << std::endl;
   }


7. After the AECS call and MSD transmission are complete, disable emergency mode

.. code-block::

   auto disableEmergencyModeStatus = callManager->setEmergencyMode(phoneId, false,
                                                                   antennaSwitchEnabled);
   std::cout << "Disable Emergency Mode Status:" << (int)disableEmergencyModeStatus << std::endl;
