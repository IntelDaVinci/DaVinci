/*
 ----------------------------------------------------------------------------------------
Copyright (c) 2011, The WebRTC project authors. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

  * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.

  * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in
    the documentation and/or other materials provided with the
    distribution.

  * Neither the name of Google nor the names of its contributors may
    be used to endorse or promote products derived from this software
    without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 -----------------------------------------------------------------------------------------
*/

#ifndef _UTILS_H_
#define _UTILS_H_

#include <sys/time.h>
#include <sys/types.h>
#include <utils/Errors.h>

namespace android {

#define DEFAULT_BIT_RATE         10000000
#define DEFAULT_FPS              30
#define DEFAULT_I_FRAME_INTERVAL 1

void ConvertRGB32ToNV12(const uint8_t* rgbframe,
                          uint8_t* yplane,
                          uint8_t* uvplane,
                          int width,
                          int height,
                          int rgbstride,
                          int ystride,
                          int uvstride);

void ConvertRGB32ToI420(const uint8_t* rgbframe,
                          uint8_t* yplane,
                          uint8_t* uplane,
                          uint8_t* vplane,
                          int width,
                          int height,
                          int rgbstride,
                          int ystride,
                          int ustride,
                          int vstride);

status_t parseUint16Value(const char* str, uint16_t* pValue);

void DumpRawData(const char *path, const void *data, int data_len, int append = 0);

uint64_t GetCurrentTimeStamp();

struct timeval TimeStampToTimeval(uint64_t ts);

uint64_t TimevalToTimeStamp(struct timeval tv);
}

#endif

