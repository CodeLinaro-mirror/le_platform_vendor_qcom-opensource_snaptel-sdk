/*
 *  Copyright (c) 2017-2021 The Linux Foundation. All rights reserved.
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

/*
 *  Changes from Qualcomm Innovation Center are provided under the following license:
 *
 *  Copyright (c) 2021-2022 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted (subject to the limitations in the
 *  disclaimer below) provided that the following conditions are met:
 *
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *
 *      * Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials provided
 *        with the distribution.
 *
 *      * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *        contributors may be used to endorse or promote products derived
 *        from this software without specific prior written permission.
 *
 *  NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 *  GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 *  HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 *  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 *  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 *  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file       SmsManager.hpp
 * @brief      SmsManager class is the primary interface to manage SMS operations such as
 *             send and receive SMS text and encoded PDU buffer(s). This class handles single part
 *             and multi-part messages.
 *
 */

#ifndef SMSMANAGER_HPP
#define SMSMANAGER_HPP

#include <memory>
#include <string>
#include <vector>
#include <telux/common/CommonDefines.hpp>

namespace telux {
namespace tel {

/** @addtogroup telematics_sms
 * @{ */

class ISmsListener;
class ISmscAddressCallback;

/**
 * @brief Specifies the encoding of the SMS message
 */
enum class SmsEncoding {
   GSM7,    /**< GSM 7-bit default alphabet encoding */
   GSM8,    /**< GSM 8-bit data encoding */
   UCS2,    /**< UCS-2 encoding */
   UNKNOWN, /**< Unknown encoding */
};

/**
 * @brief Contains structure of message attributes like encoding type, number
 * of segments, characters left in last segment
 */
struct MessageAttributes {
   SmsEncoding encoding;               /**< Data encoding type */
   int numberOfSegments;               /**< Number of segments */
   int segmentSize;                    /**< Max size of each segment */
   int numberOfCharsLeftInLastSegment; /**< characters left in last segment */
};

using PduBuffer = std::vector<uint8_t>;

/**
 * @brief Structure containing information about the part of multi-part SMS such as concatenated
 * message reference number, number of segments and segment number. During concatenation this
 * information along with originating address helps in associating each part of the multi-part
 * message to the corresponding multi-part message.
 */

struct MessagePartInfo {
   uint16_t refNumber;                     /**< Concatenated message reference number as per spec
                                           3GPP TS 23.040 9.2.3.24.1. For each part of multipart
                                           message this message reference will be the same */
   uint8_t numberOfSegments;               /**< Number of segments */
   uint8_t segmentNumber;                  /**< Segment Number */

};

/**
 * @brief Data structure represents an incoming SMS. This is applicable for single part message
 *       or part of the multipart message.
 */
class SmsMessage {
public:
   SmsMessage(std::string text, std::string sender, std::string receiver, SmsEncoding encoding,
              std::string pdu, PduBuffer pduBuffer, std::shared_ptr<MessagePartInfo> info);

   /**
    * Get the message text for the single part message or part of the multipart message.
    *
    * @returns String containing SMS message.
    */
   const std::string &getText() const;

   /**
    * Get the originating address (sender address).
    *
    * @returns String containing sender address.
    */
   const std::string &getSender() const;

   /**
    * Get the destination address (receiver address).
    *
    * @returns String containing receiver address
    */
   const std::string &getReceiver() const;

   /**
    * Get encoding format used for the single part message or part of the multipart message.
    *
    * @returns SMS message encoding used.
    */
   SmsEncoding getEncoding() const;

   /**
    * Get the raw PDU for the single part message or part of the multipart message.
    *
    * @returns String containing raw PDU content.
    *
    * @deprecated Use API SmsMessage::getRawPdu
    */
   const std::string &getPdu() const;

   /**
    * Get the raw PDU buffer for the single part message or part of the multipart message.
    *
    * @returns Buffer containing raw PDU content.
    *
    * @note    Eval: This is a new API and is being evaluated. It is subject to change
    *          and could break backward compatibility
    */
   PduBuffer getRawPdu() const;

