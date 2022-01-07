/*
 *  Copyright (c) 2019-2021, The Linux Foundation. All rights reserved.
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
#include <iostream>
#include "AerolinkSecurity.hpp"

/* STATIC VARIABLES */
/* Asynchronous Function Variables */
static      AEROLINK_RESULT signCallbackStatus;
static      void* signCallbackUserData;
static      uint8_t* signCallbackData;
static      uint32_t signCallbackDataLen;
static      volatile uint32_t signCallbackCalled;
static      AEROLINK_RESULT verifyCallbackStatus;
static      void* verifyCallbackUserData;
static      volatile uint32_t verifyCallbackCalled;

/* Logging Related Variables */
static      struct timeval currTime;
static      double prevTimeStamp, prevBatchTimeStamp;
static      double startTime, avgRate, minBatchTime, avgBatchTime, maxBatchTime;
static      uint32_t verifLatency = 0; // added into vector of verif latencies
static      uint32_t verifMsgIdx = 0;   // verif latency queue element to update next
static      uint32_t latencyListSize = 10000; // by default 10k?
static      int verifSuccess, prevVerifSuccess, verifFail;
static      int signSuccess, prevSignSuccess, signFail;
static      int queuedVerifs = 0;
static      int secVerbosity = 0;

/* Semaphores */
static      sem_t smpListSem;
static      sem_t smgListSem;
static      sem_t verifQueueSem;
static      sem_t signLogSem;
static      sem_t verifLogSem;
static      sem_t idChangeSem;

static AEROLINK_RESULT completeChangeId_status;
static bool retChangeId_status; // tells thread if callback has completed

/* LOGGING FUNCTIONS */
// Function to set the verbosity of these security related functions
// 0   -> Quiet
// 1-4 -> Statistics and Errors
// > 4 -> Verbose Mode
void AerolinkSecurity::setSecVerbosity(uint8_t verbosity){
    secVerbosity = verbosity;
}

void AerolinkSecurity::setStartTime(double start){
    prevBatchTimeStamp = startTime;
}

/* Function to print out running signing stats */
void printSignStats(std::thread::id thrId){
    time_t t;
    struct tm *info;
    gettimeofday(&currTime, NULL);
    t = currTime.tv_sec;
    info = localtime(&t);
    if(signSuccess % 10 == 0 && signSuccess > 0){
        fprintf(stdout, "ThreadID: 0x%08x; ", thrId);
        printf(" %s : SignSuccess: %d; SignFail: %d\n",
                    asctime (info), signSuccess, signFail);
    }
}

/* Function to print out running verification stats */
void printVerifStats(std::thread::id thrId){
    gettimeofday(&currTime, NULL);
    double currTimeStamp =
            (currTime.tv_sec * 1000.0) + (currTime.tv_usec/1000.0);

    if(verifFail % 2500 == 0 && verifFail > 0){
        if(secVerbosity > 4)
            printf("VerifSuccess: %d; VerifFail: %d\n", verifSuccess, verifFail);
    }

    // batch stats reporting (per 2500 successful verifications)
    if(verifSuccess % 2500 == 0 && verifSuccess > 0){
       if(verifSuccess > prevVerifSuccess){
            double dur = currTimeStamp-prevBatchTimeStamp;
            double delta = verifSuccess-prevVerifSuccess;
            double rate = delta/dur;
            // minimum time so far per 2500*N verifications
            minBatchTime = std::min(minBatchTime, dur);
            // running time per 2500*N verifications
            maxBatchTime = std::max(maxBatchTime, dur);
            // average time per 2500*N verifications
            avgBatchTime = (avgBatchTime+dur)/2.0;

            // overall average batch rate
            // dealing with initializing variables for first verification
            if(minBatchTime <= 0.0){
                minBatchTime = dur;
                avgBatchTime = dur;
            }

             // running avg rate:
            avgRate = 2500.0/avgBatchTime;

            // logging for batch verif stats
            fprintf(stdout, "ThreadID: 0x%08x; ", thrId);
            fprintf(stdout, "TotalSuccessfulVerifs: %d;\n", verifSuccess);
            fprintf(stdout, "BatchVerifRate: %fk VHz; ", rate);
            fprintf(stdout, "AvgBatchVerifRate: %fk VHz; \n", avgRate);
            fprintf(stdout, "BatchTimeStep: %fms;\n", dur);
            fprintf(stdout, "MinBatchTime: %fms; ", minBatchTime);
            fprintf(stdout, "MaxBatchTime: %fms; ", maxBatchTime);
            fprintf(stdout, "AvgBatchTime: %fms;\n", avgBatchTime);

            // logging for individual verif stats - includes ITS overhead
            if(secVerbosity > 1){
                fprintf(stdout, "CurrTime: %fms; ", currTimeStamp);
                fprintf(stdout, "PrevBatchTime: %fms;\n", prevBatchTimeStamp);
            }
            fprintf(stdout, "\n");
        }
        prevVerifSuccess = verifSuccess;
        // get latest time stamp because print statements cause delay
        gettimeofday(&currTime, NULL);
        prevBatchTimeStamp =
                    (currTime.tv_sec * 1000.0) + (currTime.tv_usec/1000.0);

    }
    // get latest time stamp because print statements cause delay
    gettimeofday(&currTime, NULL);
    prevTimeStamp =
                    (currTime.tv_sec * 1000.0) + (currTime.tv_usec/1000.0);
}

