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
  * @file: Ldm.cpp
  *
  * @brief: Implementation of Ldm.
  *
  */



#include "Ldm.h"
#include "RadioInterface.h"
using std::map;
using std::vector;
using std::pair;
using std::mutex;
using std::lock_guard;
using std::find;
using std::cout;
using std::endl;
using telux::cv2x::TrustedUEInfo;
using telux::cv2x::TrafficCategory;

Ldm::Ldm(const uint16_t size) {
    this->bsmContents.reserve(2 * size);

    for (uint16_t i = 0; i < size; i++)
    {
        msg_contents msg;
        this->bsmContents.push_back(msg);
        this->bsmFreeContents.push_back(i);
    }
}

int Ldm::getIndex(const uint32_t id) {
    lock_guard<mutex> lk(this->sync);
    if (this->hasBsm(id)) {
        return this->bsmIdMap[id];
    }
    else {
        return NO_DATA;
    }
}

void Ldm::setIndex(const uint32_t id, const uint32_t index) {
    const auto i = this->getIndex(id);
    lock_guard<mutex> lk(this->sync);
    if (i != NO_DATA && i != DIRTY_DATA) {
        this->bsmFreeContents.push_back(i);
        this->bsmIdMap[id] = index;
    }
    else {
        if (i == DIRTY_DATA)
        {
            this->bsmIdMap[id] = index;
        }
        else {
            this->bsmIdMap.insert(pair<uint32_t, int>(id, index));
        }
    }
}

uint32_t Ldm::getFreeBsm() {
    lock_guard<mutex> lk(this->sync);
    if (!this->bsmFreeContents.empty()){
        uint32_t index = this->bsmFreeContents.front();
        this->bsmFreeContents.pop_front();
        return index;
    }
    else {
        msg_contents msg;
        this->bsmContents.push_back(msg);
        return this->bsmContents.size() - 1;
    }
}

bool Ldm::hasBsm(const uint32_t id){
    map<uint32_t, int>::iterator iter = this->bsmIdMap.find(id);
    if (iter != this->bsmIdMap.end())
    {
        return true;
    }
    else {
        return false;
    }
}

void Ldm::gbCollector(const uint16_t waitTime, const uint8_t timeThreshold) {

    while (true) {
        cout << "Running LDM Garbage Collector... \n";
        cout << "Current LDM status: \n";
        printLdmIdMap();
        lock_guard<mutex> lk(this->sync);
        for (pair<uint32_t, int> element : this->bsmIdMap) {
            if(hasBsm(element.first) && this->bsmIdMap[element.first] != DIRTY_DATA){
                const auto now = timestamp_now();
                bsm_value_t *bsmp = reinterpret_cast<bsm_value_t *>(this->bsmContents[element.second].j2735_msg);
                const auto dif = now - bsmp->timestamp_ms;
                if (timeThreshold * 10000 < dif) {
                    cout << "Dif: " << dif << endl;
                    this->bsmFreeContents.push_back(element.second);
                    this->bsmIdMap[element.first] = DIRTY_DATA;
                }
            }
        }
        lk.~lock_guard();
        cout << "End of LDM Garbage Collector... \n";
        sleep(waitTime);
    }
}

void Ldm::startGb(const uint16_t gbTime, const uint8_t timeThreshold) {
    auto gbThread = [&](uint16_t waitTime, uint8_t timeTresh) {
                            gbCollector(waitTime, timeTresh);};

    if (!gbStarted) {
        this->gbThread = thread(gbThread, gbTime, timeThreshold);
        gbStarted = true;
    }
    else {
        cout << "Garbage Collector already started.";
    }
}

void Ldm::cv2xUpdateTrustedUEListCallback(ErrorCode error) {
    if (ErrorCode::SUCCESS != error) {
        cout << "Error Updating UE List.\n";
    }
}

void Ldm::trustedScan() {
    lock_guard<mutex> lk(this->sync);
    auto i = 0;
    RadioInterface inter;
    while (true) {
        for (auto info : tunnelTimingInfoList.trustedUEs) {
            //TODO SDK needs timestamp to take away UEs that are too old.
        }
        auto respCb = [&](ErrorCode error) {
                cv2xUpdateTrustedUEListCallback(error);
        };
        auto radio = inter.cv2xRadioManager->getCv2xRadio(TrafficCategory::SAFETY_TYPE);
        assert(Status::SUCCESS == radio->updateTrustedUEList(tunnelTimingInfoList, respCb));
        sleep(5);
    }
}

void Ldm::startTrusted() {
    auto trustedThread = [&]() {
        trustedScan();
    };

    if (!trustedStarted) {
        this->trustedThread = thread(trustedThread);
        trustedStarted = true;
    }
    else {
        cout << "Trust and Malicious list scan already started running.";
    }
}