   /**
    * Applicable for multi-part SMS only. Get the information such as segment number, number of
    * segments and concatenated reference number corresponding to the part of multi-part SMS.
    *
    * @returns If a message is single part SMS the method returns null otherwise returns
    *          message part information.
    *
    * @note    Eval: This is a new API and is being evaluated. It is subject to change and
    *          could break backwards compatibility.
    */
   std::shared_ptr<MessagePartInfo> getMessagePartInfo();

   /**
    * Get the text related informative representation of this object.
    *
    * @returns String containing informative string.
    */
   const std::string toString() const;

private:
   std::string text_;                                    /**< Message text */
   std::string sender_;                                  /**< Originating address (sender) */
   std::string receiver_;                                /**< Destination address (receiver) */
   SmsEncoding encoding_;                                /**< Encoding of the SMS message */
   std::string pdu_;                                     /**< Raw PDU content. This is
                                                              deprecated use rawPdu_ */
   std::shared_ptr<MessagePartInfo> msgPartInfo_;        /**< Information related to part of
                                                              multi-part message */
   PduBuffer rawPdu_;                                    /**< Raw PDU content */
};

/**
 * This function is called in response to sending a single part or multi-part SMS. This response
 * callback is invoked  when a single part message is sent or when all the parts of a multi-part
 * message is sent. This function is called in response to telux::tel::ISmsManager::sendSms and
 * telux::tel::ISmsManager::sendRawSms APIs.
 *
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [in] msgRefs        This parameter represent the unique message reference number(s)
 *                            corresponding to single/multi-part message that we successfully sent.
 *                            When part of a message is delivered, the notification API i.e
 *                            @ref telux::tel::ISmsListener::onDeliveryReport will be invoked
 *                            with the message reference number corresponding to that part.
 * @param [in] errorCode      If sending any part of a multi-part message fails or a single part
 *                            message fails this API will return an @ref telux:common::errorcode
 *                            corresponding to the failure.
 * @note    Eval: This is a new API and is being evaluated. It is subject to change
 *          and could break backwards compatibility.
 */
using SmsResponseCb = std::function<void(std::vector<int> msgRefs,
   telux::common::ErrorCode errorCode)>;

/**
 * @brief SmsManager class is the primary interface to manage SMS operations such as
 *        send and receive an SMS text and raw encoded PDU(s). This class handles single part and
 *        multi-part messages.
 */
class ISmsManager {
public:

   /**
    * This status indicates whether the ISmsManager object is in a usable state.
    *
    * @returns @ref telux::common::ServiceStatus
    *
    */
   virtual telux::common::ServiceStatus getServiceStatus() = 0;

   /**
    * Send SMS to the destination address. When registered on IMS the SMS will be attempted over
    * IMS. If sending SMS over IMS fails, an automatic retry would be attempted to send the message
    * over CS. Only support UCS2 format, GSM 7 bit default alphabet and does not support National
    * language shift tables. On platforms with access control enabled, caller needs to have
    * TELUX_TEL_SMS_OPS permission to invoke this API successfully.
    *
    * @param [in] message           Message text to be sent
    * @param [in] receiverAddress   Receiver or destination address
    * @param [in] sentCallback      Optional callback pointer to get the response
    *                               of send SMS request.
    * @param [in] deliveryCallback  Optional callback pointer to get message
    *                               delivery status
    *
    * @deprecated Use API ISmsManager::sendSms(const std::string &message,
         const std::string &receiverAddress, bool deliveryReportNeeded = true,
         SmsResponseCb callback = nullptr, std::string smscAddr = "")
    *
    * @returns Status of sendSms i.e. success or suitable error code.
    *
    */
   virtual telux::common::Status
      sendSms(const std::string &message, const std::string &receiverAddress,
              std::shared_ptr<telux::common::ICommandResponseCallback> sentCallback = nullptr,
              std::shared_ptr<telux::common::ICommandResponseCallback> deliveryCallback = nullptr)
      = 0;


