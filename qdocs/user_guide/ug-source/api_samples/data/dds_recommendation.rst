..
   *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
   *  SPDX-License-Identifier: BSD-3-Clause-Clear

.. _dds-recommendation:

DDS Recommendation
==================

This sample application demonstrates how to receive and act on Default Data Subscription
(DDS) switch recommendations from the modem. The scenario covered is a voice call received
on the nDDS SIM slot (Sub2) that causes the device to transition from DSDA mode to DSDS
mode — meaning only one radio resource is now available and data can no longer run on both
SIM slots simultaneously. The modem recommends a temporary DDS switch to Sub2 so internet
data follows the active radio resource, and then switches back once the call ends.

1. Get the DataFactory and IDualDataManager instances

Provide an initialization callback. ``DataFactory`` invokes the callback once the
``IDualDataManager`` service is ready.

.. code-block:: cpp

   std::promise<telux::common::ServiceStatus> p;
   auto &dataFactory = telux::data::DataFactory::getInstance();
   auto dualDataMgr = dataFactory.getDualDataManager(
       [&p](telux::common::ServiceStatus status) {
           p.set_value(status);
       });
   if (!dualDataMgr) {
       std::cout << "Failed to get IDualDataManager" << std::endl;
       return -1;
   }

2. Wait for the DualData service to become available

.. code-block:: cpp

   auto serviceStatus = p.get_future().get();
   if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
       std::cout << "DualData service unavailable" << std::endl;
       return -2;
   }

3. Implement IDualDataListener and register it

The listener receives two key callbacks:

- ``onDualDataUsageRecommendationChange`` — signals whether simultaneous long-running data on
  both SIM slots is currently allowed, not recommended, or forbidden.
- ``onDdsSwitchRecommendation`` — delivers the modem's recommendation to switch the DDS slot.

.. code-block:: cpp

   class MyDualDataListener : public telux::data::IDualDataListener {
   public:
       explicit MyDualDataListener(std::shared_ptr<telux::data::IDualDataManager> mgr)
           : dualDataMgr_(mgr) {}

       // Step 5: Called when dual data usage suitability changes.
       // NOT_ALLOWED     → device moved to DSDS sub-mode; simultaneous long-running data on
       //                   both slots is no longer possible.
       // NOT_RECOMMENDED → device moved to DSDA TxS mode; stop data on nDDS to avoid degradation.
       // ALLOWED         → DSDA FC mode; both slots may carry long-running data.
       void onDualDataUsageRecommendationChange(
           telux::data::DualDataUsageRecommendation recommendation) override {
           if (recommendation == telux::data::DualDataUsageRecommendation::NOT_ALLOWED) {
               std::cout << "NOT_ALLOWED: simultaneous data on both slots no longer possible."
                         << std::endl;
           } else if (recommendation == telux::data::DualDataUsageRecommendation::NOT_RECOMMENDED) {
               std::cout << "NOT_RECOMMENDED: stop data on nDDS to avoid degradation." << std::endl;
               // OEM action: pause/stop data activities on the nDDS slot.
           }
       }

       // Step 6: Called when the modem recommends a DDS switch.
       // For TEMPORARY/HIGH with DSDA_IMPOSSIBLE: device moved to DSDS; switch DDS to Sub2
       //   so internet data follows the only active radio (held by Sub2 for the voice call).
       // For TEMPORARY/REVOKE: the triggering condition cleared; switch back to the original DDS.
       void onDdsSwitchRecommendation(
           const telux::data::DdsSwitchRecommendation ddsSwitchRecommendation) override {
           const auto &info    = ddsSwitchRecommendation.recommendedDdsInfo;
           const auto &details = ddsSwitchRecommendation.recommendationDetails;

           if (info.type == telux::data::DdsType::TEMPORARY) {
               if (details.tempType == telux::data::TemporaryRecommendationType::HIGH) {
                   // Perform temporary DDS switch to the recommended slot.
                   performDdsSwitch(info);
               } else if (details.tempType == telux::data::TemporaryRecommendationType::REVOKE) {
                   // Voice call ended — switch back to original DDS permanently.
                   performDdsSwitch(info);
               }
           } else {
               // Permanent recommendation.
               performDdsSwitch(info);
           }
       }

       // Called when a DDS switch is confirmed by the modem.
       void onDdsChange(telux::data::DdsInfo currentState) override {
           std::cout << "onDdsChange: slotId=" << static_cast<int>(currentState.slotId) << std::endl;
       }

       // Called when the DualDataManager service status changes (e.g., modem SSR).
       // Re-register and re-configure DDS recommendations on SERVICE_AVAILABLE.
       void onServiceStatusChange(telux::common::ServiceStatus status) override {
           std::cout << "onServiceStatusChange: " << static_cast<int>(status) << std::endl;
       }

   private:
       void performDdsSwitch(const telux::data::DdsInfo &ddsInfo) {
           if (!dualDataMgr_) return;
           dualDataMgr_->requestDdsSwitch(
               ddsInfo, [ddsInfo](telux::common::ErrorCode errorCode) {
                   if (errorCode == telux::common::ErrorCode::SUCCESS) {
                       if (ddsInfo.type == telux::data::DdsType::TEMPORARY) {
                           // OEM must: TCP cleanup, release internet PDN/PDU on original DDS,
                           // redirect application traffic to temp DDS (nDDS slot).
                           std::cout << "Temporary DDS switch to slot "
                                     << static_cast<int>(ddsInfo.slotId)
                                     << " SUCCESS." << std::endl;
                       } else {
                           // OEM must: TCP cleanup, release internet PDN/PDU on temp DDS (nDDS),
                           // redirect application traffic back to the original DDS.
                           std::cout << "Permanent DDS switch to slot "
                                     << static_cast<int>(ddsInfo.slotId)
                                     << " SUCCESS." << std::endl;
                       }
                   } else {
                       std::cout << "requestDdsSwitch FAILED errorCode="
                                 << static_cast<int>(errorCode) << std::endl;
                   }
               });
       }

       std::shared_ptr<telux::data::IDualDataManager> dualDataMgr_;
   };

   auto listener = std::make_shared<MyDualDataListener>(dualDataMgr);
   auto status = dualDataMgr->registerListener(listener);
   if (status != telux::common::Status::SUCCESS) {
       std::cout << "registerListener failed" << std::endl;
       return -3;
   }

