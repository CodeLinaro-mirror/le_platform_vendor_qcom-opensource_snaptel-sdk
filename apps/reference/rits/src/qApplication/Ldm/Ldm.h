/*
 *  Copyright (c) 2018-2020, The Linux Foundation. All rights reserved.
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
  * @file: Ldm.h
  *
  * @brief: Api for Local Dynamic Map (Ldm) of the ITS stack.
  *
  */
#ifndef __LDM_H__
#define __LDM_H__
#include <map>
#include <cstdlib>
#include <list>
#include <atomic>
#include <vector>
#include <thread>
#include <mutex>
#include <iostream>
#include <algorithm>
#include "v2x_codec.h"
#include "bsm_utils.h"
#include <telux/cv2x/Cv2xRadio.hpp>

#define DIRTY_DATA -2
#define NO_DATA -1

using std::list;
using std::map;
using std::vector;
using std::pair;
using std::thread;
using std::mutex;
using telux::cv2x::TrustedUEInfoList;
using telux::common::ErrorCode;
class Ldm
{

private:
     void cv2xUpdateTrustedUEListCallback(ErrorCode error);

     /**
      * A trustedUeInfoList from the Snaptel SDK
      */
     TrustedUEInfoList tunnelTimingInfoList;

    /**
     * Garbage collection thread.
     */
     thread gbThread;

     /**
      * Garbage collection thread.
      */
     thread trustedThread;

    /**
     * Garbage collection thread.
     */
     bool trustedStarted = false;

     /**
      * Garbage collection thread.
      */
     bool gbStarted = false;

    /**
     * Scans for remote vehicles that can be trusted.
     */
     void trustedScan();

    /**
    * Function that runs on its own thread and allows garbage collection.
    * @param waitTime -An uint16_t that the gbCollector wait to iterate in seconds.
    * @param timeThreshold -An uint8_t that is used to purge old messages.
    * This can be added other parameters later like TTC, heading and speed,
    * so it purges them based on that.
    */
    void gbCollector(const uint16_t waitTime, const uint8_t timeThreshold);

    /**
     * Map that the the RX uses to know whether to use that entry or not.
     * If true, value is alive and shouldn't be written, if false; write value.
     * By collect means setting dirty bit to false.
     */
    list<uint32_t>bsmFreeContents;

    /**
     * Method that returns true if id is trusted or false if not.
     * @param id - an unit32_t variable representing remote vehicle id.
     * @return a bool of true if id is trusted, false if not.
     */
    bool isTrusted(uint32_t id);

    /**
    * Method that returns true if id has an stored decoded bsm in the ldm, else false.
    * id - An uint32_t unique identification of each vehicle in the CV2x.
    * @return bool that decribes whether element is in the ldm or not.
    */
    bool hasBsm(const uint32_t id);

    /**
    * Method that returns true if id has a valid cert, else false.
    * id - An uint32_t unique identification of each vehicle in the CV2x.
    * @return bool that decribes if cert exists as true or false if not.
    */
    bool validCert(uint32_t id);

    /**
     * Map of key temporal_id and value number of packets lost.
    */
    map <uint32_t, int> bsmPacketsLost;

 public:

    /**
     * Mutex for locking critical data i.e. mapping between ids and content.
     */
     mutex sync;

    /**
    * Tunc map... FIX: You won't need this once the codec includes this on encoding and decoding.
    * i.e. as part of the BSM contents
    */
     map<uint32_t, float> tuncs;

    /**
     * Map that holds where is the bsm based on the id.
     * Key is the id, and value is the index in the cache.
     * Note that you can make this strucutre <uint32_t,atomic<int> and
     * avoid locks when read. Just create a wrapper for
     * atomic<int> as it isn't copyable and therefore not able to insert
     * in STL structures.
     */
     map <uint32_t, int> bsmIdMap;

    /**
     * Function that starts a scan of remote vehicles that can be trusted.
     * if thread already started, prints to console.
     */
     void startTrusted();

    /**
     * Vector that stores decoded bsm Contents
     */
     vector<msg_contents> bsmContents;

     /**
      * Takes current information of the LDM and returns a list.
      * @return list<msg_contents> snapshot.
      */
     list<msg_contents> bsmSnapshot();

     /**
      * Takes current information of the LDM and returns a list.
      * @return list<msg_contents> snapshot.
      */
     list<msg_contents> bsmTrustedSnapshot();

     void bsmTrustedSnapshot(list<msg_contents> trusted);

     /**
      * Once Bsms are decoded, this function should run. This will check that the security
      * is on point and that there is nothing why the bsm shouldn't be disregarded.
      * @return true if bsm was filtered, false if it wasn't.
      */
     bool filterBsm(const uint32_t index);

    /**
    * Gets index of temp id if not -1.
    * @param id - An uint32_t unique identification of each car.
    * @return index at which that id is stored or -1.
    */
     int getIndex(const uint32_t id);

    /**
    * Gets index of temp id if not -1.
    * @param id - An uint32_t unique identification of each car.
    * @return index at which that id is stored or -1.
    */
     void setIndex(const uint32_t id, const uint32_t index);

    /**
    * Constructor.
    * size - uin32_t that represent the amount of elements reserved for the LDM.
    * gbTime - uint16_t that gives the amount of seconds the gb collector sleeps for.
    * timeThreshold -uint8_t that gives the amount of seconds in which a bsm is purged.
    */
    Ldm(const uint16_t size);

    /**
    * Get element that is free and ready to decode contents on it.
    * @return index of vector where there is a ready to use space.
    */
    uint32_t getFreeBsm();

    /**
     * Starts garbage collector thread. This garbage collector has 
     * especifically been design to not deallocate memory but to 
     * mark them as available resources so they can be overwritten. 
     * @param gbTime a uint16_t value representing the frequency
     * of the thread execution in seconds.
     * @param timeThreshold a uint8_t value that represents the allowed
     * BSM age to have before purging in seconds.
     */
    void startGb(const uint16_t gbTime, const uint8_t timeThreshold);

    /**
     * Prints current available contents of the LDM
     */
    void printLdmIdMap();

    /*
    * Thresholds for Tunnel Mode Filtering
    */
    uint32_t packeLossThresh = 0;
    uint32_t ageThresh = 0;
    uint32_t distanceThresh = 0;
    uint32_t positionCertaintyThresh = 0;
    uint32_t tuncThresh = 0;
};
#endif
