.. #=============================================================================
   #
   #  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
   #  SPDX-License-Identifier: BSD-3-Clause-Clear
   #
   #=============================================================================

=======================
Simulation with TelSDK
=======================

TelSDK simulation enables development of applications using the SDK's APIs on any Linux based hardware. The simulation is intended to enable software development, even before the actual Telematics hardware is available. Users can run the simulation on a Linux desktop if they choose.

The Simulation framework provides libraries which are compile time equivalent to the actual concrete libraries. The simulation libraries provide the behavior of APIs similar to the behavior seen on an actual Telematics device with the concrete libraries.

This allows applications to be developed, compiled, and executed using this framework. An application can link to the simulation libraries to use the functionality of the SDK APIs in this framework. The same application can then be recompiled for the target Telematics device without requiring any redevelopment or code changes.

The Simulation framework is designed to be configurable to tweak the behavior and the responses of the APIs. This provides flexibility to exercise the client code in multiple ways and scenarios.
Users can run the Simulation framework within a docker as well if desired.

.. _telsdk-ontarget-arch :
.. figure:: ../../images/telsdk_overview.png
  :width: 500

  TelSDK On-Target Architecture


.. _telsdk-simulation-arch :
.. figure:: ../../images/telsdk_simulation_overview.png
  :width: 500

  TelSDK Simulation Architecture


---------------------------------------
Components of the simulation framework
---------------------------------------

This section provides an overview of the components involved in the TelSDK simulation framework.


SDK Simulation Libraries
~~~~~~~~~~~~~~~~~~~~~~~~~

The telematics simulation library runs in the user space of the Linux system. It interacts with the simulation server and other subsystems to provide various services like phone calls, SMS, etc. These services are exposed by the SDK through the same fixed public APIs that are available on all Telematics platforms that support SDK.


Simulation Server
~~~~~~~~~~~~~~~~~

This is the main daemon that interacts with all the clients using the Simulation framework and provides the functionality for all the public APIs.


Event Injector
~~~~~~~~~~~~~~

Event injector allows the users to inject unsolicited events. It is a Linux based command-line utility that injects events into the simulation framework. For example: ``telsdk_event_injector -f tel_card -e cardInfoChanged <slotId> <cardPower>`` could be triggered to change the card power state.


Syntax for injecting events: ``telsdk_event_injector -f <filter/subsystem/manager> -e <event> <arguments>``

* ``-f``: specifies the filter.
* ``-e``: specifies the event that is triggered.

**To get help related to the events supported by TelSDK simulation**

* ``telsdk_event_injector -h``: provides the list of subsystems for which events are available.
* ``telsdk_event_injector -h <subsystem>``: provides the list of supported events in that particular subsystem.
* ``telsdk_event_injector -h <subsystem> <event>``: provides detailed information ,i.e., the syntax and arguments for that event.


--------------------------------
Getting started with simulation
--------------------------------

Build steps
~~~~~~~~~~~~

To build various components of the Simulation framework:

1. Fetch the source code

.. code-block::

  $ git clone https://git.codelinaro.org/clo/le/platform/vendor/qcom-opensource/snaptel-sdk.git -b telsdk.lnx.2.0.r11-rel telux
  $ cd telux/

2. Set up Build dependencies

.. code-block::

  #Install jsoncpp, cmake 3.15.3, gRPC.
  $ ./build_sim.sh setup <DESTINATION_FOLDER_ROOT_PATH>

3. Build simulation libraries, TelSDK sample apps, and test apps

.. code-block::

  $ ./build_sim.sh all <DESTINATION_FOLDER_ROOT_PATH>

**Note:** All the required TelSDK simulation include files, libs, and binaries get installed in <DESTINATION_FOLDER_ROOT_PATH>

4. Export path

.. code-block::

  $ source <DESTINATION_FOLDER_ROOT_PATH>/bin/setup_simulation.sh

The above script would setup different environment variables like ``PATH, LD_LIBRARY_PATH, PKG_CONFIG_PATH, CC, CXX``, etc.


5. To perform a build for user applications using cmake

.. code-block::

  $ cmake -DCMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES=<DESTINATION_FOLDER_ROOT_PATH>/include -DCMAKE_INSTALL_PREFIX=<DESTINATION_FOLDER_ROOT_PATH> <APPS_CMAKE_PATH> && make install


Run apps with docker
~~~~~~~~~~~~~~~~~~~~~

To run applications within the docker container:

1. Install docker

  .. code-block::

    $ sudo apt update
    $ sudo apt install apt-transport-https ca-certificates curl software-properties-common
    $ curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo apt-key add -
    $ sudo add-apt-repository "deb [arch=amd64] https://download.docker.com/linux/ubuntu bionic stable"
    $ sudo apt update
    $ sudo apt install docker-ce

  Once the installation is complete, we recommend that users do the following to add a docker affiliation group for the current user. This will make it easier to develop and avoid typing the "sudo" prefix every time.

  .. code-block::

    $ sudo usermod -aG docker ${USER}

2. Build docker image

  .. code-block::

    $ cd telux/
    $ ./build_sim.sh docker-image <DESTINATION_FOLDER_ROOT_PATH>

3. Run docker container

  .. code-block::

    $ docker run -ti --rm -h telsdk_simulation -v telsdk_volume:/data/telux telsdk-sim-image

4. User application can be started or if users wish to run one of the SDK's sample app or test app within the docker, then

  .. code-block::

    $ <APP_NAME>

  **Note:**

  1. The docker image built will have user application binaries installed to the docker container ``/usr/bin/`` path.
  2. When the docker container is started, it automatically starts the simulation server in the background.
  3. The path ``/data/telux/`` holds all TelSDK simulation related data.
  4. We are making use of the docker volume, to make this folder ``/data/telux`` persistent across multiple boots of the docker.
  5. To reset the persistent data, delete the docker volume.


Run apps without docker
~~~~~~~~~~~~~~~~~~~~~~~~

To run applications without the docker container:

1. Export path, if not already done

  .. code-block::

    $ source <DESTINATION_FOLDER_ROOT_PATH>/bin/setup_simulation.sh

2. Start simulation server in the background

  .. code-block::

    $ telsdk_simulation_server &

3. User application can be started or if users wish to run one of the SDK's sample app or test app, then

  .. code-block::

    $ <APP_NAME>

  **Note:**

  1. In the host machine, application binaries are installed to the ``<DESTINATION_FOLDER_ROOT_PATH>/bin/``
  2. The path ``<DESTINATION_FOLDER_ROOT_PATH>/data/telux/`` holds all TelSDK simulation related data.
  3. The path ``<DESTINATION_FOLDER_ROOT_PATH>/etc/telux/tel.conf`` holds TelSDK simulation configuration data.


---------------------------------------
Configuring behavior of the simulation
---------------------------------------

Managing subsystem readiness or service availability
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TelSDK initializes various subsystems during startup. It marks each subsystem as ready once the initialization procedures are completed for that subsystem. The application must wait until the corresponding subsystem on which it needs to make API requests is ready. TelSDK provides APIs to check whether a subsystem is ready or not.

The Simulation framework simulates subsystem readiness. The behavior and timing of this can be configured via the API specific JSON file. These user configurable JSON files are located under the path ``/data/telux/json/api/`` and the name of the JSON file corresponds to the subsystem name specified in the public headers.

For example: IDataConnectionManager.json present in ``/data/telux/json/api/``