   /**
    * Send single or multipart SMS to the destination address. When registered on IMS the SMS will
    * be attempted over IMS. If sending SMS over IMS fails, an automatic retry would be attempted to
    * send the message over CS. Only support UCS2 format, GSM 7 bit default alphabet and does not
    * support National language shift tables. On platforms with access control enabled, caller needs
    * to have TELUX_TEL_SMS_OPS permission to invoke this API successfully.
    *
    * @param [in] message                 Message text to be send.
    * @param [in] receiverAddress         Receiver or destination address
    * @param [in] deliveryReportNeeded    Delivery status received in the listener API
    *                                     @ref telux::tel::ISmsListener if deliveryReportNeeded is
    *                                     true. Provided recipient responds to SMSC before the
    *                                     validity period expires. If deliveryReportNeeded is false
    *                                     delivery report will not be received.
    * @param [in] sentCallback            Optional callback pointer to get the sent response for
    *                                     single part or multi-part SMS.
    * @param [in] smscAddr                SMS is sent to SMSC address. If SMSC address is empty then
    *                                     pre-configured SMSC address is used.
    *
    * @returns Status of sendSms i.e. success or suitable error code.
    *
    * @note    Eval: This is a new API and is being evaluated. It is subject to change and
    *          could break backwards compatibility.
    */
   virtual telux::common::Status sendSms(std::string message, std::string receiverAddress,
      bool deliveryReportNeeded = true, SmsResponseCb sentCallback = nullptr,
      std::string smscAddr = "") = 0;

   /**
    * Send an SMS that is provided as a raw encoded PDU(s). When registered on IMS the SMS will
    * be attempted over IMS. If sending SMS over IMS fails, an automatic retry would be attempted to
    * send the message over CS. If the SMS is a multi-part message, the API expects multiple PDU
    * to be passed to it. On platforms with access control enabled, caller needs to have
    * TELUX_TEL_SMS_OPS permission to invoke this API successfully.
    *
    * @param [in] rawPdus             Each element in the vector represents a part of a multipart
    *                                 message. For single part message the vector will have single
    *                                 element.
    * @param [in] sentCallback        Optional callback to get the sent response for single part or
    *                                 multi-part SMS.
    *
    * @returns Status of sendRawSms i.e. success or suitable error code.
    *
    * @note    Eval: This is a new API and is being evaluated. It is subject to change and
    *          could break backwards compatibility.
    */
   virtual telux::common::Status sendRawSms(const std::vector<PduBuffer> rawPdus,
      SmsResponseCb sentCallback = nullptr) = 0;

   /**
    * Request for Short Messaging Service Center (SMSC) Address.Purpose of SMSC is to store,
    * forward, convert and deliver Short Message Service (SMS) messages.
    *
    * On platforms with access control enabled, caller needs to have TELUX_TEL_SMS_CONFIG permission
    * to invoke this API successfully.
    *
    * @param [in] callback        Optional callback pointer to get the response
    *                             of Smsc address request
    *
    * @returns Status of getSmscAddress i.e. success or suitable error code.
    */
   virtual telux::common::Status requestSmscAddress(std::shared_ptr<ISmscAddressCallback> callback
                                                    = nullptr)
      = 0;

   /**
    * Sets the Short Message Service Center(SMSC) address on the device.
    *
    * On platforms with access control enabled, caller needs to have TELUX_TEL_SMS_CONFIG permission
    * to invoke this API successfully.
    *
    * This will change the SMSC address for all the SMS messages sent from any
    * app.
    *
    * @param [in] smscAddress    SMSC address
    * @param [in] callback       Optional callback pointer to get the response
    *                            of set SMSC address
    *
    * @returns Status of setSmscAddress i.e. success or suitable error code.
    */
   virtual telux::common::Status setSmscAddress(const std::string &smscAddress,
                                                telux::common::ResponseCallback callback = nullptr)
      = 0;

   /**
    * Calculate message attributes for the given message.
    *
    * @param [in] message         Message to send
    *
    * @returns MessageAttributes structure containing encoding type, number of
    * segments, max size of segment and characters left in last segment.
    */
   virtual MessageAttributes calculateMessageAttributes(const std::string &message) = 0;