/* CERT CHANGE FUNCTIONS */
/* The following type defines a callback function prototype for initiation of the ID-change
    protocol. A local certificate manager (LCM) manages sets of end-entity certificates for one or
    more applications (PSID/SSPs) that share those certificates (are authorized by those
    certificates to sign messages).
    numCerts provides the number of simultaneously valid certificates for the registered
    LCM(), e.g. 20, and on its return, this function can specify the next certificate to be used
    by setting cert_index to a value from 1 – 20. A value of 0 is used to indicate no
    preference, and Aerolink will choose the next certificate. An invalid value (greater than
    the number of certificates) will be treated as a value of 0.
    typedef void (*IdChangeInitCallback)(
    void *userData, // IN
    uint8_t numCerts, // IN
    uint8_t *certIndex); // OUT
*/

static void initIdChangeCbFn(void *userData, unsigned char numCerts, unsigned char *certIndxCb){
    // on call back, this function provides the new cert index for the complete id change cb fn
}

/* The following type defines a callback function prototype for completion of the ID-change
 *   protocol. returnCode indicates whether the ID change was aborted, or whether the
 *   data-signing certificates for all LCMs were changed and all other identifiers should now
 *   be changed. If the ID change was not aborted, certId will contain the last eight bytes of
 *   the whole-certificate hash of the new data-signing certificate for the registered LCM.
 *   The certId pointer is no longer valid after this function returns.
 *   typedef void (*IdChangeDoneCallback)(
 *   AEROLINK_RESULT returnCode, // IN
 *   void *userData, // IN
 *   uint8_t const *certId); // IN
 */
static void completeIdChangeCbFn(AEROLINK_RESULT returnCode, void *userData, const unsigned char *certIdCb){
    // tells the user whether the id change was completed successfully or not.
    completeChangeId_status = returnCode;
    if(returnCode == WS_SUCCESS){
        // cast userdata to the user data type struct
        IDChangeData* tempPtr = (IDChangeData*)userData;
        memcpy(tempPtr->certId, certIdCb, sizeof(tempPtr->certId));
        memcpy(tempPtr->tempId, certIdCb, sizeof(tempPtr->tempId)); // start at offset of 2 to get last 6 bytes
        tempPtr->idChanged = true;
        if(secVerbosity > 1){
            std::cout << "New cert hash ID is: " ;
            for(int i = 0 ; i < sizeof(tempPtr->certId); i++){
                printf("%02x:", tempPtr->certId[i]);
            }
            std::cout << "\n\n";
        }
    }
    else{
        if(secVerbosity > 1)
            fprintf(stderr,"Failed to Perform ID Change\n");
    }
    retChangeId_status = true;
    sem_post(&idChangeSem);
}

// LOCK-BASED FUNCTIONS will need to be used for safety-critical events to happen properly.
/*
 *   This function applies a lock to block ID-change-protocol initiations.
 *   AEROLINK_RESULT securityServices_idChangeLock();
 *
 *   This function removes a lock to allow ID-change-protocol initiations.
 *   AEROLINK_RESULT securityServices_idChangeUnlock();
 */

// Function to call the Aerolink id change call and provide the callback function
int AerolinkSecurity::idChange (){

    AEROLINK_RESULT result;
    result = securityServices_idChangeInit();
    if(result != WS_SUCCESS)
        return -1;

    // wait until callback function is called
    sem_wait(&idChangeSem);
    retChangeId_status = false;
    // on successful return, modify other id-related information in upper layer
    return result;
}


/* INITIALIZATION/DEINITIALIZATION FUNCTIONS */

AerolinkSecurity *AerolinkSecurity::pInstance = nullptr;

// Create new aerolinksecurity instance
AerolinkSecurity * AerolinkSecurity::Instance(std::string ctxName,
                                            uint16_t countryCode) {
    if(pInstance == nullptr){
        AerolinkSecurity::pInstance =
            new AerolinkSecurity(ctxName, countryCode);
    }
    return AerolinkSecurity::pInstance;
}

// Create new aerolinksecurity instance with optional crypto key method
AerolinkSecurity * AerolinkSecurity::Instance(std::string ctxName,
                            uint16_t countryCode, uint8_t keyGenMethod) {
    if(pInstance == nullptr){
        AerolinkSecurity::pInstance =
            new AerolinkSecurity(ctxName, countryCode, keyGenMethod);
    }
    return AerolinkSecurity::pInstance;
}

// Create new aerolinksecurity instance with optional crypto key method
AerolinkSecurity * AerolinkSecurity::Instance(std::string ctxName,
                            uint16_t countryCode,  char const* lcmName,
                            IDChangeData& idChangeData) {
    if(pInstance == nullptr){
        AerolinkSecurity::pInstance =
            new AerolinkSecurity(ctxName, countryCode, lcmName, idChangeData);
    }
    return AerolinkSecurity::pInstance;
}