+------------------+---------------------------------------+
|  JSON attribute  |  Description                          |
+==================+=======================================+
|  IsSubsystem     |  Specifies the subsystem readiness.   |
|  Ready           |                                       |
+------------------+---------------------------------------+
|  IsSubsystem     |  Specifies time the simulation        |
|  ReadyDelay      |  framework should take to indicate    |
|                  |  subsystem readiness)                 |
|                  |                                       |
+------------------+---------------------------------------+


Response Management
~~~~~~~~~~~~~~~~~~~~

TelSDK simulation framework allows simulating each API response via updating the API specific JSON file.
The table shows the JSON attributes to modify the behavior of the API in simulation.

+------------------+------------------------------------------------+
|  JSON attribute  |  Description                                   |
+==================+================================================+
|  Default         |  For all the public APIs this delay would be   |
|  CallbackDelay   |  the default delay and would only be           |
|                  |  considered if callbackDelay is not specified. |
|                  |                                                |
+------------------+------------------------------------------------+
|  status          |  Specifies the immediate synchronous           |
|                  |  response that application would receive when  |
|                  |  the TelSDK API is invoked. It is usually the  |
|                  |  return status of the API invoked.             |
|                  |                                                |
+------------------+------------------------------------------------+
|  callbackDelay   |  Specifies the time that simulation libraries  |
|                  |  shall take before invoking the callback       |
|                  |  function and -1 could be configured to avoid  |
|                  |  callback invocation. This would be given      |
|                  |  preference over DefaultCallbackDelay          |
|                  |                                                |
+------------------+------------------------------------------------+
|  error           |  Specifies ErrorCode that will be sent along   |
|                  |  with the user provided callback function.     |
|                  |                                                |
+------------------+------------------------------------------------+

The JSON for API behavior could be updated dynamically by making use of the json_update script.

For example: Considering the below API entry in JSON

.. code-block::

  "getDefaultProfile": {
  "callbackDelay": 400,
  "error": "SUCCESS",
  "status": "SUCCESS"
  },

.. code-block::

 $ json_update /api/data/IDataConnectionManager.json IDataConnectionManager getDefaultProfile callbackDelay 300

Above would update callbackDelay to 300.

**Note:** There will be cases where additional fields in response management would be needed. For example: SMS management.


Sending external events
~~~~~~~~~~~~~~~~~~~~~~~~

TelSDK simulation framework lets users simulate certain system level behaviors by injecting unsolicited event using event injector.
Use the event injector helper option to get detailed information about the events supported by each subsystem.

Multi-SIM configuration
~~~~~~~~~~~~~~~~~~~~~~~~

MULTISIM_CONFIG in ``etc/telux/tel.conf`` specifies type of multi-SIM configuration supported. By default, multi-SIM configuration is not enabled.

RPC port configuration
~~~~~~~~~~~~~~~~~~~~~~~~

Default configuration uses 8089 for communication between simulation server & its clients. If needed, ``RPC_PORT`` can be updated in tel.conf.


---------------------
Data and state usage
---------------------

The data stored and used by the simulation can be classified into three categories as mentioned below:

* **API response data:** Specifies API responses as described in the section *Response Management* above. For example: synchronous response or asynchronous response. JSON files under the path ``/data/telux/json/api/`` hold the API responses. Users can modify this file to change the value returned in the response.
* **Static system data:** Specifies static persistent data. For example: IMEI number, thermal zone information, etc. JSON files under the path ``/data/telux/json/system-info/`` hold the system static data. It is used internally by our TelSDK simulation libraries to maintain system states and shall not be altered externally.
* **Dynamic system data:** Specifies dynamic persistent data. For example: RAT preference, VLAN configuration, Backhaul configuration, etc. JSON files under the path ``/data/telux/json/system-state/`` hold the system state. It is used internally by our TelSDK simulation libraries to maintain system states and shall not be altered externally.


----------------------
Logging in simulation
----------------------

TelSDK simulation framework supports logging. User configurable logger settings available in ``/etc/telux/tel.conf`` are listed below.

+------------------+-----------------------------------------------------------------+
|  LOGGER_LEVEL    |  Supported log levels are:                                      |
|                  |                                                                 |
|                  |  NONE -- No logging.                                            |
|                  |                                                                 |
|                  |  PERF -- Prints messages with nanoseconds precision timestamp.  |
|                  |                                                                 |
|                  |  ERROR -- Very minimal logging. Prints error messages only.     |
|                  |                                                                 |
|                  |  WARNING -- Prints perf, error and warning messages.            |
|                  |                                                                 |
|                  |  INFO -- Prints errors, warning, and information messages.      |
|                  |                                                                 |
|                  |  DEBUG -- Full logging including debug messages.                |
|                  |  It is intended for debugging purposes only.                    |
|                  |                                                                 |
+------------------+-----------------------------------------------------------------+
|  LOGGER_TYPE     |  Supported log types are:                                       |
|                  |                                                                 |
|                  |  CONSOLE_LOG -- enables console logging                         |
|                  |                                                                 |
|                  |  FILE_LOG -- enables file-based logging. Once file logging is   |
|                  |  enabled,LOG_FILE_NAME and LOG_FILE_PATH could further be       |
|                  |  configured by users                                            |
|                  |                                                                 |
|                  |  SYSLOG_LOG -- enables syslog logging.                          |
|                  |                                                                 |
+------------------+-----------------------------------------------------------------+

---------------------------
Environment specifications
---------------------------
Current release has been tested with the combination below.

* Ubuntu 18.04
* gcc version 7.5.0
* ldd version 2.27

----------------------------------------
Simulation under different technologies
----------------------------------------

The details on how simulation of individual areas can be used and controlled are available below.

1. :ref:`sim-reference-telephony`
2. :ref:`sim-reference-location`
3. :ref:`sim-reference-audio`
4. :ref:`sim-reference-data`
5. :ref:`sim-reference-thermal`

.. _sim-reference-telephony:


Telephony Simulation
~~~~~~~~~~~~~~~~~~~~~


Overview of Telephony Simulation
"""""""""""""""""""""""""""""""""

This page and the sub-pages provide information about usage of simulation for the telephony
sub-system that are part of the telux::tel namespace of the Telematics SDK.

.. _fig-tel-sim-overview:
.. figure:: ../../images/simulation_telephony_overview.png
  :width: 500

  Telephony Simulation Framework


APIs supported by the Simulation framework
"""""""""""""""""""""""""""""""""""""""""""

| -SMS: ISmsManager,ISmsListener, ISmscAddressCallback, SmsMessage
| -Card: ICardApp, ICardFileHandler, ICardManager, ICardListener, ICard
| -Subscription: ISubscriptionManager, ISubscriptionListener, ISubscription
| -Phone: IPhoneManager, IPhone
| -Call: ICallManager, ICallListener, ICall
| -ServingSystem: IServingSystemManager, IServingSystemListener

Telephony APIs Response handling
"""""""""""""""""""""""""""""""""

