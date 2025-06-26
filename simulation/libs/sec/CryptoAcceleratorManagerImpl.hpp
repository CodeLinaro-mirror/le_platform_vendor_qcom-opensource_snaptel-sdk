/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef CRYPTOACCELERATORMANAGERIMPL_HPP
#define CRYPTOACCELERATORMANAGERIMPL_HPP

#include <unordered_map>

#include "common/CommonUtils.hpp"
#include "common/TaskDispatcher.hpp"
#include "common/ListenerManager.hpp"

#include <telux/sec/CryptoAcceleratorManager.hpp>

#include "internal-temp.h"

namespace telux {
namespace sec {

/* Used as cookie passed for SSR */
struct PrivateSSRData {
    std::weak_ptr<ICryptoAcceleratorManager> cryptoAcceleratorManagerImpl;
};

class CryptoAcceleratorManagerImpl
   : public ICryptoAcceleratorManager,
     public std::enable_shared_from_this<CryptoAcceleratorManagerImpl> {

 public:
    struct WaitStateCarrier {
        std::mutex guard;
        std::condition_variable cv;
        telux::common::ErrorCode ec;
        std::vector<uint8_t> resultData;
    };

    CryptoAcceleratorManagerImpl();
    ~CryptoAcceleratorManagerImpl();

    CryptoAcceleratorManagerImpl(const CryptoAcceleratorManagerImpl &)            = delete;
    CryptoAcceleratorManagerImpl &operator=(const CryptoAcceleratorManagerImpl &) = delete;

    telux::common::ErrorCode init(Mode mode, std::weak_ptr<ICryptoAcceleratorListener> caListener);

    //****** MODE_ASYNC_LISTENER/MODE_ASYNC_POLL - Asynchronous APIs ******//

    telux::common::ErrorCode eccPostDigestForVerification(const DataDigest &digest,
        const ECCPoint &publicKey, const Signature &signature, telux::sec::ECCCurve curve,
        uint32_t uniqueId, telux::sec::RequestPriority priority) override;

    telux::common::ErrorCode ecqvPostDataForMultiplyAndAdd(const ECCPoint &multiplicandPoint,
        const ECCPoint &addendPoint, const Scalar &scalar, telux::sec::ECCCurve curve,
        uint32_t uniqueId, telux::sec::RequestPriority priority) override;

    telux::common::ErrorCode getAsyncResults(std::vector<OperationResult> &results,
        uint32_t numResultsToRead, int32_t timeout, uint32_t &numResultsRead) override;

    //*********** MODE_SYNC - Synchronous APIs ***********//

    telux::common::ErrorCode eccVerifyDigest(const DataDigest &digest, const ECCPoint &publicKey,
        const Signature &signature, telux::sec::ECCCurve curve, uint32_t uniqueId,
        telux::sec::RequestPriority priority, std::vector<uint8_t> &resultData) override;

    telux::common::ErrorCode ecqvPointMultiplyAndAdd(const ECCPoint &multiplicandPoint,
        const ECCPoint &addendPoint, const Scalar &scalar, telux::sec::ECCCurve curve,
        uint32_t uniqueId, telux::sec::RequestPriority priority,
        std::vector<uint8_t> &resultData) override;

    static void cryptoAcceleratorSSRCallback(enum mvm_state newState, void *ssrCookie);

    /* Protect against concurrent SSR and CryptoAcceleratorManagerImpl destruction */
    static std::mutex destructorGuard_;

    /* Set to true to indicate - reader thread should terminate now */
    static std::atomic<bool> exitNow_;

 private:
    PrivateSSRData *privateSSRData_;

    std::unique_ptr<MVM_OUTPUT[]> resultsBuffer_;

    /* Updated with user provided mode during init */
    Mode resultDeliveryMode_ = Mode::MODE_ASYNC_POLL;

    /* Protect against concurrent map modification by application thread (add) and the
     * the reader thread (delete) */
    std::mutex dataToWSCMapGuard_;

    /* Associates ECC data to the condition variable so that reader thread can wake up
     * waiting application's thread indicating synchronous call completed */
    std::unordered_map<uint32_t, struct WaitStateCarrier *> dataToWSCMap_;

    /* Set to true if the mvm initialization succeeds */
    bool connectionInitialized_ = false;

    /* Dispatcher to deliver results to the ecc/ecqv listeners */
    std::shared_ptr<telux::common::TaskDispatcher> asyncResultAndSsrDispatcher_;

    std::shared_ptr<telux::common::ListenerManager<ICryptoAcceleratorListener>> caListenerMgr_;

    /* Used during SSR */
    telux::common::ServiceStatus currentServiceStatus_;

    /* Unblocks application threads upon SSR in sync mode, for graceful recovery */
    void unblockSyncResultWaiters(void);

    /* This is the actual task executed by the worker thread to read result from the
     * libmvm and pass it to the registered listener with the help from dispatcher */
    void resultReader();

    /* Passes ECC/ECQV result to listener asynchronously */
    inline void deliverResultAsync(const MVM_OUTPUT *const result);

    /* Passes ECC/ECQV result to listener synchronously */
    inline void deliverResultSync(const MVM_OUTPUT *const result);

    /* Common helper methods (used by both sync and async APIs) that actually sends data
     * to the crypto accelerator. */
    telux::common::ErrorCode sendECCData(const DataDigest &digest, const ECCPoint &publicKey,
        const Signature &signature, telux::sec::ECCCurve curve, uint32_t uniqueId,
        telux::sec::RequestPriority priority);

    telux::common::ErrorCode sendECQVData(const ECCPoint &multiplicandPoint,
        const ECCPoint &addendPoint, const Scalar &scalar, telux::sec::ECCCurve curve,
        uint32_t uniqueId, telux::sec::RequestPriority priority);
};

}  // End of namespace sec
}  // End of namespace telux

#endif  // CRYPTOACCELERATORMANAGERIMPL_HPP
