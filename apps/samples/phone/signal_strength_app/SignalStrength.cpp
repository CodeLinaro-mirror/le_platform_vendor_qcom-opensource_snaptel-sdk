/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * This application demonstrates how to request signal strength. The steps are as follows:
 *
 * 1. Get a PhoneFactory instance.
 * 2. Get a IPhoneManager instance from the PhoneFactory.
 * 3. Wait for the phone manager service to become available.
 * 4. Create a phone instance.
 * 5. Request signal strength information.
 * 6. Update the response
 *
 * Usage:
 * # ./signal_strength_app
 */

#include <errno.h>

#include <iostream>
#include <memory>
#include <cstdlib>
#include <future>
#include <chrono>
#include <thread>

#include <telux/common/CommonDefines.hpp>
#include <telux/tel/PhoneDefines.hpp>
#include <telux/tel/PhoneFactory.hpp>
#include <telux/tel/PhoneManager.hpp>
#include <telux/tel/SignalStrength.hpp>

class PhoneMaker : public telux::tel::ISignalStrengthCallback,
                public std::enable_shared_from_this<PhoneMaker> {
 public:
    int init() {
        telux::common::ServiceStatus serviceStatus;
        std::promise<telux::common::ServiceStatus> p{};

        /* Step - 1 */
        auto &phoneFactory = telux::tel::PhoneFactory::getInstance();

        /* Step - 2 */
        phoneMgr_ = phoneFactory.getPhoneManager(
                [&p](telux::common::ServiceStatus status) {
            p.set_value(status);
        });

        if (!phoneMgr_) {
            std::cout << "Can't get IPhoneManager" << std::endl;
            return -ENOMEM;
        }

        /* Step - 3 */
        serviceStatus = p.get_future().get();
        if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "Phone manager service unavailable, status " <<
                static_cast<int>(serviceStatus) << std::endl;
            return -EIO;
        }
        /* Step - 4 */
        if (serviceStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::vector<int> phoneIds;
            telux::common::Status status = phoneMgr_->getPhoneIds(phoneIds);
            if (status == telux::common::Status::SUCCESS) {
                for (unsigned int index = 1; index <= phoneIds.size(); index++) {
                     auto phone = phoneMgr_->getPhone(index);
                     if (phone != nullptr) {
                         phones_.emplace_back(phone);
                     }
                }
            }
       }

        std::cout << "Initialization complete" << std::endl;
        return 0;
    }

    int requestSignalStrength() {
        telux::common::Status status;
        int phoneId = DEFAULT_PHONE_ID;
        if (phones_.empty()) {
            std::cout << "No phones available" << std::endl;
            return -ENODEV;
        }
        auto phone = phones_[phoneId - 1];
        /* Step - 5 */
        status = phone->requestSignalStrength(shared_from_this());
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't request signal strength, err " << static_cast<int>(status)
                      << std::endl;
            return -EIO;
        }

        return 0;
    }

    std::string signalLevelToString(telux::tel::SignalStrengthLevel level) {
        switch(level){
            case telux::tel::SignalStrengthLevel::LEVEL_1 : return "LEVEL_1";
            case telux::tel::SignalStrengthLevel::LEVEL_2 : return "LEVEL_2";
            case telux::tel::SignalStrengthLevel::LEVEL_3 : return "LEVEL_3";
            case telux::tel::SignalStrengthLevel::LEVEL_4 : return "LEVEL_4";
            case telux::tel::SignalStrengthLevel::LEVEL_5 : return "LEVEL_5";
            case telux::tel::SignalStrengthLevel::LEVEL_UNKNOWN : return "LEVEL_UNKNOWN";
            default:
                return "Invalid Signal Level";
        }
    }
    /* Step - 6 */
    void signalStrengthResponse(std::shared_ptr<telux::tel::SignalStrength> signalStrength,
        telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "Received Signal Strength Callback with Error Code: "
             << static_cast<int>(error) << std::endl;
        if (signalStrength->getGsmSignalStrength() != nullptr) {
            if (signalStrength->getGsmSignalStrength()->getGsmSignalStrength()
                == INVALID_SIGNAL_STRENGTH_VALUE) {
                std::cout << "GSM Signal Strength: "<< "UNAVAILABLE" << std::endl;
            } else {
                std::cout << "GSM Signal Strength: "
                    << signalStrength->getGsmSignalStrength()->getGsmSignalStrength() << std::endl;
            }

           if (signalStrength->getGsmSignalStrength()->getGsmBitErrorRate()
               == INVALID_SIGNAL_STRENGTH_VALUE) {
               std::cout << "GSM Bit Error Rate: "<< "UNAVAILABLE" << std::endl;
           } else {
               std::cout << "GSM Bit Error Rate: "
                   << signalStrength->getGsmSignalStrength()->getGsmBitErrorRate()<< std::endl;
           }

           if (signalStrength->getGsmSignalStrength()->getDbm() == INVALID_SIGNAL_STRENGTH_VALUE) {
               std::cout << "GSM Signal Strength(in dBm): " << "UNAVAILABLE" << std::endl;
           } else {
               std::cout << "GSM Signal Strength(in dBm): "
                   << signalStrength->getGsmSignalStrength()->getDbm() << std::endl;
           }

           if (signalStrength->getGsmSignalStrength()->getRssi() == INVALID_SIGNAL_STRENGTH_VALUE) {
               std::cout << "GSM Received Signal Strength Indicator(in dBm): " << "UNAVAILABLE"
                   << std::endl;
           } else {
               std::cout << "GSM Received Signal Strength Indicator(in dBm): "
                   << signalStrength->getGsmSignalStrength()->getRssi() << std::endl;
           }
           if (signalStrength->getGsmSignalStrength()->getTimingAdvance()
                == INVALID_SIGNAL_STRENGTH_VALUE) {
                std::cout << "GSM Timing Advance(in bit periods): " << "UNAVAILABLE" << std::endl;
           } else {
                std::cout << "GSM Timing Advance(in bit periods): "
                    << signalStrength->getGsmSignalStrength()->getTimingAdvance() << std::endl;
           }

           std::cout << "GSM Signal Level: "
               << signalLevelToString(signalStrength->getGsmSignalStrength()->getLevel())
               << std::endl;
       }

       if (signalStrength->getLteSignalStrength() != nullptr) {
           if (signalStrength->getLteSignalStrength()->getLteSignalStrength()
               == INVALID_SIGNAL_STRENGTH_VALUE) {
               std::cout << "LTE Signal Strength: "<< "UNAVAILABLE" << std::endl;
           } else {
               std::cout << "LTE Signal Strength: "
                   << signalStrength->getLteSignalStrength()->getLteSignalStrength() << std::endl;
           }

           if (signalStrength->getLteSignalStrength()->getDbm() == INVALID_SIGNAL_STRENGTH_VALUE) {
               std::cout << "LTE Signal Strength(in dBm): "<< "UNAVAILABLE" << std::endl;
               std::cout << "LTE Reference Signal Receive Power(in dBm): "<< "UNAVAILABLE"
                   << std::endl;
           } else {
               std::cout << "LTE Signal Strength(in dBm): "
                   << signalStrength->getLteSignalStrength()->getDbm() << std::endl;
               std::cout << "LTE Reference Signal Receive Power(in dBm): "
                   << signalStrength->getLteSignalStrength()->getDbm() << std::endl;
           }

           if (signalStrength->getLteSignalStrength()->getRssi() == INVALID_SIGNAL_STRENGTH_VALUE) {
               std::cout << "LTE Received Signal Strength Indicator(in dBm): " << "UNAVAILABLE"
                   << std::endl;
           } else {
               std::cout << "LTE Received Signal Strength Indicator(in dBm): "
                   << signalStrength->getLteSignalStrength()->getRssi() << std::endl;
           }

           if (signalStrength->getLteSignalStrength()->getLteReferenceSignalReceiveQuality()
               == INVALID_SIGNAL_STRENGTH_VALUE) {
               std::cout << "LTE Reference Signal Receive Quality(in dB): "
                   << "UNAVAILABLE" << std::endl;
           } else {
               std::cout << "LTE Reference Signal Receive Quality(in dB): "
                   << signalStrength->getLteSignalStrength()->getLteReferenceSignalReceiveQuality()
                   << std::endl;
           }

           if (signalStrength->getLteSignalStrength()->getLteReferenceSignalSnr()
               == INVALID_SIGNAL_STRENGTH_VALUE) {
               std::cout << "LTE Reference Signal SNR(in dB): "<< "UNAVAILABLE" << std::endl;
           } else {
               std::cout << "LTE Reference Signal SNR(in dB): "
                   << signalStrength->getLteSignalStrength()->getLteReferenceSignalSnr() * 0.1
                 << std::endl;
           }

           std::cout << "LTE Signal Level: "
               << signalLevelToString(signalStrength->getLteSignalStrength()->getLevel())
               << std::endl;
       }

       if (signalStrength->getWcdmaSignalStrength() != nullptr) {
           if (signalStrength->getWcdmaSignalStrength()->getSignalStrength()
               == INVALID_SIGNAL_STRENGTH_VALUE) {
               std::cout << "WCDMA Signal Strength: "<< "UNAVAILABLE" << std::endl;
           } else {
                std::cout << "WCDMA Signal Strength: "
                    << signalStrength->getWcdmaSignalStrength()->getSignalStrength() << std::endl;
           }

           if (signalStrength->getWcdmaSignalStrength()->getDbm() == INVALID_SIGNAL_STRENGTH_VALUE)
           {
               std::cout << "WCDMA Signal Strength(in dBm): "<< "UNAVAILABLE" << std::endl;
           } else {
               std::cout << "WCDMA Signal Strength(in dBm): "
                   << signalStrength->getWcdmaSignalStrength()->getDbm() << std::endl;
           }

           if (signalStrength->getWcdmaSignalStrength()->getRssi() == INVALID_SIGNAL_STRENGTH_VALUE)
           {
               std::cout << "WCDMA Received Signal Strength Indicator(in dBm): " << "UNAVAILABLE"
                   << std::endl;
           } else {
               std::cout << "WCDMA Received Signal Strength Indicator(in dBm): "
                   << signalStrength->getWcdmaSignalStrength()->getRssi() << std::endl;
           }

           if (signalStrength->getWcdmaSignalStrength()->getBitErrorRate()
               == INVALID_SIGNAL_STRENGTH_VALUE) {
               std::cout << "WCDMA Bit Error Rate: "<< "UNAVAILABLE" << std::endl;
           } else {
               std::cout << "WCDMA Bit Error Rate: "
                   << signalStrength->getWcdmaSignalStrength()->getBitErrorRate() << std::endl;
           }

           if (signalStrength->getWcdmaSignalStrength()->getEcio()
               == INVALID_SIGNAL_STRENGTH_VALUE) {
               std::cout << "WCDMA Energy per chip to Interference Power Ratio(in dB): "
                  << "UNAVAILABLE" << std::endl;
           } else {
               std::cout << "WCDMA Energy per chip to Interference Power Ratio(in dB): "
                  << signalStrength->getWcdmaSignalStrength()->getEcio() << std::endl;
           }

          if (signalStrength->getWcdmaSignalStrength()->getRscp()
              == INVALID_SIGNAL_STRENGTH_VALUE) {
              std::cout << "WCDMA Reference Signal Code Power(in dBm): "
                  << "UNAVAILABLE" << std::endl;
          } else {
              std::cout << "WCDMA Reference Signal Code Power(in dBm): "
                  << signalStrength->getWcdmaSignalStrength()->getRscp() << std::endl;
          }

          std::cout
              << "WCDMA Signal Level: "
              << signalLevelToString(signalStrength->getWcdmaSignalStrength()->getLevel())
              << std::endl;
        }


        if (signalStrength->getNr5gSignalStrength() != nullptr) {
            if (signalStrength->getNr5gSignalStrength()->getNr5gSignalStrength()
                == INVALID_SIGNAL_STRENGTH_VALUE) {
                std::cout << "5G NR Signal Strength: "<< "UNAVAILABLE" << std::endl;
            } else {
                std::cout << "5G NR Signal Strength: "
                    << signalStrength->getNr5gSignalStrength()->getNr5gSignalStrength()
                    << std::endl;
            }
            if (signalStrength->getNr5gSignalStrength()->getDbm() == INVALID_SIGNAL_STRENGTH_VALUE)
            {
                std::cout << "5G NR Signal Strength(in dBm): "<< "UNAVAILABLE" << std::endl;
            } else {
                std::cout << "5G NR Signal Strength(in dBm): "
                    << signalStrength->getNr5gSignalStrength()->getDbm() << std::endl;
            }

            if (signalStrength->getNr5gSignalStrength()->getReferenceSignalReceiveQuality()
                == INVALID_SIGNAL_STRENGTH_VALUE) {
                std::cout << "5G NR Receive Quality(in dB): "<< "UNAVAILABLE" << std::endl;
            } else {
                std::cout << "5G NR Receive Quality(in dB): "
                    << signalStrength->getNr5gSignalStrength()->getReferenceSignalReceiveQuality()
                    << std::endl;
            }

            if (signalStrength->getNr5gSignalStrength()->getReferenceSignalSnr()
                == INVALID_SIGNAL_STRENGTH_VALUE) {
                std::cout << "5G Reference Signal SNR(in dB): "<< "UNAVAILABLE" << std::endl;
            } else {
                std::cout << "5G Reference Signal SNR(in dB): "
                    << signalStrength->getNr5gSignalStrength()->getReferenceSignalSnr() * 0.1
                    << std::endl;
            }

            std::cout << "5G Signal Level: "
                << signalLevelToString(signalStrength->getNr5gSignalStrength()->getLevel())
                << std::endl;
        }

        if (signalStrength->getNb1NtnSignalStrength() != nullptr) {
            if (signalStrength->getNb1NtnSignalStrength()->getSignalStrength()
                == INVALID_SIGNAL_STRENGTH_VALUE) {
                std::cout << "NB1 NTN Signal Strength: "<< "UNAVAILABLE" << std::endl;
            } else {
                std::cout << "NB1 NTN Signal Strength: "
                    << signalStrength->getNb1NtnSignalStrength()->getSignalStrength()
                    << std::endl;
            }

            if (signalStrength->getNb1NtnSignalStrength()->getDbm()
                == INVALID_SIGNAL_STRENGTH_VALUE) {
                std::cout << "NB1 NTN Signal Strength(in dBm): "<< "UNAVAILABLE" << std::endl;
                std::cout << "NB1 NTN Reference Signal Receive Power(in dBm): " << "UNAVAILABLE"
                    << std::endl;
            } else {
                std::cout << "NB1 NTN Signal Strength(in dBm): "
                    << signalStrength->getNb1NtnSignalStrength()->getDbm() << std::endl;
                std::cout << "NB1 NTN Reference Signal Receive Power(in dBm): "
                    << signalStrength->getNb1NtnSignalStrength()->getDbm() << std::endl;
            }

            if (signalStrength->getNb1NtnSignalStrength()->getRssi()
                == INVALID_SIGNAL_STRENGTH_VALUE) {
                std::cout << "NB1 NTN Received Signal Strength Indicator(in dBm): " << "UNAVAILABLE"
                    << std::endl;
            } else {
                std::cout << "NB1 NTN Received Signal Strength Indicator(in dBm): "
                    << signalStrength->getNb1NtnSignalStrength()->getRssi() << std::endl;
            }

            if (signalStrength->getNb1NtnSignalStrength()->getRsrq()
                == INVALID_SIGNAL_STRENGTH_VALUE) {
                std::cout << "NB1 NTN Reference Signal Receive Quality(in dB): "
                    << "UNAVAILABLE" << std::endl;
            } else {
                std::cout << "NB1 NTN Reference Signal Receive Quality(in dB): "
                    << signalStrength->getNb1NtnSignalStrength()->getRsrq() << std::endl;
            }

            if (signalStrength->getNb1NtnSignalStrength()->getRssnr()
                == INVALID_SIGNAL_STRENGTH_VALUE) {
                std::cout << "NB1 NTN Reference Signal SNR(in dB): " << "UNAVAILABLE" << std::endl;
            } else {
                std::cout << "NB1 NTN Reference Signal SNR(in dB): "
                    << signalStrength->getNb1NtnSignalStrength()->getRssnr() * 0.1 << std::endl;
            }

            std::cout << "NB1 NTN Signal Level: "
                << signalLevelToString(signalStrength->getNb1NtnSignalStrength()->getLevel())
                << std::endl;
        }
    }

 private:
    std::vector<std::shared_ptr<telux::tel::IPhone>> phones_;
    std::shared_ptr<telux::tel::IPhoneManager> phoneMgr_;
};

int main(int argc, char *argv[]) {

    int ret;
    std::shared_ptr<PhoneMaker> app;

    try {
        app = std::make_shared<PhoneMaker>();
    } catch (const std::exception& e) {
        std::cout << "Can't allocate PhoneMaker" << std::endl;
        return -ENOMEM;
    }

    ret = app->init();
    if (ret < 0) {
        return ret;
    }

    ret = app->requestSignalStrength();
    if (ret < 0) {
        return ret;
    }

    /* Wait for receiving all asynchronous responses */
    std::this_thread::sleep_for(std::chrono::seconds(3));

    std::cout << "\nSignal strength app exiting" << std::endl;
    return 0;
}