SMS
''''

The framework allows responses to each API to be configured using a JSON file.
Each manager has its own JSON configuration file present under ``simulation/json/api/tel/``

Example for API command response for telux::tel::ISmsManager::SendSms

.. code-block::

  "SendSms" :
      [
          {   "numberOfSegments" : 2,
              "status" : "SUCCESS"
          },
          {
              "smsResponseCbErrorCode" : "SUCCESS",
              "smsResponseCbDelay" : 9000,
              "smsResponseCbMsgRefs" : "1 2"
          },
          {
              "onDeliveryReportErrorCode" : "SUCCESS",
              "onDeliveryReportMsgRef" : 1,
              "onDeliveryReportCallbackDelay" : 12000
          },
          {
              "onDeliveryReportErrorCode" : "SUCCESS",
              "onDeliveryReportMsgRef" : 2,
              "onDeliveryReportCallbackDelay" : 12000
          }
      ]

Here, clients are expected to provide the inputs for JSON attributes of API according to an encoded message.

The table explains the configuration for API response parameters of *simulation/json/api/tel/ISmsManagerSlot1.json and simulation/json/api/tel/ISmsManagerSlot2.json*

+----------------------------------+------------------------------------------------------------+
|  JSON attribute                  |  Description                                               |
+==================================+============================================================+
|  numberOfSegments                |  Signifies number of segments present in encoded message.  |
|                                  |                                                            |
+----------------------------------+------------------------------------------------------------+
|  status                          |  Status of request.                                        |
|                                  |                                                            |
+----------------------------------+------------------------------------------------------------+
|  sentCallbackErrorCode,          |  Response callback errorcode.                              |
|  smsResponseCbErrorCode          |                                                            |
+----------------------------------+------------------------------------------------------------+
|  sentCallbackDelay,              |  Response callback delay.                                  |
|  smsResponseCbDelay              |                                                            |
+----------------------------------+------------------------------------------------------------+
|  sentCallbackMsgRefs,            |  Add reference number for every message segment according  |
|  smsResponseCbMsgRefs            |  to numberOfSegments with one space between the digits.    |
+----------------------------------+------------------------------------------------------------+
|  deliveryCallbackErrorCode       |  Delivery report for individual segment is invoked using   |
|                                  |  delivery callback error                                   |
+----------------------------------+------------------------------------------------------------+
|  deliveryCallbackDelay,          |  Delivery report for individual segment is invoked using   |
|  onDeliveryReportCallbackDelay   |  delivery callback delay.                                  |
+----------------------------------+------------------------------------------------------------+
|  onDeliveryReportErrorCode       |  Delivery callback errorcode.                              |
|                                  |                                                            |
+----------------------------------+------------------------------------------------------------+
|  onDeliveryReportMsgRef          |  Delivery report for message reference of a segment.       |
|                                  |                                                            |
+----------------------------------+------------------------------------------------------------+
|  receiverAddress                 |  Input receiver address according to encoded pdu.          |
|                                  |                                                            |
+----------------------------------+------------------------------------------------------------+


Call
'''''

Currently the simulation framework supports outgoing, incoming voice calls and regulatory ECalls.It provides the flexibility to configure ECall HLAP timer failures maintained by NAD device.
Support for conference calls will be added in upcoming releases.

Details of parameters that can be configured in the Simulation framework.

- "configureFailureForRegulatoryECall" : To configure ECall HLAP timer expiry.

  Supported Values: SUCCESS, T5FAILED, T6FAILED , T7FAILED.


 -SUCCESS: Successful behavior during an eCall.


 -T5FAILED: This configuration would lead to expiry of T5 timer. The simulation framework depicts that SEND-MSD signal is not decoded successfully by PSAP so it shall terminate eCall specific behaviour (i.e. it shall not proceed with the sending of MSD data).

  When it is configured, T5 timer's timeout is set to 5 sec as per EN 16062:2015.


 -T6FAILED: This configuration would lead to expiry of T6 timer. The simulation framework depicts that AL-ACK message is not recieved by IVS-NAD so it shall mark the transfer of the MSD as unsuccessful and terminate eCall specific behaviour.

   When it is configured, T6 timer's timeout is set to 5 sec as per EN 16062:2015.


 -T7FAILED: This configuration would lead to expiry of T7 timer. The simulation framework depicts that LL-ACK message is not recieved by IVS-NAD so it shall mark the transfer of the MSD as unsuccessful and terminate eCall specific behaviour.

   Timer can be configured using telux::tel::ICallManager::getECallConfig(EcallConfig::t7Timer).


- "eCallType" : To configure service domain for regulatory ECall.

  Supported Values: NGeCall , CSeCall.

These parameters can be configured via *simulation/json/api/tel/ICallManagerSlot1.json and simulation/json/api/tel/ICallManagerSlot2.json*.

Retrieve eCall MSD payload or encoded optional additional data content
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
To simulate retrieval of eCall MSD payload or optional additional data content, msdsettings.txt is copied to the application(ecall_app) launching path in "out/bin/". To modify MSD information, goto "out/bin/" and edit msdsettings.txt file.

Telephony data handling
""""""""""""""""""""""""

Card
'''''

Details of parameters currently configured in the Simulation framework to perform pin operations.

* "Pin1password" : "1234"  -- Password for PIN1 operations for public APIs changeCardPassword, unlockCardByPin, changeCardPassword.
* "Pin2password" : "5678" -- Password for PIN2 operations for public API changeCardPassword, unlockCardByPin, changeCardPassword.
* "Puk1password" : 11111111 -- Password for puk1 for public API unlockCardByPuk.
* "Puk2password" : 22222222 -- Password for puk2 for public API unlockCardByPuk.

file: ``simulation/json/system-state/tel/ICardManagerStateSlot1.json and simulation/json/system-state/tel/ICardManagerStateSlot2.json``


**Details of supported apps, EF files, and records**

All the EF files are considered to be present at same file structure directory of simulated SIM files.

User input for filepaths provided using APIs present under telux::tel::ICardFileHandler is not validated in the simulation library.

.. table:: ADF files

  +------------------------------------+-------------+------------------------------+--------------------------------------+
  | AppId                              | AppType     | Transparent EFs              | Linear Fixed EFs                     |
  +====================================+=============+==============================+======================================+
  |  a0000000871004f310ffff89080000ff  |  ISIM       |  File Id 1234, 5648          |  File Id 1234 , No of records 3      |
  |                                    |             |                              |                                      |
  +------------------------------------+-------------+------------------------------+--------------------------------------+
  |  a000000063504b43532d3135          |  Unknown    |  No files present            |  File Id 1234 , No of records 3      |
  |                                    |             |                              |                                      |
  +------------------------------------+-------------+------------------------------+--------------------------------------+
  |  a0000000871002fffff00189000001ff  |  USIM       |  File Id 1234, 5648, 7777    |  | File Id 1234 , No of records 3    |
  |                                    |             |                              |  | File Id 5678 , No of records 1    |
  |                                    |             |                              |  | File Id 7676 , No of records 4    |
  +------------------------------------+-------------+------------------------------+--------------------------------------+


.. table:: DF files

  +------------------------------+------------------------------------+
  | Transparent EFs              | Linear Fixed EFs                   |
  +==============================+====================================+
  |  File Id 3456 , File Id 5666 | | File Id 1234 , No of records 3   |
  |                              | | File Id 5678 , No of records 1   |
  +------------------------------+------------------------------------+


**Note:** Data stored in every record of Linear fixed EF file should be of same length.

Phone
''''''

Currently, the simulation framework supports the APIs for ``telux::tel::IPhoneManager`` and ``telux::tel::IPhone`` using the canned response configured in the JSON file: ``simulation/json/api/tel/IPhoneManagerSlot*.json``, and events are injected through the event-injector utility using the JSON file: ``simulation/json/Events.json``. When using events related to the phone, refer to ``Events.json`` and look for events under ``tel_phone``.

Performing operations such as setting the operating mode (``telux::tel::IPhoneManager::setOperatingMode`` API) on the target can have system-level impact, which could alter the response of other APIs within the same class or different classes, such as getting signal strength (``telux::tel::IPhone::requestSignalStrength`` API), etc. Currently, this behavior is not implemented in entirety, but it will be in future releases.

Note: Some APIs, such as ``telux::tel::PhoneManager::requestCellularCapabilityInfo`` and ``telux::tel::PhoneManager::requestOperatingMode``, provide device-specific information and are not specific to a particular subscription/slot. For these APIs, the JSON configuration is defined only in ``simulation/json/api/tel/IPhoneManagerStateSlot1.json`` pertaining to the first slot.

Serving system
'''''''''''''''

