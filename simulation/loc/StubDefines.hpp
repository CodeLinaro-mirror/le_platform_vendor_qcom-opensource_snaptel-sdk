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

#ifndef STUBDEFINES_HPP
#define STUBDEFINES_HPP

// Location Manager system start values
#define LOC_MANAGER_START_DEFAULT_VALUE 2000
#define LOC_MANAGER_START_MIN_VALUE 100
#define LOC_MANAGER_START_MAX_VALUE 10000

// Location Configurator system start values
#define LOC_CONFIGURATOR_START_DEFAULT_VALUE 2000
#define LOC_CONFIGURATOR_START_MIN_VALUE 500
#define LOC_CONFIGURATOR_START_MAX_VALUE 10000

// Location Dgnss system start values
#define DGNSS_START_DEFAULT_VALUE 2000
#define DGNSS_START_MIN_VALUE 500
#define DGNSS_START_MAX_VALUE 10000

// Command callback delay values
#define CALLBACK_DELAY_DEFAULT_VALUE 100
#define CALLBACK_DELAY_MIN_VALUE 50
#define CALLBACK_DELAY_MAX_VALUE 500

enum class SubSystemType {
    LOCATION_MANAGER,
    LOCATION_CONFIGURATOR,
    DGNSS_MANAGER,
};


#endif
