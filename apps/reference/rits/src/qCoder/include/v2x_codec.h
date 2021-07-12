/*
 * Copyright (c) 2019-2020, The Linux Foundation. All rights reserved.
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
 * 
 * @file v2x_codec.h
 * @purpose top-level asn encode/decode APIs header file.
 */

#ifndef _V2X_CODEC_H_
#define _V2X_CODEC_H_

#include <stdio.h>
#include <inttypes.h>
#include "v2x_msg.h"
#include "asnbuf.h"
#include "wsmp.h"
#include "ieee1609.2.h"
#include "j2735.h"
#include "CAM.h"
#include "DENM.h"
#include "etsi.h"
#include "btp.h"


#ifdef __cplusplus
extern "C"
{
#endif
void set_codec_verbosity(int value);
/**
 * decode_msg top-level codec API, decode message stored in mc->abuf and
 * populate the corresponding data structure in mc.
 *
 * @param[in] mc the message content, mc->abuf contains the buffer to be
 * decoded.
 * @return 0 if whole message is succesfully decoded.
 *         1 if the message is signed/encrypted, need security service before
 *         contituing decode the rest of the message.
 *         -1 failure.
 */
int decode_msg(msg_contents *mc);

/**
 * decode_msg_continue continue decoding of the message after the security
 * service has succesfully verified/decrypted the message.
 * @param[in] mc the message content that has been partially decoded previously
 * by calling decode_msg().
 * @return 0 on success or -1 on failure.
 */
int decode_msg_continue(msg_contents *mc);

/**
 * encode_msg top-level codec API, encode data stored in mc into mc->abuf
 *
 * @param[in] mc the message content
 * @return > 1 whole message is succesfully encoded, returned encoded length.
 *         = 1 requested to sign/encrypt the message, need security service to
 *         perform operation before continuing encode the rest of the message.
 *         < 0 failure.
 * 
 * NOTE: if mc->stackId is ETSI, then mc->etsi_msg_id shall be set by the caller
 * and correspdong data structure(mc->cam or mc->denm) shall be intialized by
 * the caller before calling this function.
 */
int encode_msg(msg_contents *mc);

/**
 * encode_msg_continue continue encoding the message after the security service
 * has succesfully signed/encrypted the message.
 * @param [in] mc the message that has been partially encoded previosuly by
 * calling encode_msg().
 * @return the encoded length on success or -1 on failure.
 */
int encode_msg_continue(msg_contents *mc);
#ifdef __cplusplus
}
#endif

#endif // #ifndef _V2X_CODEC_H_