Currently, the simulation framework supports the APIs for ``telux::tel::IServingSystemManager`` using the canned response configured in the JSON file: ``simulation/json/api/tel/IServingSystemSlot*.json``, and events are injected through the event-injector utility using the JSON file: ``simulation/json/Events.json``. When using events related to the serving system, refer to ``Events.json`` and look for events under ``tel_serv``.

Performing operations such as setting the RAT mode preference or service domain preference (``telux::tel::IServingSystemManager::setRatPreference`` API or ``telux::tel::IServingSystemManager::setServiceDomainPreference`` API) on the target can have system-level impact, which could alter the response of other APIs within the same class or different classes, such as getting signal strength notifications or change of current serving RAT etc. Currently, this behavior is not implemented, but it will be in future releases.

Supplementary Services
''''''''''''''''''''''

Details of parameters that can be configured in the simulation framework.
- "failureCause" : To configure failureCause for supplementary services.
  Refer ``telux::tel::FailureCause`` for values.

To simulate, No service as a failure cause.
**Sample input:**

.. code-block::

 telsdk_event_injector -f json_update -e modify /api/tel/ISuppServicesManagerSlot1.json ISuppServicesManager.failureCause 0x15

- "suppSvcProvisionStatus" : To configure supplementary services provision status.
  Refer ``telux::tel::SuppSvcProvisionStatus`` for valid values of supplementary services provision status.

**Sample input:**

.. code-block::

 telsdk_event_injector -f json_update -e modify /api/tel/ISuppServicesManagerSlot1.json ISuppServicesManager.requestOirPref.suppSvcProvisionStatus 0

Telephony event handling
"""""""""""""""""""""""""

Incoming SMS
'''''''''''''

To simulate an incoming SMS, event injector is used by providing the inputs of encoded and decoded attributes of a message.

The client is expected to construct all the members of ``telux::tel::SmsMessage`` and send the message segment using event injector.

Simulation framework maintains the database to store the message attributes of an incoming SMS.

**Sample input:**

Segment-1 of multipart message:

.. code-block::

  telsdk_event_injector -f tel_sms -e incomingsms 1 2 1 1 GSM7 1 07918185690700F0240B918189486537F300003280126063128A0F54741934AFBB41F2F4BC3C07A501 +18988456733 The sun rises i

Segment-2 of multipart message:

.. code-block::

  telsdk_event_injector -f tel_sms -e incomingsms 1 2 1 2 GSM7 1 07918185690700F0240B918189486537F300003280126014208A0A6E101D5D0695C3733A +18988456733 n the east

* Incoming SMS will be stored on the simulation server only when the preferred storage type is set to SIM.
* It can be changed using ``telux::tel::ISmsManager::setPreferredStorage``
* Memory full indication ( ``telux::tel::ISmsListener::onMemoryFull`` ) will be sent to all the clients when the messages count on
  the simulation server exceeds the defined count specified in ``simulation/json/api/tel/ISmsManagerSlot1.json and simulation/json/api/tel/ISmsManagerSlot2.json``

.. code-block::

  "requestStorageDetails" :
  {
      "requestStorageDetailsCb_maxCount" : 10
  }


Update subscription information
''''''''''''''''''''''''''''''''

The event injector allows you to inject event for subscription information change.

**Sample input:**

.. code-block::

  telsdk_event_injector -f tel_sub -e subscriptionInfoChanged 1 Carrier-1 8984653739 89010020000011293999 310 11 310018984653739 ffffffffffffffff ffffffffffffffff

Update cell information list
'''''''''''''''''''''''''''''

The event injector allows you to inject event for cell information change related to current cell and neighboring cells.

Sample: ``telsdk_event_injector -f tel_phone -e cellInfoListUpdate <slotId> ,<cellType1> <isRegistered> <p1> <p2> <p3> <....> ,<cellType2> <isRegistered> <p1> <p2> <p3> <....>``

``Note:`` Cell information is separated by commas.

- **slotId:** An integer representing valid slotIds, which are 1 and 2 only.
- **cellType:** An integer representing valid cellTypes. The supported types are:-

 - GSM - 1
 - LTE - 3
 - WCDMA - 4
 - NR5G - 6
 - (CDMA and TDSCDMA are not supported)

- **isRegistered:** An integer indicating whether the cell is registered or not. Valid values are 1 (registered) or 0 (not registered).

Refer below for configuring different cells with their respective attributes in the same pre-determined order:-

- **GSM cell info:**

 - <cellType> <isRegistered> <p1-MCC> <p2-MNC> <p3-LAC> <p4-CID> <p5-ARFCN> <p6-BSIC> <p7-Signal Strength> <p8-Bit Error Rate>

- **WCDMA cell info:**

 - <cellType> <isRegistered> <p1-MCC> <p2-MNC> <p3-LAC> <p4-CID> <p5-PSC> <p6-UARFCN> <p7-Signal Strength> <p8-Bit Error Rate>

- **LTE cell info:**

 - <cellType> <isRegistered> <p1-MCC> <p2-MNC> <p3-CI> <p4-PCI> <p5-TAC> <p6-EARFCN> <p7-Signal Strength> <p8-RSRP> <p9-RSRQ> <p10-RSSNR> <p11-CQI> <p12-Timing Advance>

- **NR5G cell info:**

 - <cellType> <isRegistered> <p1-MCC> <p2-MNC> <p3-CI> <p4-PCI> <p5-TAC> <p6-ARFCN> <p7-RSRP> <p8-RSRQ> <p9-RSSNR>

Please note that CDMA and TDSCDMA are deprecated.

**Sample input:**

.. code-block::

 telsdk_event_injector -f tel_phone -e "cellInfoListUpdate 1 ,1 1 310 00 70 81 10 1 28 5,4 0 311 00 70 81 10 1 30 3,3 0 312 00 10 11 13 14 23 -50 -5 200 13 11,6 0 313 00 10 20 30 40 -50 15 300"

Update signal strength information
'''''''''''''''''''''''''''''''''''

The event injector allows you to inject event for signal strength change with different RATs.

Sample: ``telsdk_event_injector -f tel_phone -e signalStrengthUpdate <slotId> ,<Rat1> <p1> <p2> <p3> <...> ,<Rat2> <p1> <p2> <p3> <...>``

``Note:`` Similarly to cell info updates, the signal strength event with different RATs is separated by commas.

- **RAT:** A string representing the RAT type. RAT is used for retrieving Signal Strength information. Valid values are GSM, LTE, WCDMA, and NR5G.

Refer below for configuring different RAT signal strength with their respective attributes in the same pre-determined order:-

- **GSM:** Use the following parameters:

 - p1 - Signal Strength
 - p2 - Bit Error Rate

- **LTE:** Use the following parameters:

 - p1 - Signal Strength
 - p2 - RSRP
 - p3 - RSRQ
 - p4 - RSSNR
 - p5 - CQI
 - p6 - Timing Advance

- **WCDMA:** Use the following parameters:

 - p1 - Signal Strength
 - p2 - Bit Error Rate

- **NR5G:** Use the following parameters:

 - p1 - RSRP
 - p2 - RSRQ
 - p3 - RSSNR

Please note that CDMA and TDSCDMA are deprecated.

**Sample input:**

.. code-block::

 telsdk_event_injector -f tel_phone -e "signalStrengthUpdate 1 ,GSM 28 3 ,WCDMA 29 6 ,LTE 23 -50 -5 200 13 11 ,NR5G -50 15 300"

Trigger an incoming call
'''''''''''''''''''''''''

To simulate ICallManager event - telux::tel::ICallListener::onIncomingCall

**Sample input:**

.. code-block::

 telsdk_event_injector -f tel_call -e incomingCall 1 999999999

Hangup a voice call or an eCall
''''''''''''''''''''''''''''''''

**Sample input:**

.. code-block::

 telsdk_event_injector -f tel_call -e hangupCall 1 1

Request to update Minimum Set of Data (MSD) for an eCall
'''''''''''''''''''''''''''''''''''''''''''''''''''''''''