// Primary initialization function for Aerolink services
int AerolinkSecurity::init(void) {
    sem_init(&verifLogSem, 0, 1);
    sem_init(&signLogSem, 0, 1);
    sem_init(&smpListSem, 0, 1);
    sem_init(&smgListSem, 0, 1);
    sem_init(&idChangeSem, 0, 1);
    gettimeofday(&currTime, NULL);
    startTime = currTime.tv_sec*1000.0 + currTime.tv_usec/1000;
    prevBatchTimeStamp = startTime;

    const char *aerolinkLibVersion = securityServices_getVersion();
    if(aerolinkLibVersion != nullptr) {
        std::cout << "Aerolink Library Version: " << aerolinkLibVersion << std::endl;
    }

    AEROLINK_RESULT result;
    if ((result = securityServices_initialize()) != WS_SUCCESS) {
        if(secVerbosity > 0)
            fprintf(stderr, "SecurityServices initialization failed (%s)\n",
                ws_errid(result));
    }
    /*
     * Adjust the time for the expiration of signatures and certificates
     */
    uint32_t leapSeconds = 0;
    if ((result = securityServices_setTimeAdjustment(leapSeconds)) != WS_SUCCESS) {
        std::cerr << "securityServices_setTimeAdjustment failed: " << result << std::endl;
        return -1;
    }

    // May add the generator location here as well
    result = sc_open(SecurityCtxName_.c_str(), &secContext_);
    if (result != WS_SUCCESS) {
        if(secVerbosity > 0)
            fprintf(stderr,
                    "Failed to open security context (%s)\n",
                    ws_errid(result));
        return -1;
    }

    /*
     * Create Secured Message Generator
     */
    result = smg_new(secContext_, &this->smg_);
    if (result != WS_SUCCESS) {
        std::cerr << "Failed to create signed message generator: " << result << std::endl;
        return -1;
    }

    /*
     * Register LCM for ID change.
     */
    if(lcmName_) {
        if(lcmName_[0] != '\0'){
            fprintf(stdout, "Aerolink:: Lcm name is: %s\n", lcmName_);
            result = securityServices_idChangeRegister(
                    secContext_,
                    lcmName_,
                    idChangeData_,  // void* cb data struct - user data
                    initIdChangeCbFn,
                    completeIdChangeCbFn
                    );

            if(result != WS_SUCCESS) {
                std::cerr << "Failed to register the ID change callback: "
                            << ws_errid(result) << std::endl;
                return -1;
            }else{
                fprintf(stdout, "Successful ID change callback registration\n");
            }
        }
    }


    uint8_t const *import_key = (uint8_t*)"";
    SymmetricKeyType symmetricKeyType = SKT_DEK;
    /* Encryption key setup */
    // need to check if encryption is on as well
    // provide option to do either generate symmetric,
    // asymmetric keys or import symmetric
    switch(keyGenMethod_){

    // asymmetric key generation
    case ASYMMETRIC_KEY_GEN:
        result = securityServices_generatePublicEncryptionKeyPair(
                PEA_ECIES_NISTP256, &encryptionKey);
        if (result != WS_SUCCESS){
            if(secVerbosity > 0)
                fprintf(stderr,
                        "Failed to generate public key pair (%s)\n",
                        ws_errid(result));
            return -1;
        }
    break;

    // symmetric key generation
    case SYMMETRIC_KEY_GEN:
        if(secVerbosity > 7)
            fprintf(stdout,"Creating a symmetric encryption key\n");
        result = securityServices_generateSymmetricEncryptionKey(
                SEA_AES128CCM, symmetricKeyType, &encryptionKey);
        if (result != WS_SUCCESS){
            if(secVerbosity > 0)
                fprintf(stderr,
                    "Failed to generate symmetric key (%s)\n",
                    ws_errid(result));
            return -1;
        }
     break;

     // Symmetric key import example
     case IMPORT_SYMMETRIC_KEY:
        result = securityServices_importSymmetricEncryptionKey(
            SEA_AES128CCM, // IN
            symmetricKeyType, // IN
            import_key, // IN
            &encryptionKey); // OUT

        if(result != WS_SUCCESS)
        {
            if(secVerbosity > 0)
                fprintf(stderr,
                        "Failed to import symmetric key (%s)\n",
                            ws_errid(result));
            return -1;
        }
     break;
     default:
        if(secVerbosity > 4)
            fprintf(stderr,
                    "Provided encryption type is invalid: %d\n", keyGenMethod_);
    }
    return 0;
}

// Deinitialize aerolink services when process is finished
void AerolinkSecurity::deinit(void) {
    fprintf(stdout,"Aerolink deinitializing\n");
    if (smg_ != nullptr)
        smg_delete(smg_);
    if(!threadSmps.empty()){
        threadSmps.clear();
    }
    if(!threadSmgs.empty()){
        threadSmgs.clear();
    }
    if(lcmName_)
        securityServices_idChangeUnregister(secContext_,lcmName_);
    (void)sc_close(secContext_);
    (void)securityServices_shutdown();
    if(pInstance != nullptr)
        delete(pInstance);
}

/* UTILITY AND MULTI-THREADING FUNCTIONS */

// Set permissions struct for signing/verifying
static void setPermissions(
    SigningPermissions    *p,
    uint32_t               psid,
    uint8_t const * const  ssp,
    uint8_t const * const  sspMask,
    uint32_t               sspLen)
{
    p->psid = psid;
    p->ssp = ssp;
    p->sspMask = sspMask;
    p->isBitmappedSsp = (NULL == sspMask) ? 0 : 1;
    p->sspLen = sspLen;
}


// Function to create a new SecuredMessageParser object -- for verification
int AerolinkSecurity::createNewSmp(SecuredMessageParserC* smpPtr){
    int result;
    result = smp_new(secContext_, smpPtr);
    if (result != WS_SUCCESS){
        if(secVerbosity > 4)
            fprintf(stderr,"Unable to create secure message parser (0x%08x)\n",
                     ws_errid(result));
        return -1;
    }
    return result;
}

// Function to create a new SecuredMessageGenerator object -- for signing
int AerolinkSecurity::createNewSmg(SecuredMessageGeneratorC* smgPtr){
    int result;
    result = smg_new(secContext_, smgPtr);
    if (result != WS_SUCCESS) {
        if(secVerbosity > 7)
            fprintf(stderr,"Unable to create secure message generator (0x%08x)\n",
                     ws_errid(result));
        return -1;
    }
    return result;
}

