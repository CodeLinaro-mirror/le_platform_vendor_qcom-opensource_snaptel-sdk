/*
 * Copyright (c) 2019-2021, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#ifdef __cplusplus
extern "C" {
#endif
#include <v2x_msg.h>
int encode_singleline_fromCSV(char *line, msg_contents *mc, bool bsmLog);
double get_CPU_percentage(uint64_t monotonicTime);
void write_bsm_header(FILE *fp);
void writeToCsv(msg_contents *mc, FILE *fp, bool isTx, uint64_t periodicityMs,
    bool validPkt, uint32_t RVsInRange, uint64_t monotonicTime, uint64_t realworldTime,
    float locPositionDop, uint16_t locNumSvUsed, uint64_t gnssTime, uint8_t cbr);
void print_summary_RV(msg_contents *mc);
int writeGeneralLog(char* tmpLogStr, uint32_t maxSize, bsm_data *bs, FILE *myfp, bool isTx, uint64_t periodicityMs,
    bool validPkt, uint32_t RVsInRange, const char* timeStamp, uint64_t monotonicTime, uint64_t realworldTimeNow,
    float locPositionDop, uint16_t locNumSvUsed, uint64_t gnssTime, uint8_t cbr,
    uint64_t txInterval, uint32_t l2SrcAddr);
double bsmCompute2dDistance(double hvLat, double hvLon, double rvLat, double rvLon);
    long double deg2rad(double deg);
     double rad2deg(long double rad);
#ifdef __cplusplus
}
#endif