To simulate ICallManager event - telux::tel::ICallListener::OnMsdUpdateRequest

**Sample input:**

.. code-block::

 telsdk_event_injector -f tel_call -e msdUpdateRequest 1

Trigger an modification request to change the call from a normal voice call to a real time text (RTT) call
'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''

To simulate ICallManager event - telux::tel::ICallListener::onModifyCallRequest

Command: ``telsdk_event_injector -f tel_call -e modifyCallRequest <slotId> <callId>``

**Parameters of event injector command:**

- slotId: valid slotIds are 1 & 2 only
- callId: valid call index on which upgrade request was triggered by remote party

**Sample input:**

.. code-block::

 telsdk_event_injector -f tel_call -e modifyCallRequest 1 1

Trigger an event to send RTT message from a remote party.
'''''''''''''''''''''''''''''''''''''''''''''''''''''''''

To simulate ICallManager event - telux::tel::ICallListener::onRttMessage

Command: ``telsdk_event_injector -f tel_call -e rttMessageRequest <slotId> <message>``

**Parameters of event injector command:**

- slotId: valid slotIds are 1 & 2 only
- message: valid string (text message) received from remote device.

**Sample input:**

.. code-block::

 telsdk_event_injector -f tel_call -e rttMessageRequest 1 GoodDay

Update RAT preference and service domain preference
''''''''''''''''''''''''''''''''''''''''''''''''''''

To simulate IServingSystemManager event - telux::tel::IServingSystemListener::onRatPreferenceChanged
and telux::tel::IServingSystemListener::onServiceDomainPreferenceChanged

Command: ``telsdk_event_injector -f tel_serv -e systemSelectionPreferenceUpdate <slotId> <serviceDomainPreference> <ratPreferences>``

**Parameters of event injector command:**

- slotId: valid slotIds are 1 & 2 only
- serviceDomainPreference: valid integer value is filled as per telux::tel::ServiceDomainPreference
- ratPreferences: valid integer value is filled as per telux::tel::RatPrefType

example - ratPreferences input is 012 for RAT preference PREF_CDMA_1X , PREF_CDMA_EVDO and PREF_GSM

**Sample input:**

.. code-block::

 telsdk_event_injector -f tel_serv -e systemSelectionPreferenceUpdate 1 0 012

Update current system information
''''''''''''''''''''''''''''''''''

To simulate IServingSystemManager event - telux::tel::IServingSystemListener::onSystemInfoChanged
and telux::tel::IServingSystemListener::onDcStatusChanged

Command: ``telsdk_event_injector -f tel_serv -e systemInfoUpdate <slotId> <currentServingRat> <currentServingDomain> <endcAvailability> <dcnrRestriction>``

**Parameters of event injector command:**

- slotId: valid slotIds are 1 & 2 only
- currentServingRat: valid integer value is filled as per telux::tel::RadioTechnology
- currentServingDomain: valid integer value is filled as per telux::tel::ServiceDomain
- endcAvailability: valid integer value is filled as per telux::tel::endcAvailability
- dcnrRestriction: valid integer value is filled as per telux::tel::dcnrRestriction

**Sample input:**

.. code-block::

 telsdk_event_injector -f tel_serv -e systemInfoUpdate 1 16 3 0 1

Update network time information
''''''''''''''''''''''''''''''''

To simulate IServingSystemManager event - telux::tel::IServingSystemListener::onNetworkTimeChanged

Command: ``telsdk_event_injector -f tel_serv -e networkTimeUpdate <slotId> <year> <month> <day> <hour> <minute> <second> <dayOfWeek> <timeZone> <dstAdj> <nitzTime>``

**Parameters of event injector command:**

- slotId:    valid slotIds are 1 & 2 only
- year:      valid integer value is filled as per telux::tel::NetworkTimeInfo
- month:     valid integer value is filled as per telux::tel::NetworkTimeInfo
- day:       valid integer value is filled as per telux::tel::NetworkTimeInfo
- hour:      valid integer value is filled as per telux::tel::NetworkTimeInfo
- minute:    valid integer value is filled as per telux::tel::NetworkTimeInfo
- second:    valid integer value is filled as per telux::tel::NetworkTimeInfo
- dayOfWeek: valid integer value is filled as per telux::tel::NetworkTimeInfo
- timeZone:  valid integer value is filled as per telux::tel::NetworkTimeInfo
- dstAdj:    valid integer value is filled as per telux::tel::NetworkTimeInfo
- nitzTime:  valid string value is filled as per telux::tel::NetworkTimeInfo

**Sample input:**

.. code-block::

 telsdk_event_injector -f tel_serv -e networkTimeUpdate 1 2004 08 01 06 11 23 8 3444 2024/01/12

Update RF band information
'''''''''''''''''''''''''''
To simulate IServingSystemManager event - telux::tel::IServingSystemListener::onRFBandInfoChanged

Command: ``telsdk_event_injector -f tel_serv -e rFBandInfoUpdate <slotId> <band> <channel> <bandWidth>``

**Parameters of event injector command:**

- slotId:    valid slotIds are 1 & 2 only
- band:      valid integer value is filled as per telux::tel::RFBand
- channel:   valid integer value is filled as per telux::tel::RFBandInfo
- bandWidth: valid integer value is filled as per telux::tel::RFBandWidth

**Sample input:**

.. code-block::

 telsdk_event_injector -f tel_serv -e rFBandInfoUpdate 1 86 88 23

Trigger network registration rejection
'''''''''''''''''''''''''''''''''''''''
To simulate IServingSystemManager event - telux::tel::IServingSystemListener::onNetworkRejection

Command: ``telsdk_event_injector -f tel_serv -e networkRejectionUpdate <slotId> <rejectSrvInfoRat> <rejectSrvInfoDomain> <rejectCause> <mcc> <mnc>``

**Parameters of event injector command:**

- slotId:    valid slotIds are 1 & 2 only<br>
- rejectSrvInfoRat :    valid integer value is filled as per telux::tel::ServingSystemInfo
- rejectSrvInfoDomain : valid integer value is filled as per telux::tel::ServingSystemInfo
- rejectCause :         valid integer value is filled as per telux::tel::NetworkRejectInfo
- mcc :                 valid string value is filled as per telux::tel::NetworkRejectInfo
- mnc :                 valid string value is filled as per telux::tel::NetworkRejectInfo

**Sample input:**

.. code-block::

 telsdk_event_injector -f tel_serv -e networkRejectionUpdate 1 14 2 1 810 10

Update network selection mode
'''''''''''''''''''''''''''''''''''''''
To simulate INetworkSelectionManager event - telux::tel::INetworkSelectionListener::onSelectionModeChanged

Command: ``telsdk_event_injector -f tel_network_select -e selectionModeUpdate <slotId> <selectionMode> <mcc> <mnc>``

**Parameters of event injector command:**