4. Configure DDS switch recommendations

This call is required before ``onDdsSwitchRecommendation`` indications are delivered.
The configuration is **not persistent** across reboots or modem SSR; reapply it in
``onServiceStatusChange`` whenever the service becomes available.

.. code-block:: cpp

   telux::data::DdsSwitchRecommendationConfig config;
   config.enableTemporaryRecommendations = true;
   config.enablePermanentRecommendations = true;
   config.recommBasis = telux::data::DDSRecommendationBasis::THROUGHPUT;

   auto errCode = dualDataMgr->configureDdsSwitchRecommendation(config);
   if (errCode != telux::common::ErrorCode::SUCCESS) {
       std::cout << "configureDdsSwitchRecommendation failed" << std::endl;
       return -4;
   }

5. Voice call on nDDS begins — handle onDualDataUsageRecommendationChange

A voice call arrives on the nDDS slot (Sub2). The device transitions from DSDA sub-mode to
DSDS sub-mode (e.g., due to a RAT change or thermal mitigation). In DSDS mode there is only
one radio resource shared between both SIMs, so long-running data on both slots simultaneously
is no longer possible. The modem fires ``onDualDataUsageRecommendationChange`` with
``NOT_ALLOWED``, signalling that long-running data can no longer run on both slots simultaneously.

6. Handle onDdsSwitchRecommendation — temporary DDS switch

The modem fires ``onDdsSwitchRecommendation`` recommending a temporary DDS switch from Sub1
to Sub2. Because the device is now in DSDS mode, only one SIM can actively use the radio at a
time. Sub2 is holding the radio for the voice call, so internet data must also move to Sub2 —
keeping data on Sub1 would mean Sub1 cannot reach the radio while Sub2 holds it for the call:

.. code-block:: text

    recommendedDdsInfo.slotId  = SLOT_ID_2   (switch DDS to Sub2 — the active radio slot)
    recommendedDdsInfo.type    = TEMPORARY
    recommendationDetails.tempType  = HIGH
    recommendationDetails.tempCause = TEMP_CAUSE_CODE_DSDA_IMPOSSIBLE

**OEM action**: Call ``requestDdsSwitch`` with ``slotId=SLOT_ID_2``, ``type=TEMPORARY``.
On SUCCESS: perform TCP cleanup, release internet PDN/PDU connections on original DDS (Sub1),
and redirect application traffic to the temp DDS (Sub2). This is handled by the
``performDdsSwitch`` helper in the listener.

7. Voice call ends — handle the REVOKE recommendation

When the voice call on nDDS ends, the modem fires ``onDdsSwitchRecommendation`` with a REVOKE:

.. code-block:: text

    recommendedDdsInfo.slotId  = SLOT_ID_1
    recommendedDdsInfo.type    = TEMPORARY
    recommendationDetails.tempType  = REVOKE
    recommendationDetails.tempCause = TEMP_CAUSE_CODE_CALL_STATUS_CHANGED

**OEM action**: Call ``requestDdsSwitch`` with ``slotId=SLOT_ID_1``, ``type=PERMANENT`` to
restore the original DDS. On SUCCESS: perform TCP cleanup, release internet PDN/PDU connections
on temp DDS (Sub2), and redirect application traffic back to the original DDS (Sub1).

.. note::
   If the OEM did **not** act on the previous temporary recommendation, no action is needed on
   REVOKE — simply discard the indication.

8. Deregister the listener and clean up

.. code-block:: cpp

   dualDataMgr->deregisterListener(listener);
   listener    = nullptr;
   dualDataMgr = nullptr;