// get the smp corresponding to current thread (to avoid multi-threaded issues)
SecuredMessageParserC* AerolinkSecurity::getThrSmp(std::thread::id thrId){
    std::map<std::thread::id, SecuredMessageParserC>::iterator iter =
                threadSmps.find(thrId);
    if(iter == threadSmps.end()){
        if(secVerbosity > 7)
          fprintf(stderr,
                    "Unable to find secure message parser for thread (0x%08x)\n",
                thrId);
        return NULL;
    }
    return &iter->second;
}

// get the smg corresponding to current thread (to avoid multi-threaded issues)
SecuredMessageGeneratorC* AerolinkSecurity::getThrSmg(std::thread::id thrId){
    std::map<std::thread::id, SecuredMessageGeneratorC>::iterator iter =
                threadSmgs.find(thrId);
    if(iter == threadSmgs.end()){
        if(secVerbosity > 7)
            fprintf(stderr,
                    "Unable to find secure message generator for thread (0x%08x)\n",
                thrId);
        return NULL;
    }
    return &iter->second;
}

// get the semaphore according to this thread's smp; for callback management
sem_t* AerolinkSecurity::getThrSmpSem(std::thread::id thrId){
    std::map<std::thread::id, sem_t>::iterator iter =
                verifSmpSems.find(thrId);
    if(iter == verifSmpSems.end()){
        if(secVerbosity > 7)
            fprintf(stderr,"Unable to find SMP semaphore for thread (0x%08x)\n",
                thrId);
        return nullptr;
    }
    return &iter->second;
}

// get the semaphore according to this thread's smg; for callback management
sem_t* AerolinkSecurity::getThrSmgSem(std::thread::id thrId){
    std::map<std::thread::id, sem_t>::iterator iter =
                signSmgSems.find(thrId);
    if(iter == signSmgSems.end()){
        if(secVerbosity > 7)
            fprintf(stderr,"Unable to find SMG semaphore for thread (0x%08x)\n",
                thrId);
        return nullptr;
    }
    return &iter->second;
}


// Function to add new unique SMP corresponding to a thread
bool AerolinkSecurity::addNewThrSmp(std::thread::id thrId){

    // First check if it already exists, so that we do not create mem leak
    if(getThrSmp(thrId) != nullptr){
        if(secVerbosity > 7)
            fprintf(stderr,"Unable to add smp for this thread\n");
        return false;
    }
    sem_wait(&smpListSem);
    SecuredMessageParserC thrSmp;
    // Attempt to create new SMP for this thread and add it
    if(createNewSmp(&thrSmp) < 0){
        if(secVerbosity > 7)
            fprintf(stderr,"Unable to create smp for this thread\n");
        return false;
    }
    // No error in smp creation. Add to <thread,smp> map.
    // This returns pair <iterator, bool>
    threadSmps.insert(std::make_pair(thrId, thrSmp));
    // if there is no assigned sem for this thread, then add one
    sem_t verifThrSem;
    sem_t* verifThrSemPtr;
    if(nullptr == (verifThrSemPtr = getThrSmpSem(thrId))){
        std::pair<std::map<std::thread::id, sem_t>::iterator, bool> ret_pair =
            verifSmpSems.insert(std::make_pair(thrId, verifThrSem));
        if(ret_pair.second){
            verifThrSemPtr = getThrSmpSem(thrId);
            sem_init(verifThrSemPtr, 0, 1);
        }
    }else{
        sem_post(&smpListSem);
        return false;
    }
    sem_post(&smpListSem);
    return true;
}

// Function to create a unique SMG corresponding to a thread
bool AerolinkSecurity::addNewThrSmg(std::thread::id thrId){

    // First check if it already exists, so that we do not create mem leak
    if(getThrSmg(thrId) != nullptr){
        if(secVerbosity > 7)
          fprintf(stderr,"Unable to add smg for this thread\n");
        return false;
    }
    SecuredMessageGeneratorC thrSmg;
    // Attempt to create new SMG for this thread and add it
    if(createNewSmg(&thrSmg) < 0){
        if(secVerbosity > 7)
          fprintf(stderr,"Unable to create new smg\n");
        return false;
    }
    // No error in smg creation. Add to <thread,smg> map.
    // This returns pair <iterator,bool>
    threadSmgs.insert(std::make_pair(thrId, thrSmg));
    // if there is no assigned sem for this thread, then add one
    sem_t signThrSem;
    sem_t* signThrSemPtr;
    if(nullptr == (signThrSemPtr = getThrSmgSem(thrId))){
        std::pair<std::map<std::thread::id, sem_t>::iterator, bool> ret_pair =
            signSmgSems.insert(std::make_pair(thrId, signThrSem));
        if(ret_pair.second){
            signThrSemPtr = getThrSmgSem(thrId);
            sem_init(signThrSemPtr, 0, 1);
        }
    }else{
        return false;
    }
    return true;
}


/* VERIFICATION FUNCTIONS */

// Example async verify callback not meant to reflect realistic callback fn
void AerolinkSecurity::verifyCallback(AEROLINK_RESULT returnCode,
                                        void *userCallbackData)
{
    /* This code needs to be replaced with the users desired actions.
     * A real application will have to be concerned with locking issues that
     * are not being addressed in this sample.
     * A real application may want to use a condition variable to signal that
     * the callback has occurred.
     */
    verifyCallbackStatus = returnCode;
    verifyCallbackUserData = userCallbackData;
    verifyCallbackCalled = 1;
}