- slotId:           valid slotIds are 1 & 2 only
- selectionMode:    valid integer value is filled as per telux::tel::NetworkModeInfo
- mcc:              valid string value is filled as per telux::tel::NetworkModeInfo
- mnc:              valid string value is filled as per telux::tel::NetworkModeInfo

**Sample input:**

.. code-block::

 telsdk_event_injector -f tel_network_select -e selectionModeUpdate 1 1 460 00

Update network scan results
'''''''''''''''''''''''''''''''''''''''
To simulate INetworkSelectionManager event - telux::tel::INetworkSelectionListener::onNetworkScanResults

Command: ``telsdk_event_injector -f tel_network_select -e networkScanResultsUpdate <slotId> ,<scanStatus> ,<operatorName1> <mcc> <mnc> <rat> <inUseStatus> <roamingStatus> <forbiddenStatus> <preferredStatus> ,<operatorName2> <mcc> <mnc> <rat> <inUseStatus> <roamingStatus> <forbiddenStatus> <preferredStatus>``

**Parameters of event injector command:**

- slotId:           valid slotIds are 1 & 2 only
- scanStatus:       valid integer value is filled as per telux::tel::NetworkScanStatus
- operatorName:     valid string value is filled as per telux::tel::OperatorInfo
- mcc:              valid string value is filled as per telux::tel::OperatorInfo
- mnc:              valid string value is filled as per telux::tel::OperatorInfo
- rat:              valid integer value is filled as per telux::tel::RadioTechnology
- inUseStatus:      valid integer value is filled as per telux::tel::OperatorStatus
- roamingStatus:    valid integer value is filled as per telux::tel::OperatorStatus
- forbiddenStatus:  valid integer value is filled as per telux::tel::OperatorStatus
- preferredStatus:  valid integer value is filled as per telux::tel::OperatorStatus

**Sample input:**

.. code-block::

 telsdk_event_injector -f tel_network_select -e networkScanResultsUpdate 1 ,0 ,CMCC 460 00 14 1 1 1 1 ,CU 460 01 14 1 1 1 1

Update IMS registration status
'''''''''''''''''''''''''''''''''''''''
To simulate IImsServingSystemManager event - telux::tel::IImsServingSystemListener::onImsRegStatusChange

Command: ``telsdk_event_injector -f tel_ims_serv -e regStatusUpdate <slotId> <regStatus> <rat> <errorCode> <errorString>``

**Parameters of event injector command:**

- slotId:           valid slotIds are 1 & 2 only
- regStatus:        valid integer value is filled as per telux::tel::RegistrationStatus
- rat:              valid integer value is filled as per telux::tel::RadioTechnology
- errorCode:        valid integer value is filled as per telux::tel::ImsRegistrationInfo
- errorString:      valid string value is filled as per telux::tel::ImsRegistrationInfo

**Sample input:**

.. code-block::

 telsdk_event_injector -f tel_ims_serv -e regStatusUpdate 1 2 20 0

Update IMS service information
'''''''''''''''''''''''''''''''''''''''
To simulate IImsServingSystemManager event - telux::tel::IImsServingSystemListener::onImsServiceInfoChange

Command: ``telsdk_event_injector -f tel_ims_serv -e serviceInfoUpdate <slotId> <smsStatus> <voiceStatus>``

**Parameters of event injector command:**

- slotId:           valid slotIds are 1 & 2 only
- smsStatus:        valid integer value is filled as per telux::tel::CellularServiceStatus
- voiceStatus:      valid integer value is filled as per telux::tel::CellularServiceStatus

**Sample input:**

.. code-block::

 telsdk_event_injector -f tel_ims_serv -e serviceInfoUpdate 1 0 2

Update IMS PDP status information
'''''''''''''''''''''''''''''''''''''''
To simulate IImsServingSystemManager event - telux::tel::IImsServingSystemListener::onImsPdpStatusInfoChange

Command: ``telsdk_event_injector -f tel_ims_serv -e pdpStatusInfoUpdate <slotId> <isConnected> <pdpFailure> <dataCallEndReason> <apnName>``

**Parameters of event injector command:**

- slotId:             valid slotIds are 1 & 2 only
- isConnected:        valid bool value is filled as per telux::tel::ImsPdpStatusInfo
- pdpFailure:         valid integer value is filled as per telux::tel::PdpFailureCode
- dataCallEndReason:  valid integer value is filled as per telux::common::EndReasonType
- apnName:            valid string value is filled as per telux::tel::ImsPdpStatusInfo

**Sample input:**

.. code-block::

 telsdk_event_injector -f tel_ims_serv -e pdpStatusInfoUpdate 1 1 0 2 IMS

Additional notes
"""""""""""""""""

Currently, the simulation behavior for Telephony is as follows:-

1. Support for encoding of text messages provided by the client for ``telux::tel::ISmsManager::sendSms`` will be added in future releases.
2. Support for subsystem restart notifications provided using ``telux::common::IServiceStatusListener::onServiceStatusChange`` will be added in future releases.


.. _sim-reference-location:

Location Simulation
~~~~~~~~~~~~~~~~~~~~


This section has details about the simulation for APIs under the telux::loc namespace.


Overview of Location Simulation
""""""""""""""""""""""""""""""""

The location simulation framework provides the ability to record location data from an actual Telematics device.
Applications using ILocationManager APIs will get reports from this recorded data.

.. _fig-loc-sim-overview:
.. figure:: ../../images/simulation_location_overview.png
  :width: 500

  Location Simulation Framework


Configuring Location API responses
"""""""""""""""""""""""""""""""""""

The framework allows responses to each API to be configured via a JSON file.

Each location manager has its own JSON configuration file. Clients can configure these files for:

**Location Manager:** configured via ``simulation/json/api/loc/ILocationManager.json``

Example:

**API command response for startBasicReports**

.. code-block::

  "startBasicReports": {
      "callbackDelay": 400,
      "error": "SUCCESS",
      "status": "SUCCESS"
  },

**Location Configurator:** configured via ``simulation/json/api/loc/ILocationConfigurator.json``

Example:

**API command response for configureRobustLocation**

.. code-block::

  "configureRobustLocation": {
      "callbackDelay": 400,
      "error": "SUCCESS",
      "status": "SUCCESS"
  },

**Dgnss Manager:** configured via ``simulation/json/api/loc/IDgnssManager.json``

Example:

**API command response for createSource**

.. code-block::

  "createSource": {
      "callbackDelay": 400,
      "status": "SUCCESS"
  },


Support for location reports and notifications
"""""""""""""""""""""""""""""""""""""""""""""""

Events supported from LocationManager:

1. Basic PVT Location reports.
2. Detailed PVT Location of different engine types, NMEA, SV, Measurements, Jammer and Disaster Crisis reports.
3. Location system info change.
4. Location capabilities update.

Events supported from LocationConfigurator:

1. Xtra status update.
2. Gnss signal update.

Format of location reports
"""""""""""""""""""""""""""

The location simulation has the ability to provide data to clients that is
prerecorded/captured from the target device in the form of a CSV.

**Reports captured via CSV currently:**

Basic PVT location report.

Detailed PVT Location report for different engine types, NMEA, SV, Measurements, and Jammer reports.

**Data representation:**

1. Each row of the CSV represents a complete iteration of the report at time interval T.

**Data reporting:**

The location reports are reported at the same frequency with which they were recorded
in the target.
On reaching the CSV's EOF, clients can configure if they wish the CSV to be replayed
via the "sim.loc.location_report_replay" configuration in tel.conf.
If the replay configuration is not selected, the location reports are stopped.

**Client usage:**