   /**
    * Get associated phone id for this SMSManager.
    *
    * @returns PhoneId.
    */
   virtual int getPhoneId() = 0;

   /**
    * Register a listener for Sms events
    *
    * @param [in] listener    Pointer to ISmsListener object which receives event
    *                         corresponding to SMS
    *
    * @returns Status of registerListener i.e. success or suitable error code.
    */
   virtual telux::common::Status registerListener(std::weak_ptr<ISmsListener> listener) = 0;

   /**
    * Remove a previously added listener.
    *
    * @param [in] listener    Pointer to ISmsListener object
    *
    * @returns Status of removeListener i.e. success or suitable error code.
    */
   virtual telux::common::Status removeListener(std::weak_ptr<ISmsListener> listener) = 0;

   virtual ~ISmsManager(){};
};

/**
 * @brief A listener class receives notification for the incoming message(s) and delivery report
 *       for sent message(s).
 *
 * The methods in listener can be invoked from multiple different threads. The
 * implementation should be thread safe.
 */
class ISmsListener : public telux::common::IServiceStatusListener {
public:
   /**
    * This function will be invoked when a single part message is received or when a part of a
    * multi-part message is received.
    *
    * On platforms with access control enabled, the client needs to have TELUX_TEL_SMS_LISTEN
    * permission to invoke this API successfully.
    *
    * @param [in] phoneId      Unique identifier per SIM slot. Phone on which the message is
    *                          received.
    * @param [in] message   Pointer to SmsMessage object
    */
   virtual void onIncomingSms(int phoneId, std::shared_ptr<SmsMessage> message) {
   }

   /**
    * This function will be invoked when either a single part message is received, or when all the
    * parts of a multipart message have been received.
    *
    * On platforms with access control enabled, the client needs to have TELUX_TEL_SMS_LISTEN
    * permission to invoke this API successfully.
    *
    * @param [in] phoneId           Unique identifier per SIM slot. Phone on which the message is
    *                               received.
    * @param [in] messages          Pointer to list of SmsMessage received corresponding to
    *                               single part or all parts of multipart message.
    *
    * @note    Eval: This is a new API and is being evaluated. It is subject to change and
    *          could break backwards compatibility.
    */
   virtual void onIncomingSms(int phoneId, std::shared_ptr<std::vector<SmsMessage>> messages) {
   }

   /**
    * This function will be invoked when either a delivery report for a single part message is
    * received or when the delivery report for part of a multi-part message is received. In order
    * to determine delivery of all parts of the multi-part message, the client application shall
    * compare message reference received in the delivery indications with message references
    * received in @ref telux::tel::SmsResponseCb.
    *
    * On platforms with access control enabled, the client needs to have TELUX_TEL_SMS_OPS
    * permission to invoke this API successfully.
    *
    * @param [in] phoneId             Unique identifier per SIM slot. Phone on which the message is
    *                                 received.
    * @param [in] msgRef              Message reference number (as per spec 3GPP TS 23.040 9.2.2.3)
    *                                 for a single part message or part of multipart message.
    * @param [in] receiverAddress     Receiver or destination address
    * @param [in] error               @ref telux::common::ErrorCode
    *
    * @note    Eval: This is a new API and is being evaluated. It is subject to change and
    *          could break backwards compatibility.
    */
   virtual void onDeliveryReport(int phoneId, int msgRef, std::string receiverAddress,
      telux::common::ErrorCode error) {
   }

   virtual ~ISmsListener() {
   }
};

/**
 * Interface for SMS callback object. Client needs to implement this interface
 * to get single shot responses for send SMS.
 *
 * The methods in callback can be invoked from multiple different threads. The
 * implementation should be thread safe.
 */
class ISmscAddressCallback : public telux::common::ICommandCallback {
public:
   /**
    * This function is called with the response to the Smsc address request.
    *
    * @param [in] address    Smsc address
    * @param [in] error      @ref telux::common::ErrorCode
    */
   virtual void smscAddressResponse(const std::string &address, telux::common::ErrorCode error) = 0;
};
/** @} */ /* end_addtogroup telematics_sms */
}
}

#endif