// More realistic callback function for async verification
static void handle_verify_result(
    AEROLINK_RESULT returnCode,
    void           *userData)
{
    sem_t* cb_sem = (sem_t*) userData;
    if(returnCode != WS_SUCCESS){
        if(secVerbosity > 4)
                fprintf(stderr,
                "\n------------Unable to verify signature: (%s)------------\n\n",
                 ws_errid(returnCode));
        verifFail++;
    }else{
        if(secVerbosity > 4)
                fprintf(stdout,
                "\n------------Successful verification! (%s)-------------\n\n",
                ws_errid(returnCode));
        verifSuccess++;
    }

    if(cb_sem == nullptr){
        fprintf(stderr,"Callback data was not properly set\n");
        return;
    }

    // print related verification stats per 2500
    sem_wait(&verifLogSem);
    printVerifStats(std::this_thread::get_id());
    sem_post(&verifLogSem);

    sem_post(cb_sem);
}

void print_exception(std::exception& e){
    fprintf(stderr, "Exception caught : %s\n", e.what());
}

int AerolinkSecurity::ExtractMsg(const SecurityOpt opt,
                const uint8_t * msg,
                uint32_t msgLen,
                uint8_t const *payload,
                uint32_t       payloadLen,
                uint32_t       &dot2HdrLen){

    // Add new smp (if none exists) for this thread
    std::thread::id thrId = std::this_thread::get_id();
    try{
        addNewThrSmp(thrId);
    }
    catch (std::exception& e)
    {
        if(secVerbosity > 4){
            print_exception(e);
        }
        return -1;
    }
    AEROLINK_RESULT result;

    // Get corresponding smp for this thread
    SecuredMessageParserC* smp;
    smp = getThrSmp(thrId);
    if(smp == nullptr){
        if(secVerbosity > 4)
            fprintf(stderr,"Unable to retreive smp for this thread %d\n",
              std::this_thread::get_id());
        return -1;
    }

    // smp_extract
    PayloadType    spduType, payloadType;
    uint8_t const *externData;
    ExternalDataHashAlg edhAlg;
    result = smp_extract(
        *smp, msg, msgLen,
        &spduType, &payload, &payloadLen, &payloadType,
        &externData, &edhAlg);
    if (result != WS_SUCCESS)
    {
        if(secVerbosity > 4)
            fprintf(stderr,"Unable to extract message (%s)\n", ws_errid(result));
        return -1;
    }
    dot2HdrLen = (uint32_t)(payload - msg);
    return 0;
}


// A function to verify a signed packet that can handle multi-threading:
//   smp_extract
//   smp_checkRelevance
//   smp_checkConsistency
//   smp_verifySignaturesAsync
int AerolinkSecurity::syncVerify(
    Kinematics hvKine, Kinematics rvKine,
    VerifStats  *verifStat, MisbehaviorStats* misbehaviorStat
    )
{
    // Add new smp (if none exists) for this thread
    std::thread::id thrId = std::this_thread::get_id();
    try{
        addNewThrSmp(thrId);
    }
    catch (std::exception& e)
    {
        if(secVerbosity > 4){
            print_exception(e);
        }
        return -1;
    }
    AEROLINK_RESULT result;

    // Get corresponding smp for this thread
    SecuredMessageParserC* smp;
    smp = getThrSmp(thrId);
    if(smp == nullptr){
        if(secVerbosity > 4)
            fprintf(stderr,"Unable to retreive smp for this thread %d\n",
              std::this_thread::get_id());
        return -1;
    }

    // set the generation location
    if(secVerbosity > 7){
        fprintf(stdout, "HV Latitude, HV Longitude, HV Elevation: %i, %i, %hu\n",
            hvKine.latitude, hvKine.longitude, hvKine.elevation);

        fprintf(stdout, "RV Latitude, RV Longitude, RV Elevation: %i, %i, %hu\n",
            rvKine.latitude, rvKine.longitude, rvKine.elevation);
    }

    // set the generation position based on provided kinematics (if any)
    result = smp_setGenerationLocation(*smp, rvKine.latitude, rvKine.longitude,
            rvKine.elevation);
    if (result != WS_SUCCESS)
    {
        if(secVerbosity > 4)
            fprintf(stderr,"Unable to set generation location (%s)\n", ws_errid(result));
        return -1;
    }

    // smp_checkRelevance
    result = smp_checkRelevance(*smp);
    if (result != WS_SUCCESS)
    {
        if(secVerbosity > 4)
            fprintf(stderr,"Unable to check relevance (%s)\n", ws_errid(result));
        return -1;
    }

    // smp_checkConsistency
    result = smp_checkConsistency(*smp);
    if(result != WS_SUCCESS){
        if(secVerbosity > 4)
            fprintf(stderr,"Unable to check consistency (%s)\n", ws_errid(result));
        return -1;
    }

    // smp_verifySignatures
    //startLatencyTime
    gettimeofday(&currTime, NULL);
    double startLatencyTime =
            (currTime.tv_sec * 1000.0) + (currTime.tv_usec/1000.0);
    result = smp_verifySignatures(*smp);
    gettimeofday(&currTime, NULL);
    double endLatencyTime =
            (currTime.tv_sec * 1000.0) + (currTime.tv_usec/1000.0);
    if (result != WS_SUCCESS)
    {
        if(secVerbosity > 4)
            fprintf(stderr,"Unable to call verify signature function (%s)\n",
                     ws_errid(result));
        verifFail++;
        return -1;
    }else{

        // add latency stats struct to the vector
        if(verifStat != nullptr){
            verifStat->timestamp = endLatencyTime-startTime;
            verifStat->verifLatency = endLatencyTime-startLatencyTime;
        }
        //Misbehavior detection if enabled
        if(this->enableMisbehavior){
            mbdCheck(&rvKine, misbehaviorStat);
        }

        // track overall security performance
        // includes ITS
        if(secVerbosity > 0){
            sem_wait(&verifLogSem);
            verifSuccess++;
            // print relevant verification performance stats
            printVerifStats(thrId);
            sem_post(&verifLogSem);
        }
   }
    return 1;
}