1. The prerecorded/captured CSV file is present by default under - ``${ROOTFS}/data/telux/csv/PRE-RECORDED_LOCATION_DATA.csv``.

2. Clients can configure the retrieval of reports via tel.conf by modifying the following fields:

  **sim.loc.location_report_file_name**

  Clients are requested to refer to the documentation of these fields under tel.conf for further usage.


Recording utility for Location Reports
"""""""""""""""""""""""""""""""""""""""

The goal of the recording utility is to capture the location reports running on the target device
so that the collected data can be used for the off-target SDK simulation.

**Reports supported by the recording utility currently:**

Detailed PVT Location generated by FUSED Engine type, Detailed Engine Location report based on engine type specified, NMEA, SV, Measurements and Jammer reports.

**Data representation:**

Each row of the CSV represents a complete iteration of the report at time interval T.

The report is represented by a string containing the fields separated by a comma(,).

**The tool to capture data is provided under:**

* ``simulation/record_location.sh``
* ``simulation/record_location.bat``

**Client usage:**

1. Connect the target to the PC.

2. Ensure that adb is available and restart adb as root by running "adb root".

3. Run the record_location script and capture the data in a CSV file.


Additional Notes
"""""""""""""""""

Currently, the simulation behavior for Location is as follows:-

1. Dgnss report injection is not supported.
2. Inter-manager dependencies are not supported.


.. _sim-reference-audio:

Audio Simulation
~~~~~~~~~~~~~~~~~


Overview of Audio Simulation
"""""""""""""""""""""""""""""

This page and the subpages provide information about Audio subsystem simulation.
Audio simulation utilizes the Advanced Linux Sound Architecture (ALSA) library and Linux sound devices for its operations.
The ALSA framework provides comprehensive audio functionality for Linux which includes recording and playing of audio streams using
the host machine speakers and mic.

Audio simulation supports the following features.

+----------------------------------+------------------------------------------------------------+
|  Audio feature                   |  Description                                               |
+==================================+============================================================+
|  Voice Call                      |  Predefined canned responses supported.                    |
|                                  |                                                            |
+----------------------------------+------------------------------------------------------------+
|  Playback                        |  ALSA library responses supported.                         |
|                                  |                                                            |
+----------------------------------+------------------------------------------------------------+
|  Capture                         |  ALSA library responses supported.                         |
|                                  |                                                            |
+----------------------------------+------------------------------------------------------------+
|  Loopback                        |  ALSA library responses supported.                         |
|                                  |                                                            |
+----------------------------------+------------------------------------------------------------+
|  Tone                            |  ALSA library responses supported.                         |
|                                  |                                                            |
+----------------------------------+------------------------------------------------------------+
|  Endless Playback                |  ALSA library responses supported.                         |
|                                  |                                                            |
+----------------------------------+------------------------------------------------------------+
|  Incall playback                 |  Predefined canned responses supported.                    |
|                                  |                                                            |
+----------------------------------+------------------------------------------------------------+
|  Incall capture                  |  Predefined canned responses supported.                    |
|                                  |                                                            |
+----------------------------------+------------------------------------------------------------+
|  HPCM                            |  Predefined canned responses supported.                    |
|                                  |                                                            |
+----------------------------------+------------------------------------------------------------+
|  Transcoder                      |  Predefined canned responses supported.                    |
|                                  |                                                            |
+----------------------------------+------------------------------------------------------------+

.. _fig_audio_sim_overview:
.. figure:: ../../images/simulation_audio_overview.PNG
  :width: 500

  Audio Simulation Framework


Audio simulation system requirements
"""""""""""""""""""""""""""""""""""""

Linux audio packages
'''''''''''''''''''''

- The following linux audio packages are required:

.. code-block::

 alsa alsa-utils alsa-tools pulseaudio pulseaudio-utils libasound2-dev

- For example on Ubuntu 18.04, these packages can be installed using the below command:

.. code-block::

 apt-get -y alsa alsa-utils alsa-tools pulseaudio pulseaudio-utils libasound2-dev

- References:

a. `ALSA <https://alsa-project.org/main/index.php/Main_Page>`_
b. `PulseAudio <https://wiki.ubuntu.com/PulseAudio/>`_

SoundCard
''''''''''

A sound card (also known as an audio card) is an internal expansion card that provides input and output of audio signals to and from a computer under the control of computer programs.

Tests should be performed to make sure both ALSA and PulseAudio are properly recognizing host machine's audio hardware. Notable places to check:

- Check to determine if the soundcards are detected by the system:

.. code-block::

 cat /proc/asound/cards

- Check to determine if the soundcards are detected by ALSA:

.. code-block::

 aplay -l
 arecord -l

- Check to determine if the soundcards are detected by PulseAudio:

.. code-block::

 pactl list

- If no soundcards are listed, virtual devices can be emulated using the following steps on the host-machine and docker.

.. code-block::

 # Start the pulseaudio server
 pulseaudio -D --exit-idle-time=-1

 # Load the virtual sink and set it as default
 pacmd load-module module-virtual-sink sink_name=v1
 pacmd set-default-sink v1

 # set the monitor of v1 sink to be the default source
 pacmd set-default-source v1.monitor

ALSA devices
'''''''''''''

Audio simulation supports configurability for ALSA devices and amixer control devices. User configurable settings available in ``/etc/telux/tel.conf`` are listed below.


+----------------------------------+------------------------------------------------------------+
|                                  |                                                            |
|  PCM_DEVICE                      |  This is used to configure ALSA devices.                   |
|                                  |                                                            |
|                                  |  To list all the ALSA devices, use below commands:         |
|				   |                                                            |
|                                  |  1. aplay -l                                               |
|                                  |                                                            |
|                                  |  2. arecord -l                                             |
|                                  |                                                            |
|				   |  Example: PCM_DEVICE = h3:0                                |
+----------------------------------+------------------------------------------------------------+
|  SND_CARD_CTL_DEVICE             |  This is used to configure soundcard control-device.       |
|                                  |                                                            |
|                                  |  A ctl device ("control-device") on a soundcard is         |
|                                  |                                                            |
|                                  |  used to help the user control several aspects of          |
|                                  |                                                            |
|                                  |  the soundcard's behaviour. The main use is to control     |
|                                  |                                                            |
|                                  |  the mixer. To list all the soundcards, use below          |
|                                  |                                                            |
|                                  |  commands:                                                 |
|                                  |                                                            |
|                                  |  1. aplay -l                                               |
|                                  |                                                            |
|                                  |  2. arecord -l                                             |
|                                  |                                                            |
|                                  |  Example: SND_CARD_CTL_DEVICE = h3                         |
+----------------------------------+------------------------------------------------------------+


Supported interfaces
"""""""""""""""""""""

| IAudioBuffer, IAudioCaptureStream, IAudioDevice, IAudioLoopbackStream, IAudioManager,
| IAudioPlayStream, IAudioStream, IAudioToneGeneratorStream, IAudioVoiceStream, IStreamBuffer.


