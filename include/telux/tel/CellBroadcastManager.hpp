/*
 *  Copyright (c) 2020, The Linux Foundation. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of The Linux Foundation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file       CellBroadcastManager.hpp
 *
 * @brief      CellBroadcastManager class is primary interface to configure,
 *             activate and receive 3GPP ETWS/CMAS cell broadcast messages as
 *             per 3GPP TS 23.041 specification. Currently 3GPP2 cell broadcast
 *             message is not supported.
 *
 * @note    Eval: This is a new API and is being evaluated. It is subject to change
 *          and could break backwards compatibility.
 *
 */

#ifndef CELLBROADCASTMANAGER_HPP
#define CELLBROADCASTMANAGER_HPP

#include <vector>
#include <string>
#include <memory>
#include <future>
#include <telux/tel/CellBroadcastDefines.hpp>
#include <telux/common/CommonDefines.hpp>

namespace telux {
namespace tel {

class ICellBroadcastListener;

/**
 * This function is called with the response to requestMessageIdFilters API.
 *
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [in] filters         List of broadcast filters @ref CellBroadcastFilter
 * @param [in] error           Return code which indicates whether the operation
 *                             succeeded or not @ref ErrorCode
 */
using RequestFiltersResponseCallback
   = std::function<void(std::vector<CellBroadcastFilter> filters, telux::common::ErrorCode error)>;


/**
 * This function is called with the response to requestActivationStatus API.
 *
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [in] isActivated     If true then filters are activated else filters are deactivated
 * @param [in] error           Return code which indicates whether the operation
 *                             succeeded or not @ref ErrorCode
 */
using RequestActivationStatusResponseCallback
   = std::function<void(bool isActivated, telux::common::ErrorCode error)>;

/**
 * Contains information elements for a GSM/UMTS/E-UTRAN/NG-RAN ETWS warning notification.
 * Supported values for each element are defined in 3GPP TS 23.041.
 */
class EtwsInfo {
public:

   /**
    * EtwsInfo constructor.
    * @param [in] geographicalScope           @ref GeographicalScope
    * @param [in] msgId                       Unique message identifier
    * @param [in] serialNumber                Serial number for message
    * @param [in] languageCode                ISO-639-1 language code for message
    * @param [in] messageText                 Message text
    * @param [in] priority                    @ref MessagePriority
    * @param [in] warningType                 @ref EtwsWarningType
    * @param [in] emergencyUserAlert          If true message is emergency user alert
    *                                         otherwise not
    * @param [in] activatePopup               If true message message activate popup flag
    *                                         is set, otherwise popup flag is false.
    * @param [in] primary                     If true ETWS message is primary notification
    *                                         otherwise not
    * @param [in] warningSecurityInformation  Buffer containing security information about ETWS
    *                                         primary notification such as timestamp and digital
    *                                         signature
    */
   EtwsInfo(GeographicalScope geographicalScope, int msgId, int serialNumber,
      std::string languageCode, std::string messageText, MessagePriority priority,
      EtwsWarningType warningType, bool emergencyUserAlert, bool activatePopup,
      bool primary, std::vector<uint8_t> warningSecurityInformation);

   /**
    * Get the geographicalScope of cellbroadcast message.
    *
    * @returns @ref GeographicalScope.
    *
    * @note    Eval: This is a new API and is being evaluated. It is subject to change
    *          and could break backwards compatibility.
    */
   GeographicalScope getGeographicalScope() const;

   /**
    * Get cellbroadcast message identifier. The message identifier identifies the type of the
    * cell broadcast message defined in spec 3GPP TS 23.041 9.4.1.2.2
    *
    * @returns Message identifier.
    *
    * @note    Eval: This is a new API and is being evaluated. It is subject to change
    *          and could break backwards compatibility.
    */
   int getMessageId() const;

   /**
    * Get the serial number of broadcast (geographical scope + message code + update number
    * for GSM/UMTS).
    *
    * @returns int containing cellbroadcast serial number.
    *
    * @note    Eval: This is a new API and is being evaluated. It is subject to change
    *          and could break backwards compatibility.
    */
   int getSerialNumber() const;

   /**
    * Get the ISO-639-1 language code for cell broadcast message, or empty string if unspecified.
    * This is not applicable for ETWS primary notification.
    *
    * @return Language code
    *
    * @note    Eval: This is a new API and is being evaluated. It is subject to change
    *          and could break backwards compatibility.
    */
   std::string getLanguageCode() const;

   /**
    * Get the body of cell broadcast message, or empty string if no body available.
    * For ETWS primary notification based on message identifier pre cannned message will be
    * sent.
    *
    * @return body or empty string
    *
    * @note    Eval: This is a new API and is being evaluated. It is subject to change
    *          and could break backwards compatibility.
    */
   std::string getMessageBody() const;