// A function to verify a signed packet that can handle multi-threading:
//   smp_extract
//   smp_checkRelevance
//   smp_checkConsistency
//   smp_verifySignaturesAsync
int AerolinkSecurity::asyncVerify(
    Kinematics hvKine, Kinematics rvKine,
    sem_t    *queue_sem, MisbehaviorStats* misbehaviorStat) {

    // Add new smp (if none exists) for this thread
    AEROLINK_RESULT result;
    std::thread::id thrId = std::this_thread::get_id();
    addNewThrSmp(thrId);
    sem_t* thrVerifSemPtr = getThrSmpSem(thrId);

    // Get corresponding smp for this thread
    SecuredMessageParserC* smp;
    smp = getThrSmp(thrId);
    if(smp == nullptr){
        if(secVerbosity > 4)
        fprintf(stderr,"Unable to retrieve SMP for this thread 0x%08x\n",
              std::this_thread::get_id());
        return -1;
    }

    // set the generation location
    if(secVerbosity > 7){
        fprintf(stdout, "HV Latitude, HV Longitude, HV Elevation: %i, %i, %hu\n",
            hvKine.latitude, hvKine.longitude, hvKine.elevation);

        fprintf(stdout, "RV Latitude, RV Longitude, RV Elevation: %i, %i, %hu\n",
            rvKine.latitude, rvKine.longitude, rvKine.elevation);
    }

    result = smp_setGenerationLocation(*smp, rvKine.latitude, rvKine.longitude,
            rvKine.elevation);
    if (result != WS_SUCCESS)
    {
        if(secVerbosity > 4)
            fprintf(stderr,"Unable to set the generation location (%s)\n", ws_errid(result));
        return -1;
    }

    // smp_checkConsistency
    result = smp_checkConsistency(*smp);
    if(result != WS_SUCCESS){
        if(secVerbosity > 4)
            fprintf(stderr,"Unable to check consistency (%s)\n", ws_errid(result));
        return -1;
    }

    // smp_checkRelevance
    result = smp_checkRelevance(*smp);
    if (result != WS_SUCCESS)
    {
        if(secVerbosity > 4)
            fprintf(stderr,"Unable to check relevance (%s)\n", ws_errid(result));
        return -1;
    }
    // not TRULY async verification
    // smp_verifySignaturesAsync
    result = smp_verifySignaturesAsync
                    (*smp, thrVerifSemPtr, handle_verify_result);
    if (result != WS_SUCCESS)
    {
        if(secVerbosity > 4)
            fprintf(stderr,"Unable to call verify signature function (%s)\n",
                     ws_errid(result));
        return -1;
    }
    //Misbehavior detection if enabled
    if(this->enableMisbehavior){
        mbdCheck(&rvKine, misbehaviorStat);
    }
    sem_wait(thrVerifSemPtr);
    return 1;
}

void AerolinkSecurity:: mbdCheck(Kinematics* rvBsmInfo, MisbehaviorStats* misbehaviorStat) {
    AEROLINK_RESULT result;
    std::thread::id thrId = std::this_thread::get_id();
    SecuredMessageParserC* smp;
    smp = getThrSmp(thrId);
    if (misbehaviorAppDataPtr == nullptr){
        misbehaviorAppDataPtr = std::make_shared<BsmData_t>();
    }
    if (misbehaviorResultPtr == nullptr){
        misbehaviorResultPtr = std::make_shared<MisbehaviorDetectedType_t>();
    }
    if(smp != nullptr){
        fillBsmDataForMbd(rvBsmInfo);
        gettimeofday(&currTime, NULL);
        double startLatencyTime = (currTime.tv_sec * 1000.0) + (currTime.tv_usec/1000.0);
        result = smp_checkMisbehavior(*smp, static_cast<void*>(misbehaviorAppDataPtr.get()),
                            static_cast<MisbehaviorDetectedType_t*>(misbehaviorResultPtr.get()));
        gettimeofday(&currTime, NULL);
        double endLatencyTime = (currTime.tv_sec * 1000.0) + (currTime.tv_usec/1000.0);

        if (result != WS_SUCCESS && result != WS_ERR_MISBEHAVIOR_DETECTED){
            if(secVerbosity > 4)
                fprintf(stderr, "Error in checking misbehavior\n");
        }else{
            if(secVerbosity > 4){
                fprintf(stdout, "Detected Misbehavior Class is 0x%08x\n",
                misbehaviorResultPtr->detectedMisbehaviorClass);
            }
        }
        if(misbehaviorStat != nullptr){
            misbehaviorStat->timestamp = endLatencyTime-startTime;
            misbehaviorStat->misbehaviorLatency = endLatencyTime-startLatencyTime;
        }
    }
}

