..
   *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
   *  SPDX-License-Identifier: BSD-3-Clause-Clear
.. _cell-channel-info:

Request Cell Channel Info
==================================================================

This sample application demonstrates how to request LTE and NR5G channel information for serving
cells (Primary and Secondary) and neighboring cells.

1. Implement ResponseCallback interface to receive subsystem initialization status

.. code-block::

   std::promise<telux::common::ServiceStatus> cbProm = std::promise<telux::common::ServiceStatus>();
   void initResponseCb(telux::common::ServiceStatus status) {
      cbProm.set_value(status);
   }


2. Get the PhoneFactory and PhoneManager instance

.. code-block::

   auto &phoneFactory = telux::tel::PhoneFactory::getInstance();
   auto phoneManager = phoneFactory.getPhoneManager(initResponseCb);
   if(phoneManager == NULL) {
      std::cout << " Failed to get Phone Manager instance" << std::endl;
      return -1;
   }


3. Wait for Phone Manager subsystem to be ready

.. code-block::

   telux::common::ServiceStatus status = cbProm.get_future().get();
   if(status != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
      std::cout << Unable to initialize Phone Manager subsystem << std::endl;
      return -1;
   }


4. Get the phone object corresponding to a phone ID

.. code-block::

   auto phone = phoneManager->getPhone(DEFAULT_PHONE_ID);


5. Implement CellChannelInfoListCallback and request cell channel information

.. code-block::

   auto channelInfoCallback = [](telux::tel::CellChannelInfoList channelInfoList,
         telux::common::ErrorCode error) {
      if(error == telux::common::ErrorCode::SUCCESS) {
         std::cout << "LTE Serving/SCell ARFCNs: "
                   << channelInfoList.lteServingAndSCellArfcn.size() << std::endl;
         std::cout << "LTE Neighbor ARFCNs: " << channelInfoList.lteNeighborArfcn.size()
                   << std::endl;
         std::cout << "NR5G Serving/SCell ARFCNs: "
                   << channelInfoList.nr5gServingAndSCellArfcn.size() << std::endl;
         std::cout << "NR5G Neighbor ARFCNs: " << channelInfoList.nr5gNeighborArfcn.size()
                   << std::endl;
      } else {
         std::cout << "requestCellChannelInfo failed, errorCode: " << static_cast<int>(error)
                   << std::endl;
      }
   };

   telux::common::Status status = phone->requestCellChannelInfo(channelInfoCallback);
   if(status != telux::common::Status::SUCCESS) {
      std::cout << "Can't request cell channel info, err " << static_cast<int>(status)
                << std::endl;
      return -1;
   }


After receiving the response in channelInfoCallback, LTE serving/neighbor and NR5G serving/neighbor
channel information can be accessed through the CellChannelInfoList structure. The application can
also register an IPhoneListener and override onCellChannelInfoListChanged() to receive a notification
whenever the channel information changes.
