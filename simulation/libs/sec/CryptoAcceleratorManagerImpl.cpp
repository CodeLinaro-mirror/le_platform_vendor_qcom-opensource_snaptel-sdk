/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "CryptoAcceleratorManagerImpl.hpp"

namespace telux {
namespace sec {

std::atomic<bool> CryptoAcceleratorManagerImpl::exitNow_;
std::mutex CryptoAcceleratorManagerImpl::destructorGuard_;

CryptoAcceleratorManagerImpl::CryptoAcceleratorManagerImpl() {
}

CryptoAcceleratorManagerImpl::~CryptoAcceleratorManagerImpl() {
    LOG(DEBUG, __FUNCTION__);
}

/*
 * Allocates and initialize resources when an application gets
 * an ICryptoAcceleratorManager instance using SecurityFactory.
 *
 * -----------------------------------------------------------------------------------------------------------
 *|        Mode         |             Request             |              Result |
 * -----------------------------------------------------------------------------------------------------------
 *| MODE_SYNC           | eccVerifyDigest()               | eccVerifyDigest() | | |
 *ecqvPointMultiplyAndAdd()       | ecqvPointMultiplyAndAdd()                         | |
 *MODE_ASYNC_POLL     | eccPostDigestForVerification()  | getAsyncResults() | | |
 *ecqvPostDataForMultiplyAndAdd() | getAsyncResults()                                 | |
 *MODE_ASYNC_LISTENER | eccPostDigestForVerification()  |
 *ICryptoAcceleratorListener::onVerificationResult()| |                     |
 *ecqvPostDataForMultiplyAndAdd() | ICryptoAcceleratorListener::onCalculationResult() |
 * -----------------------------------------------------------------------------------------------------------
 */
telux::common::ErrorCode CryptoAcceleratorManagerImpl::init(
    Mode mode, std::weak_ptr<ICryptoAcceleratorListener> cryptoAccelListener) {

    return telux::common::ErrorCode::NOT_SUPPORTED;
}

/*
 * Pass verification/calculation result to the dispatcher. If error occurs, pass result
 * as an empty vector. For ECC verification, r' is sent to application. For ECQV
 * calculation, output ecc point is sent.
 */
void CryptoAcceleratorManagerImpl::deliverResultAsync(const MVM_OUTPUT *const result) {
}

/*
 * 1. Reader thread gets a result from libmvm. It finds there is no async listener
 *    therefore attempts synchronous delivery by calling this method.
 * 2. Recepient of this result is searched by unique ID. If found, result is
 *    populated in application provided shared pointer (resultData) otherwise dropped.
 * 3. Finally, we wake up the application's thread to let the synchronous call flow
 *    complete.
 *
 *  For ECC  - R-prime is sent to application
 *  For ECQV - Output point is sent
 */
void CryptoAcceleratorManagerImpl::deliverResultSync(const MVM_OUTPUT *const result) {
}

/*
 * Unblock all application's thread waiting for results that will never come
 * due to SSR.
 */
void CryptoAcceleratorManagerImpl::unblockSyncResultWaiters() {
}

/*
 * Common worker thread to fetch both ecc/ecqv, sync/async type results from libmvm.
 */
void CryptoAcceleratorManagerImpl::resultReader() {
}

/*
 * ECC verification helper.
 * 1. Extract parameters received from the caller.
 * 2. Convert them into format as expected by libmvm.
 * 3. Finally, send them to the accelerator via Linux driver.
 */
telux::common::ErrorCode CryptoAcceleratorManagerImpl::sendECCData(const DataDigest &digest,
    const ECCPoint &publicKey, const Signature &signature, telux::sec::ECCCurve curve,
    uint32_t uniqueId, telux::sec::RequestPriority priority) {

    return telux::common::ErrorCode::NOT_SUPPORTED;
}

/*
 * Async ECC verify.
 */
telux::common::ErrorCode CryptoAcceleratorManagerImpl::eccPostDigestForVerification(
    const DataDigest &digest, const ECCPoint &publicKey, const Signature &signature,
    telux::sec::ECCCurve curve, uint32_t uniqueId, telux::sec::RequestPriority priority) {

    return telux::common::ErrorCode::NOT_SUPPORTED;
}

/*
 * Sync ECC verify.
 */
telux::common::ErrorCode CryptoAcceleratorManagerImpl::eccVerifyDigest(const DataDigest &digest,
    const ECCPoint &publicKey, const Signature &signature, telux::sec::ECCCurve curve,
    uint32_t uniqueId, telux::sec::RequestPriority priority, std::vector<uint8_t> &resultData) {

    return telux::common::ErrorCode::NOT_SUPPORTED;
}

/*
 * ECQV calculation helper.
 */
telux::common::ErrorCode CryptoAcceleratorManagerImpl::sendECQVData(
    const ECCPoint &multiplicandPoint, const ECCPoint &addendPoint, const Scalar &scalar,
    telux::sec::ECCCurve curve, uint32_t uniqueId, telux::sec::RequestPriority priority) {

    return telux::common::ErrorCode::NOT_SUPPORTED;
}

/*
 * Async ECQV calculation.
 */
telux::common::ErrorCode CryptoAcceleratorManagerImpl::ecqvPostDataForMultiplyAndAdd(
    const ECCPoint &multiplicandPoint, const ECCPoint &addendPoint, const Scalar &scalar,
    telux::sec::ECCCurve curve, uint32_t uniqueId, telux::sec::RequestPriority priority) {

    return telux::common::ErrorCode::NOT_SUPPORTED;
}

/*
 * Sync ECQV calculation.
 */
telux::common::ErrorCode CryptoAcceleratorManagerImpl::ecqvPointMultiplyAndAdd(
    const ECCPoint &multiplicandPoint, const ECCPoint &addendPoint, const Scalar &scalar,
    telux::sec::ECCCurve curve, uint32_t uniqueId, telux::sec::RequestPriority priority,
    std::vector<uint8_t> &resultData) {

    return telux::common::ErrorCode::NOT_SUPPORTED;
}

/*
 * Block until libmvm has result to provide.
 */
telux::common::ErrorCode CryptoAcceleratorManagerImpl::getAsyncResults(
    std::vector<OperationResult> &results, uint32_t numResultsToRead, int32_t timeout,
    uint32_t &numResultsRead) {

    return telux::common::ErrorCode::NOT_SUPPORTED;
}

/*
 *  Receives SSR events from libmvm and passes to the registered application's listener.
 *
 *  SSR occurrence possibilities and corresponding handling:
 *
 *  1. SSR happens just after returning from mvm_connect() but before returning from
 *     getCryptoAcceleratorManager(). If the application has registered listener, it will
 *     be called. Application should address this case as per its requirement possibly by
 *     lock protecting getCryptoAcceleratorManager() and onServiceStatusChange().
 *  2. When getCryptoAcceleratorManager() is called, MVM crashed and restarting. Neither
 *     libmvm nor kernel driver initialize or access MVM HW during open system call.
 *     Therefore, depending upon the time instant application may or may not get ONLINE event,
 *     which does not affect it in any way.
 *  3. SSR happens during CryptoAcceleratorManagerImpl destruction (various possibilities).
 *     ICryptoAcceleratorListener is saved as a weak pointer, therefore, handled implicitly.
 */
void CryptoAcceleratorManagerImpl::cryptoAcceleratorSSRCallback(
    mvm_state newState, void *ssrCookie) {

}

}  // End of namespace sec
}  // End of namespace telux