void AerolinkSecurity::fillBsmDataForMbd(Kinematics* rvBsmData) {
    misbehaviorAppDataPtr->version = 1;
    misbehaviorAppDataPtr->dataType = rvBsmData->dataType;
    misbehaviorAppDataPtr->id =  rvBsmData->id;
    misbehaviorAppDataPtr->msgCount = rvBsmData->msgCount;
    misbehaviorAppDataPtr->latitude = rvBsmData->latitude;
    misbehaviorAppDataPtr->longitude = rvBsmData->longitude;
    misbehaviorAppDataPtr->elevation =  rvBsmData->elevation;
    misbehaviorAppDataPtr->speed =  rvBsmData->speed;
    misbehaviorAppDataPtr->longitudeAcceleration =  rvBsmData->longitudeAcceleration;
    misbehaviorAppDataPtr->heading =  rvBsmData->heading;
    misbehaviorAppDataPtr->latitudeAcceleration =  rvBsmData->latitudeAcceleration;
    misbehaviorAppDataPtr->yawAcceleration =  rvBsmData->yawAcceleration;
    misbehaviorAppDataPtr->brakes = rvBsmData->brakes;
}

// Verifies a signed message and returns payload length of actual packet
int AerolinkSecurity::VerifyMsg(const SecurityOpt opt) {
    setSecVerbosity(opt.secVerbosity);
    this->enableMisbehavior = opt.enableMbd;
    int ret = 0;
    if(opt.enableAsync){
        // Asynchronous Verification
        ret = asyncVerify(
                opt.hvKine, opt.rvKine,
                &verifQueueSem, opt.misbehaviorStat);
    }else{
        //Synchronous Verification
        ret = syncVerify(
                opt.hvKine, opt.rvKine,
                opt.verifStat, opt.misbehaviorStat
              );
    }
    return ret;
}

/* SIGNING FUNCTIONS */
// Simple async sign callback example not meant to reflect realistic callback fn
void AerolinkSecurity::signCallback(
                AEROLINK_RESULT returnCode, void *userCallbackData,
                uint8_t* cbSignedSpduData, uint32_t cbSignedSpduDataLen)
{
    /* This code needs to be replaced with the user's desired actions.
     * A real application will have to be concerned with locking issues that
     * are not being addressed in this sample.
     * A real application may want to use a condition variable to signal that
     * the callback has occurred.
     */
    signCallbackStatus = returnCode;
    signCallbackUserData = userCallbackData;
    signCallbackData = cbSignedSpduData;
    signCallbackDataLen = cbSignedSpduDataLen;
    signCallbackCalled = 1;
}