   /**
    * Get the priority for the cell broadcast message.
    *
    * @return @ref MessagePriority.
    *
    * @note    Eval: This is a new API and is being evaluated. It is subject to change
    *          and could break backwards compatibility.
    */
   MessagePriority getPriority() const;

   /**
    * Get the cellbroadcast message code.
    *
    * @returns int containing cellbroadcast message code.
    *
    * @note    Eval: This is a new API and is being evaluated. It is subject to change
    *          and could break backwards compatibility.
    */
   int getMessageCode() const;

   /**
    * Get the cellbroadcast message update number.
    *
    * @returns int containing cellbroadcast message's update number.
    *
    * @note    Eval: This is a new API and is being evaluated. It is subject to change
    *          and could break backwards compatibility.
    */
   int getUpdateNumber() const;

   /**
    * Get ETWS warning type.
    *
    * @returns @ref EtwsWarningType.
    *
    * @note    Eval: This is a new API and is being evaluated. It is subject to change
    *          and could break backwards compatibility.
    */
   EtwsWarningType getEtwsWarningType();

   /**
    * Returns the ETWS emergency user alert flag.
    *
    * @return true to notify terminal to activate emergency user alert or false otherwise
    *
    * @note    Eval: This is a new API and is being evaluated. It is subject to change
    *          and could break backwards compatibility.
    */
   bool isEmergencyUserAlert();

   /**
    * Returns the ETWS activate popup flag.
    *
    * @return true to notify terminal to activate display popup or false otherwise
    *
    * @note    Eval: This is a new API and is being evaluated. It is subject to change
    *          and could break backwards compatibility.
    */
   bool isPopupAlert();

   /**
     * Returns the ETWS format flag. This flag determine whether ETWS message is primary
     * notification or not.
     *
     * @return true if the message is primary message, otherwise secondary message
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     */
   bool isPrimary();

   /**
     * Returns security information about ETWS primary notification such as
     * timestamp and digital signature(applicable only for GSM).
     *
     * @return byte buffer
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     */
   std::vector<uint8_t> getWarningSecurityInformation();

private:
   GeographicalScope scope_;
   int messageId_;
   int serialNum_;
   int messageCode_;
   int updateNumber_;
   std::string languageCode_;
   std::string body_;
   MessagePriority priority_;
   EtwsWarningType warningType_;
   bool emergencyUserAlert_;
   bool activatePopup_;
   bool isPrimary_;
   std::vector<uint8_t> warningInfo_;

};

/**
 * Contains information elements for a GSM/UMTS/E-UTRAN/NG-RAN CMAS warning notification.
 * Supported values for each element are defined in 3GPP TS 23.041.
 */
class CmasInfo {
public:

   /**
    * CmasInfo constructor.
    * @param [in] geographicalScope       @ref GeographicalScope
    * @param [in] msgId                   Unique message identifier
    * @param [in] serialNumber            Serial number for message
    * @param [in] languageCode            ISO-639-1 language code for message
    * @param [in] messageText             Message text
    * @param [in] priority                @ref MessagePriority
    * @param [in] messageClass            @ref CmasMessageClass
    * @param [in] severity                @ref CmasSeverity
    * @param [in] urgency                 @ref CmasUrgency
    * @param [in] certainty               @ref CmasCertainty
    */
   CmasInfo(GeographicalScope geographicalScope, int msgId, int serialNumber,
      std::string languageCode, std::string messageText, MessagePriority priority,
      CmasMessageClass messageClass, CmasSeverity severity, CmasUrgency urgency,
      CmasCertainty certainty);

   /**
    * Get the geographicalScope of cellbroadcast message.
    *
    * @returns @ref GeographicalScope.
    *
    * @note    Eval: This is a new API and is being evaluated. It is subject to change
    *          and could break backwards compatibility.
    */
   GeographicalScope getGeographicalScope() const;

   /**
    * Get cellbroadcast message identifier. The message identifier identifies the type of the
    * cell broadcast message defined in spec 3GPP TS 23.041 9.4.1.2.2
    *
    * @returns Message identifier.
    *
    * @note    Eval: This is a new API and is being evaluated. It is subject to change
    *          and could break backwards compatibility.
    */
   int getMessageId() const;

   /**
    * Get the serial number of broadcast (geographical scope + message code + update number
    * for GSM/UMTS).
    *
    * @returns int containing cellbroadcast serial number.
    *
    * @note    Eval: This is a new API and is being evaluated. It is subject to change
    *          and could break backwards compatibility.
    */
   int getSerialNumber() const;

   /**
    * Get the ISO-639-1 language code for cell broadcast message, or empty string if unspecified.
    * This is not applicable for ETWS primary notification.
    *
    * @return Language code
    *
    * @note    Eval: This is a new API and is being evaluated. It is subject to change
    *          and could break backwards compatibility.
    */
   std::string getLanguageCode() const;