Audio API response handling
""""""""""""""""""""""""""""

An audio API response can be configured through a JSON file.

Audio manager has its own JSON configuration file present under ``simulation/json/api/audio/``.

Clients can configure the inputs for JSON attributes of API request statuses and response error codes.

If the client configures the API behavior to mimic an error scenario by setting the response error code
to anything other than "SUCCESS", the audio server's response will be modified to simulate the erroneous behavior.


Example 1:
'''''''''''

API command response for telux::audio::IAudioManager::getStreamTypes

.. code-block::

 "getStreamTypes": {
     "callbackDelay": 400,
     "status" : "SUCCESS",
     "error" : "SUCCESS"
 },

When the async response to a IAudioManager::getStreamTypes request is ready and the "error" field is set to SUCCESS,
the server sends the response to the client without making any modifications.

Example 2:
'''''''''''

API command response for telux::audio::IAudioManager::createStream

.. code-block::

 "createStream": {
    "callbackDelay": 400,
    "status": "SUCCESS",
    "error": "INTERNAL_ERR"
 },

When the async response to a IAudioManager::createStream request is ready and the "error" field is set to INTERNAL_ERR,
the errorCode in the server’s response is modified and overwritten with the JSON value from the error field.

Audio event handling and notifications
"""""""""""""""""""""""""""""""""""""""

DTMF Detection:
''''''''''''''''

DTMF tone on the voice call downlink path can be detected and reported to the application.
The application needs to register a listener using the IAudioVoiceStream::registerListener API.
The IVoiceListener::onDtmfToneDetection() method is invoked to notify of the detected DTMF tone.
In a simulated environment telsdk_event_injector is used to inject the DTMF tone in the downlink path.

Sample input:

.. code-block::

 telsdk_event_injector -f audio -e dtmf_tone 697 1209

Modem SSR:
'''''''''''

In this scenario the modem crashes and the audio lower-layer notifies the server that the modem has crashed. The server
needs to notify all clients about the modem crash and clean all underlying resources used.
To receive this event notification, the client needs to register a listener using the IAudioManager::registerListener API.
The IAudioListener::onServiceStatusChange() method  is invoked to notify of the new status.

In a simulated environment telsdk_event_injector is used to inject a modem SSR event.

Sample input:

.. code-block::

 telsdk_event_injector -f audio -e ssr SERVICE_AVAILABLE

Additional notes:
""""""""""""""""""

1. Device switching for all streams only supports canned responses.
2. For compressed playback, TelSDK API behavior is simulated using PCM playback internally since ALSA doesn't support AMR codec. The call flow and API behavior remain similar to on-target compressed playback.


.. _sim-reference-data:

Data Simulation
~~~~~~~~~~~~~~~~


Overview of Data Simulation
""""""""""""""""""""""""""""

This page and the sub-pages provide information about usage of simulation for the data
sub-system that are part of the telux::data namespace of the Telematics SDK.

.. _fig_data_sim_overview:
.. figure:: ../../images/simulation_data_overview.PNG
  :width: 500

  Data Simulation Framework


Managers Supported
"""""""""""""""""""

The following managers are currently available in the simulation:

1. DataConnectionManager
2. DataProfileManager
3. DataFilterManager
4. DataSettingsManager
5. ServingSystemManager
6. SocksManager
7. NatManager
8. L2tpManager
9. FirewallManager


Data APIs Response handling
""""""""""""""""""""""""""""

The framework allows responses of each API to be configured using a JSON file.

Each manager has its own JSON configuration file present under ``simulation/json/api/data/``, For example

**Data Connection Manager:** configured via simulation/json/api/data/IDataConnectionManager.json has API command response for telux::data::IDataConnectionManager::getDefaultProfile

.. code-block::

 "getDefaultProfile": {
    "callbackDelay": 400,
    "error": "SUCCESS",
    "status": "SUCCESS"
 },

The JSON file holds the default values and could be updated dynamically by the users of the simulation.


Network Interface Configuration
""""""""""""""""""""""""""""""""

To simulate data calls, the framework requires physical interfaces provided by the host or docker environment, to be configured. Each data call would be mapped to one physical interface.

To configure which interface should be used, in tel.conf update the following configuration:

sim.data.physical_interface_name = eth0, eth1

Here the first data call that is brought up will be mapped to eth0 and second will be mapped to eth 1. If the client attempts to bring up a third data call it will fail because no more physical interfaces are available.

If a host/docker is limited in the number of interfaces it has, one could use the following commands to add virtual interfaces.


If the host machine does not have multiple interfaces available:
'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''

Considering the host machine has only eth0 interface available, the client can add the virtual interfaces using the command ``ip addr add <IP_ADDRESS>/subnet dev eth0 label eth0:1``

Now user can update ``sim.data.physical_interface_name = eth0, eth0:1`` in ``/etc/telux/tel.conf``.

The first datacall would be started with interface eth0 and the second datacall would be started with interface eth0:1.

Any further attempt to start the datacall would result in failure to start datacall and telux::data::IDataCall object will return NET_NO_NET.


IPv6 support inside docker container
'''''''''''''''''''''''''''''''''''''

Docker does not have IPv6 support by default. Users of the simulation can follow the steps below to enable IPv6 inside docker container.

.. code-block::

 $ vi /etc/docker/daemon.json

 "ipv6": true,
 "fixed-cidr-v6": "<SUBNET>",
 "dns": [<dnsAddress1>,<dnsAddress2>,<dnsAddress3>,<dnsAddress4>],
 "experimental": true,
 "ip6tables": true,

 $ systemctl restart docker

Once above settings are done in daemon.json, we can enable IPv6 support with the following options, while starting the docker container.

``--cap-add=NET_ADMIN --sysctl net.ipv6.conf.all.disable_ipv6=0``


Additional notes
"""""""""""""""""

1. To simulate data calls, the framework requires physical interfaces provided by the host or docker environment, to be configured.
2. Listener APIs under IServingSystemListener are not yet supported in simulation.
3. SSR use-cases are not yet supported.


.. _sim-reference-thermal:

Thermal Simulation
~~~~~~~~~~~~~~~~~~~


Overview of Thermal Simulation
"""""""""""""""""""""""""""""""

This page and the sub-pages provide information about Thermal subsystem simulation.

.. _fig-therm-sim-overview:
.. figure:: ../../images/simulation_thermal_overview.png
  :width: 500

  Thermal Simulation Framework

Supported interfaces
"""""""""""""""""""""

IThermalManager, IThermalListener.

Thermal API response handling
""""""""""""""""""""""""""""""

A thermal API response can be configured through a JSON file.

Thermal manager has its own JSON configuration file present under ``simulation/json/api/therm/``.

If the client configures the API behavior to mimic an error scenario by setting the response error code
to anything other than "SUCCESS", the thermal server's response will be modified to simulate the erroneous behavior.

Example 1:
'''''''''''

API command response for telux::therm::IThermalManager::getThermalZones

.. code-block::

 "getThermalZones": {
        "error": "SUCCESS"
 },

When the "error" field is set to SUCCESS, the server sends the response to the client without making any modifications.

Example 2:
'''''''''''

API command response for telux::therm::IThermalManager::getCoolingDevices

.. code-block::

 "getCoolingDevices": {
        "error": "INTERNAL_ERR"
 },

When the the "error" field is set to INTERNAL_ERR, the server sends nullptr as the response to the client.

To configure simulation with the static configuration describing the thermal zone, trip points and cooling devices in the system, one should refer and update the ``simulation/json/system-info/therm/ThermalState.json``

Thermal event handling
"""""""""""""""""""""""

Thermal events
'''''''''''''''

The following thermal events can be simulated by changing the current temperature of any thermal zone.

1. telux::therm::IThermalListener::onCoolingDeviceLevelChange
2. telux::therm::IThermalListener::onTripEvent

Sample input:

.. code-block::

 telsdk_event_injector -f therm -e setTemp <tzone_id> <tzone_temp>
 telsdk_event_injector -f therm -e setTemp 24 126000

- Above, 24 indicates the tzone Id and 126000 indicates the new temperature to be set for a thermal zone.
- Based on the new temperature, the thermal subsystem will calculate the events for trip and cooling device level.