// Sign and return signed message
int AerolinkSecurity::SignMsg(const SecurityOpt opt,
                             const uint8_t *msg, uint32_t msgLen,
                             uint8_t *signedSpdu, uint32_t &signedSpduLen) {
    setSecVerbosity(opt.secVerbosity);
    AEROLINK_RESULT result;

    // Add new smg (if none exists) for this thread
    std::thread::id thrId = std::this_thread::get_id();
    try{
        addNewThrSmg(thrId);
    }
    catch (std::exception& e)
    {
        if(secVerbosity > 4){
            print_exception(e);
        }
        return -1;
    }
    // Get corresponding smp for this thread
    SecuredMessageGeneratorC* smg;
    smg = getThrSmg(thrId);
    if(smg == nullptr){
        if(secVerbosity > 7)
            fprintf(stderr,
                "Unable to retreive smg for this thread %d\n",
                std::this_thread::get_id());
        return -1;
    }

    uint32_t Slen = signedSpduLen;

    if(opt.hvKine.latitude != 0 && opt.hvKine.longitude != 0 &&
                opt.hvKine.elevation != 0 && countryCode_ != 0){
        if(secVerbosity > 7){
            fprintf(stdout, "HV Latitude, HV Longitude, HV Elevation: %i, %i, %hu\n",
                opt.hvKine.latitude, opt.hvKine.longitude, opt.hvKine.elevation);
        }
        result = securityServices_setCurrentLocation(
                 opt.hvKine.latitude, opt.hvKine.longitude,
                 opt.hvKine.elevation, countryCode_);
        if (result != WS_SUCCESS) {
            fprintf(stderr, "Failed to set current location (%s)\n",
                    ws_errid(result));
            return -1;
        }
    }

    // Set signing permissions.
    uint32_t  ieeePsidValue = 0x20;
    uint8_t   ieeeSsp [32] = {0};
    uint32_t  ieeeSspLength = 0;
    uint8_t   ieeeSspMask [32] = {0};
    uint32_t  ieeeSspMaskLength = 0;
    uint32_t  psidValue = opt.psidValue;
    uint8_t   sspValue [32] = {0};
    uint32_t  sspLength = 0;
    uint8_t   sspMaskValue[32] = {0};
    uint32_t  sspMaskLength = 0;

    const AerolinkEncryptionKey* pubEncryptKey = NULL;

    if(opt.enableEnc && keyGenMethod_ == ASYMMETRIC_KEY_GEN ){
        pubEncryptKey = &encryptionKey;
    }

    psidValue = (opt.psidValue > 0) ? opt.psidValue : ieeePsidValue;
    if (opt.sspLength > 0)
    {
        sspLength = opt.sspLength;
        memcpy(sspValue, opt.sspValue, opt.sspLength);
    }
    else
    {
        sspLength = ieeeSspLength;
        memcpy(sspValue, ieeeSsp, ieeeSspLength);
    }

    if (opt.sspMaskLength > 0)
    {
        sspMaskLength = opt.sspMaskLength;
        memcpy(sspMaskValue, opt.sspMaskValue, opt.sspMaskLength);
    }
    else
    {
        sspMaskLength = ieeeSspMaskLength;
        memcpy(sspMaskValue, ieeeSspMask, ieeeSspMaskLength);
    }

   if(secVerbosity > 7){
        fprintf(stdout,"\nSigning permissions: psid = 0x%02x\n", psidValue);
        if(sspLength>0){
            printf("SSP Length: %d\n", sspLength);
            printf("SSP = ");
            for (size_t j = 0; j < sspLength; ++j)
            {
                fprintf(stdout,"%02x", opt.sspValue[j]);
            }
            std::cout << std::endl;
        }
        if(sspMaskLength>0)
        {
            printf("SSP Mask Length: %d\n", sspMaskLength);
            printf("SSPMask = ");
            for (size_t k = 0; k < sspMaskLength; ++k)
            {
                fprintf(stdout,"%02x", opt.sspMaskValue[k]);
            }
            std::cout << std::endl;
        }
    }


    SigningPermissions permissions;
    setPermissions(&permissions, psidValue, (sspLength > 0) ? sspValue : NULL,
                    (sspMaskLength > 0) ? sspMaskValue : NULL, sspLength);
    double startLatencyTime = 0.0;
    double endLatencyTime = 0.0;
    if(!opt.enableAsync){
        //synchronous signing
        gettimeofday(&currTime, NULL);
        startLatencyTime = (currTime.tv_sec * 1000.0) + (currTime.tv_usec/1000.0);
        result = smg_sign(*smg,
            permissions,
            STO_AUTO,
            0,
            msg,
            msgLen,
            0,
            opt.externalDataHash,
            EDHA_NONE,
            pubEncryptKey, // for receiver to reply with encrypted msg
            signedSpdu,
            &Slen);
        gettimeofday(&currTime, NULL);
        endLatencyTime = (currTime.tv_sec * 1000.0) + (currTime.tv_usec/1000.0);
        // Check if the sign function successfully ran
        if (result != WS_SUCCESS) {
                fprintf(stderr, "Failed to sign the message, error=%d(%s)\n",
                result, ws_errid(result));
            if(secVerbosity>0)
                signFail++;
            return -1;
        }

    }else{
        // optionally, can keep this in SecurityService class
        gettimeofday(&currTime, NULL);
        startLatencyTime = (currTime.tv_sec * 1000.0) + (currTime.tv_usec/1000.0);
        result = smg_signAsync(*smg,
            permissions,
            STO_AUTO,
            0,
            msg,
            msgLen,
            0, // set to 1 for encryption
            opt.externalDataHash,
            EDHA_NONE,
            pubEncryptKey, // encryption will be implemented in future
            signedSpdu,
            Slen,
            nullptr,
            signCallback);
        gettimeofday(&currTime, NULL);
        endLatencyTime = (currTime.tv_sec * 1000.0) + (currTime.tv_usec/1000.0);
        // Now wait for the last signing to finish
        if (result != WS_SUCCESS) {
                fprintf(stderr, "Failed to sign the message, error=%d(%s)\n",
                result, ws_errid(result));
            if(secVerbosity>0)
                signFail++;
            return -1;
        }

        /* At this point your application will want to do other processing while
        * waiting for the callback to be fired.
        */
        while (!signCallbackCalled)
        {
            usleep(10);
        }
        signCallbackCalled = 0;
        if (signCallbackStatus != WS_SUCCESS)
        {
            fprintf(stderr,"Failed to sign the message, error=%d(%s)\n",
                signCallbackStatus, ws_errid(signCallbackStatus));
            if(secVerbosity>0)
                signFail++;
            return -1;
        }

    }

    // add latency stats struct to the vector
    if(opt.signStat != nullptr){
        opt.signStat->timestamp = endLatencyTime-startTime;
        opt.signStat->signLatency = (double)(endLatencyTime-startLatencyTime);
    }

    // track overall security performance
    // includes ITS
    if(secVerbosity>0){
        sem_wait(&signLogSem);
        signSuccess++;
        if(secVerbosity > 7)
            fprintf(stdout,"Signing successful\n");
        printSignStats(thrId);
        sem_post(&signLogSem);
    }

    if(!opt.enableAsync){
        signedSpduLen = Slen;
    }else{
        signedSpduLen = signCallbackDataLen;
    }

    // TODO: No encryption yet.
    // Check if encryption is on
    // Run the encryption function over the signed packet
    if(opt.enableEnc){
        //encryptMsg();
    }
    return 0;
}


/* ENCRYPTION FUNCTIONS */

//AerolinkEncryptionKey const * const recipients_[], uint32_t numRecipients_,
int AerolinkSecurity::encryptMsg(
                uint8_t const * const plainText, uint32_t plainTextLength,
                uint8_t isPayloadSpdu, uint8_t * const encryptedData,
                uint32_t * const encryptedDataLength){
    // Check if there are actually any recipients
    if(numRecipients_==0 || recipients_.empty()){
        if(secVerbosity > 0)
            fprintf(stderr,
               "Number of recipients if zero or they are invalid\n");
        return -1;
    }

    // Generate encrypted data with given key; depending on the generated key type
    // the security services will use the appropriate key enc mechanism.
    int result;
    AerolinkEncryptionKey const * recipientsArr[recipients_.size()];
    std::copy(recipients_.begin(), recipients_.end(), recipientsArr);
    result = smg_encrypt(smg_, recipientsArr, numRecipients_,
                  plainText, plainTextLength, 1, encryptedData, encryptedDataLength);
    if (result != WS_SUCCESS)
    {
        if(secVerbosity > 0)
            fprintf(stderr,
                "Unable to encrypt message (%s)\n",
                ws_errid(result));
        recipients_.clear();
        return -1;
    }
    if(secVerbosity > 7)
        fprintf(stdout,"Encrypted data generated correctly\n");

    // delete keys after sent?
    recipients_.clear();
    return result;
}

// TODO: Decrypt encrypted message
int AerolinkSecurity::decryptMsg(){
    return 0;
}