void Ldm::printLdmIdMap() {
    lock_guard<mutex> lk(this->sync);
    auto i = 0;
    for (pair<uint32_t, int> element : this->bsmIdMap) {
        cout << "Temp Id: " << element.first << " has data in " << element.second <<endl;
        cout << "Summary:\n";
        if (element.second != NO_DATA && element.second != DIRTY_DATA)
        {
            print_summary_RV(&this->bsmContents[element.second]);
            i++;
        }
    }
    cout << "Total unique clean temp ids " << i << endl;
}

bool Ldm::isTrusted(uint32_t id) {
    //TODO Finds if vehicle is trusted by iterating STL vector
    //tunnelTimingInfoList.trustedUEs

    return true;
}

list<msg_contents> Ldm::bsmSnapshot() {
    lock_guard<mutex> lk(this->sync);
    list<msg_contents> snap;
    auto i = 0;
    for (pair<uint32_t, int> element : this->bsmIdMap) {
        if (element.second != NO_DATA && element.second != DIRTY_DATA)
        {
            snap.push_back(this->bsmContents[element.second]);
        }
    }

    return snap;
}

list<msg_contents> Ldm::bsmTrustedSnapshot() {
    lock_guard<mutex> lk(this->sync);
    list<msg_contents> snap;
    auto i = 0;
    for (pair<uint32_t, int> element : this->bsmIdMap) {
        if (element.second != NO_DATA && element.second != DIRTY_DATA)
        {
            if (isTrusted(element.first)) {
                snap.push_back(this->bsmContents[element.second]);
            }
        }
    }

    return snap;
}

bool Ldm::validCert(uint32_t id) {
    //TODO Implement security validation of certs.
    return true;

}

bool Ldm::filterBsm(const uint32_t index) {
    const auto i = index;
    //TODO
    //After bsm has been decoded check security.
    //If is wrong, give index to freeBsm contents and put MAC address in malicious list.
    //if verified, give id to trusted list.
    //Returns true if message has been filtered, false else.
    msg_contents* msg = &this->bsmContents[index];
    bsm_value_t *bsm = reinterpret_cast<bsm_value_t *>(msg->j2735_msg);
    //const auto id = msg->j2735.bsm.id; // FIX: Use L2 instead of temp ID.
    const auto id = bsm->id;
    TrustedUEInfo tunnelInfo;
    const auto begin = tunnelTimingInfoList.maliciousIds.begin();
    const auto end = tunnelTimingInfoList.maliciousIds.end();
    const auto malicious = find(begin,end, id) != end;
    uint32_t age = -1;
    const auto trusted = isTrusted(id);
    if (malicious) {
        return true;
    }

    //populate info with the right data and send it to malicious.
    tunnelInfo.sourceL2Id = id; //FIX: Use L2 instead of temp ID.
    tunnelInfo.timeUncertainty = tuncs[id]; //FIX: Use bsm tunnelInfo when it is fix.
    tunnelInfo.positionConfidenceLevel = 0; //TODO:You can calculate this from BSM data.
    tunnelInfo.propagationDelay = 0; //TODO get that data.

    if (hasBsm(id))
    {
        const auto i = this->bsmIdMap[id]; //Careful with parallelism, you can use a lock to access here.
        if (i != DIRTY_DATA && i != NO_DATA)
        {
            msg_contents* prevMsg = &this->bsmContents[i];
            bsm_value_t *prev_bsm = reinterpret_cast<bsm_value_t *>(this->bsmContents[i].j2735_msg);
            const auto packetDif = bsm->MsgCount - prev_bsm->MsgCount;
            age = prev_bsm->timestamp_ms;
            if (bsm->timestamp_ms == prev_bsm->timestamp_ms) {
                if (trusted) {
                    //TODO remove from trusted list
                }
                tunnelTimingInfoList.maliciousIds.push_back(id);
            }
            // Check for packet loss
            if (packetDif > 1 && packetDif < 127)
            {
                if (bsmPacketsLost.find(id) == bsmPacketsLost.end()) {
                    bsmPacketsLost.insert(pair<uint32_t, int>(id, packetDif));
                }
                else {
                    bsmPacketsLost[id] += packetDif;
                }
            }
        }

    }else {
        age = 0;
    }

    const auto distance = 0;//TODO calcultate 3D distance
    const auto positionCertainty = 0; //TODO calculate position certainty

    //Running Filtering Options
    if (!validCert(id) || (bsmPacketsLost[id] > packeLossThresh) || (age > ageThresh)
        || (distance <distanceThresh) || (positionCertainty > positionCertaintyThresh)
        || (tuncs[id] >tuncThresh)){
        if (trusted) {
            //TODO Remove from trusted
        }
        tunnelTimingInfoList.maliciousIds.push_back(id);
        return true;
    }
    else {
        if (!trusted) {
            //TODO Add to trusted
        }
    }
}