   /**
    * Get the body of cell broadcast message, or empty string if no body available.
    * For ETWS primary notification based on message identifier pre cannned message will be
    * sent.
    *
    * @return body or empty string
    *
    * @note    Eval: This is a new API and is being evaluated. It is subject to change
    *          and could break backwards compatibility.
    */
   std::string getMessageBody() const;

   /**
    * Get the priority for the cell broadcast message.
    *
    * @return @ref MessagePriority.
    *
    * @note    Eval: This is a new API and is being evaluated. It is subject to change
    *          and could break backwards compatibility.
    */
   MessagePriority getPriority() const;

   /**
    * Get the cellbroadcast message code.
    *
    * @returns int containing cellbroadcast message code.
    *
    * @note    Eval: This is a new API and is being evaluated. It is subject to change
    *          and could break backwards compatibility.
    */
   int getMessageCode() const;

   /**
    * Get the cellbroadcast message update number.
    *
    * @returns int containing cellbroadcast message's update number.
    *
    * @note    Eval: This is a new API and is being evaluated. It is subject to change
    *          and could break backwards compatibility.
    */
   int getUpdateNumber() const;

   /**
    * Get CMAS message class.
    *
    * @returns @ref CmasMessageClass.
    *
    * @note    Eval: This is a new API and is being evaluated. It is subject to change
    *          and could break backwards compatibility.
    */
   CmasMessageClass getMessageClass();

   /**
    * Get CMAS message severity.
    *
    * @returns @ref CmasSeverity.
    *
    * @note    Eval: This is a new API and is being evaluated. It is subject to change
    *          and could break backwards compatibility.
    */
   CmasSeverity getSeverity();

   /**
    * Get CMAS message urgency.
    *
    * @returns @ref CmasUrgency.
    *
    * @note    Eval: This is a new API and is being evaluated. It is subject to change
    *          and could break backwards compatibility.
    */
   CmasUrgency getUrgency();

   /**
    * Get CMAS message certainty.
    *
    * @returns @ref CmasCertainty.
    *
    * @note    Eval: This is a new API and is being evaluated. It is subject to change
    *          and could break backwards compatibility.
    */
   CmasCertainty getCertainty();

private:
   GeographicalScope scope_;
   int messageId_;
   int serialNum_;
   int messageCode_;
   int updateNumber_;
   std::string languageCode_;
   std::string body_;
   MessagePriority priority_;
   CmasMessageClass messageClass_;
   CmasSeverity severity_;
   CmasUrgency urgency_;
   CmasCertainty certainity_;
};

/**
 * @brief Cell Broadcast message.
 */
class CellBroadcastMessage {
public:

   /**
    * CellBroadcastMessage constructor.
    * @param [in] type               @ref MessageType
    * @param [in] etwsWarningInfo    @ref EtwsInfo
    * @param [in] cmasWarningInfo    @ref CmasInfo
    */
   CellBroadcastMessage(MessageType type, std::shared_ptr<EtwsInfo> etwsWarningInfo,
      std::shared_ptr<CmasInfo> cmasWarningInfo);

   /**
    * Get the cellbroadcast message type.
    *
    * @returns @ref MessageType.
    *
    * @note    Eval: This is a new API and is being evaluated. It is subject to change
    *          and could break backwards compatibility.
    */
   MessageType getMessageType() const;

   /**
    * Get ETWS warning notification containing information about the ETWS warning type,
    * the emergency user alert flag and the popup flag. This method should be called only if
    * message type returned by getMessageType() API is MessageType::ETWS
    *
    * @return  pointer to EtwsInfo or null if this is not an ETWS warning notification
    *
    * @note    Eval: This is a new API and is being evaluated. It is subject to change
    *          and could break backwards compatibility.
    */
   std::shared_ptr<EtwsInfo> getEtwsWarningInfo() const;

   /**
    * Get CMAS warning notification containing information about the CMAS message class, severity,
    * urgency and certainty.This method should be called only if message type returned by
    * getMessageType() API is MessageType::CMAS
    *
    * @return pointer to CmasInfo or null if this is not a CMAS warning notification
    *
    * @note    Eval: This is a new API and is being evaluated. It is subject to change
    *          and could break backwards compatibility.
    */
   std::shared_ptr<CmasInfo> getCmasWarningInfo() const;

private:
   MessageType messageType_;
   std::shared_ptr<EtwsInfo> etwsInfo_;
   std::shared_ptr<CmasInfo> cmasInfo_;
};

/**
 * @brief CellBroadcastManager class is primary interface to configure and
 *        activate emergency broadcast messages and receive broadcast messages.
 */
class ICellBroadcastManager {
public:
   /**
    * Checks the status of network subsystem and returns the result.
    *
    * @returns True if network subsystem is ready for service otherwise false.
    *
    * @note    Eval: This is a new API and is being evaluated. It is subject to change
    *          and could break backwards compatibility.
    */
   virtual bool isSubsystemReady() = 0;

   /**
    * Wait for network subsystem to be ready.
    *
    * @returns  A future that caller can wait on to be notified when network
    *           subsystem is ready.
    *
    * @note    Eval: This is a new API and is being evaluated. It is subject to change
    *          and could break backwards compatibility.
    */
   virtual std::future<bool> onSubsystemReady() = 0;

   /**
    * Get associated slot for this CellBroadcastManager.
    *
    * @returns SlotId
    *
    * @note    Eval: This is a new API and is being evaluated. It is subject to change
    *          and could break backwards compatibility.
    */
   virtual SlotId getSlotId() = 0;

   /**
    * Configures the broadcast messages to be received.
    *
    * @param [in] filters        List of filtered broadcast message identifiers.
    * @param [in] callback       Optional callback to get the response
    *                            of set cell broadcast filters.
    *
    * @returns Status of updateMessageIdFilters i.e. success or suitable error code.
    *
    * @note    Eval: This is a new API and is being evaluated. It is subject to
    *          change and could break backwards compatibility.
    */
   virtual telux::common::Status updateMessageFilters(std::vector<CellBroadcastFilter> filters,
      telux::common::ResponseCallback callback = nullptr)
      = 0;

   /**
    * Retrieve configured message filters for which broadcast messages will be received.
    *
    * @param [in] callback       Callback to get the response of get cell broadcast
    *                            filters.
    *
    * @returns Status of requestMessageIdFilters i.e. success or suitable error code.
    *
    * @note    Eval: This is a new API and is being evaluated. It is subject to
    *          change and could break backwards compatibility.
    */
   virtual telux::common::Status requestMessageFilters(
      RequestFiltersResponseCallback callback) = 0;

   /**
    * Allows activation and deactivation of configured broadcast messages.
    *
    * @param [in] activate       Activate/deactivate broadcast messages.
    * @param [in] callback       Optional callback pointer to get the response.
    *
    * @returns Status of setActivationStatus i.e. success or suitable error code.
    *
    * @note    Eval: This is a new API and is being evaluated. It is subject to
    *          change and could break backwards compatibility.
    */
   virtual telux::common::Status setActivationStatus(bool activate,
      telux::common::ResponseCallback callback = nullptr) = 0;

   /**
    * Get activation status for configured broadcast messages.
    *
    * @param [in] callback       Callback pointer to get the response.
    *
    * @returns Status of requestActivationStatus i.e. success or suitable error code.
    *
    * @note    Eval: This is a new API and is being evaluated. It is subject to
    *          change and could break backwards compatibility.
    */
   virtual telux::common::Status requestActivationStatus(
      RequestActivationStatusResponseCallback callback) = 0;

   /**
    * Register a listener for cell broadcast messages.
    *
    * @param [in] listener    Pointer to ICellBroadcastListener object which receives broadcast
    * message.
    *
    * @returns Status of registerListener i.e. success or suitable error code.
    *
    * @note    Eval: This is a new API and is being evaluated. It is subject to change
    *          and could break backwards compatibility.
    */
   virtual telux::common::Status registerListener(std::weak_ptr<ICellBroadcastListener> listener)
      = 0;

   /**
    * De-register the listener.
    *
    * @param [in] listener    Listener to be de-registered
    *
    * @returns Status of deregisterListener i.e. success or suitable error code.
    *
    * @note    Eval: This is a new API and is being evaluated. It is subject to change
    *          and could break backwards compatibility.
    */
   virtual telux::common::Status deregisterListener(std::weak_ptr<ICellBroadcastListener> listener)
      = 0;

   virtual ~ICellBroadcastManager(){};
};

/**
 * @brief A listener class which monitors cell broadcast messages.
 *
 * The methods in listener can be invoked from multiple different threads. The
 * implementation should be thread safe.
 */
class ICellBroadcastListener : public common::IServiceStatusListener {
public:
   /**
    * This function is called when device receives an incoming cell broadcast message.
    *
    * @param [in] cbMessage - Broadcast message with information related to ETWS/CMAS notification.
    *
    * @note    Eval: This is a new API and is being evaluated. It is subject to change
    *          and could break backwards compatibility.
    */
   virtual void onIncomingMessage(const std::shared_ptr<CellBroadcastMessage> cbMessage) {
   }

   virtual ~ICellBroadcastListener() {
   }
};

}  // End of namespace tel
}  // End of namespace telux
#endif // CELLBROADCASTMANAGER_HPP
