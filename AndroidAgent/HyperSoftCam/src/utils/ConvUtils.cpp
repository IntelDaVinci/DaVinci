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

#include "utils/ConvUtils.h"

#include <fcntl.h>
#include <stdlib.h>

#include "utils/Logger.h"

namespace android {

#define LABELALIGN ".p2align 2\n"
#define MEMACCESS(offset, base) #offset "(%" #base ")"
#define MEMOPREG(opcode, offset, base, index, scale, reg) \
    #opcode " " #offset "(%" #base ",%" #index "," #scale "),%%" #reg "\n"
#define MEMOPMEM(opcode, reg, offset, base, index, scale) \
    #opcode " %%" #reg ","#offset "(%" #base ",%" #index "," #scale ")\n"
#define IS_ALIGNED(p, a) (!((uintptr_t)(p) & ((a) - 1)))

typedef unsigned char __attribute__((vector_size(16))) uvec8;
typedef signed char __attribute__((vector_size(16))) vec8;

// Constants for RGBA.
static uvec8 kRGBAToY = {
  33, 65, 13, 0, 33, 65, 13, 0, 33, 65, 13, 0, 33, 65, 13, 0
};

static uvec8 kAddY16 = {
  16u, 16u, 16u, 16u, 16u, 16u, 16u, 16u, 16u, 16u, 16u, 16u, 16u, 16u, 16u, 16u
};

static vec8 kRGBAToU = {
  -38, -74, 112, 0, -38, -74, 112, 0, -38, -74, 112, 0, -38, -74, 112, 0
};

static vec8 kRGBAToV = {
  112, -94, -18, 0, 112, -94, -18, 0, 112, -94, -18, 0, 112, -94, -18, 0
};

#if 0
static vec8 kRGBAToUV = {
  -38, -74, 112, 0, 112, -94, -18, 0, -38, -74, 112, 0, 112, -94, -18, 0
};
#endif

static uvec8 kAddUV128 = {
  128u, 128u, 128u, 128u, 128u, 128u, 128u, 128u,
  128u, 128u, 128u, 128u, 128u, 128u, 128u, 128u
};

#ifdef TARGET_ARCH_X86
#pragma message "Current platform is x86"
void RGBAToYRow_SSSE3(const uint8_t* src_rgba, uint8_t* dst_y, int pix) {
  asm volatile (
    "movdqa    %4,%%xmm5                       \n"
    "movdqa    %3,%%xmm4                       \n"
    LABELALIGN
  "1:                                          \n"
    "movdqa    " MEMACCESS(0x00,0) ",%%xmm0    \n"
    "movdqa    " MEMACCESS(0x10,0) ",%%xmm1    \n"
    "movdqa    " MEMACCESS(0x20,0) ",%%xmm2    \n"
    "movdqa    " MEMACCESS(0x30,0) ",%%xmm3    \n"
    "pmaddubsw %%xmm4,%%xmm0                   \n"
    "pmaddubsw %%xmm4,%%xmm1                   \n"
    "pmaddubsw %%xmm4,%%xmm2                   \n"
    "pmaddubsw %%xmm4,%%xmm3                   \n"
    "lea       " MEMACCESS(0x40,0) ",%0        \n"
    "phaddw    %%xmm1,%%xmm0                   \n"
    "phaddw    %%xmm3,%%xmm2                   \n"
    "psrlw     $0x7,%%xmm0                     \n"
    "psrlw     $0x7,%%xmm2                     \n"
    "packuswb  %%xmm2,%%xmm0                   \n"
    "paddb     %%xmm5,%%xmm0                   \n"
    "sub       $0x10,%2                        \n"
    "movdqa    %%xmm0," MEMACCESS(0x00,1) "    \n"
    "lea       " MEMACCESS(0x10,1) ",%1        \n"
    "jg        1b                              \n"
  : "+r"(src_rgba),  // %0
    "+r"(dst_y),     // %1
    "+r"(pix)        // %2
  : "m"(kRGBAToY),   // %3
    "m"(kAddY16)     // %4
  : "memory", "cc", "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
  );
}

/*
  pix - the width in pixel;
 */
void RGBAToYRow_Unaligned_SSSE3(const uint8_t* src_rgba, uint8_t* dst_y, int pix) {
  asm volatile (
    "movdqa    %4,%%xmm5                       \n"
    "movdqa    %3,%%xmm4                       \n"
    LABELALIGN
  "1:                                          \n"
    "movdqu    " MEMACCESS(0x00,0) ",%%xmm0    \n"
    "movdqu    " MEMACCESS(0x10,0) ",%%xmm1    \n"
    "movdqu    " MEMACCESS(0x20,0) ",%%xmm2    \n"
    "movdqu    " MEMACCESS(0x30,0) ",%%xmm3    \n"
    "pmaddubsw %%xmm4,%%xmm0                   \n"
    "pmaddubsw %%xmm4,%%xmm1                   \n"
    "pmaddubsw %%xmm4,%%xmm2                   \n"
    "pmaddubsw %%xmm4,%%xmm3                   \n"
    "lea       " MEMACCESS(0x40,0) ",%0        \n"
    "phaddw    %%xmm1,%%xmm0                   \n"
    "phaddw    %%xmm3,%%xmm2                   \n"
    "psrlw     $0x7,%%xmm0                     \n"
    "psrlw     $0x7,%%xmm2                     \n"
    "packuswb  %%xmm2,%%xmm0                   \n"
    "paddb     %%xmm5,%%xmm0                   \n"
    "sub       $0x10,%2                        \n"
    "movdqu    %%xmm0," MEMACCESS(0x00,1) "    \n"
    "lea       " MEMACCESS(0x10,1) ",%1        \n"
    "jg        1b                              \n"
  : "+r"(src_rgba),  // %0
    "+r"(dst_y),     // %1
    "+r"(pix)        // %2
  : "m"(kRGBAToY),   // %3
    "m"(kAddY16)     // %4
  : "memory", "cc", "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
  );
}

#if 0
void RGBAToUVRow_SSSE3(const uint8_t* src_rgba, int src_stride_rgba,
                       uint8_t* dst_uv, int width) {
  asm volatile (
    "movdqa    %0,%%xmm4                       \n"
    "movdqa    %1,%%xmm5                       \n"
  :
  : "m"(kRGBAToUV),        // %0
    "m"(kAddUV128)         // %1
  );
  asm volatile (
    LABELALIGN
  "1:                                          \n"
    "movdqa    " MEMACCESS(0x00,0) ",%%xmm0    \n"
    "movdqa    " MEMACCESS(0x10,0) ",%%xmm1    \n"
    "movdqa    " MEMACCESS(0x20,0) ",%%xmm2    \n"
    "movdqa    " MEMACCESS(0x30,0) ",%%xmm6    \n"
    MEMOPREG(pavgb,0x00,0,3,1,xmm0)            //  pavgb   (%0,%4,1),%%xmm0
    MEMOPREG(pavgb,0x10,0,3,1,xmm1)            //  pavgb   0x10(%0,%4,1),%%xmm1
    MEMOPREG(pavgb,0x20,0,3,1,xmm2)            //  pavgb   0x20(%0,%4,1),%%xmm2
    MEMOPREG(pavgb,0x30,0,3,1,xmm6)            //  pavgb   0x30(%0,%4,1),%%xmm6
    "lea       " MEMACCESS(0x40,0) ",%0        \n"
    "movdqa    %%xmm0,%%xmm7                   \n"
    "shufps    $0x88,%%xmm1,%%xmm0             \n"
    "shufps    $0xdd,%%xmm1,%%xmm7             \n"
    "pavgb     %%xmm7,%%xmm0                   \n"
    "movdqa    %%xmm2,%%xmm7                   \n"
    "shufps    $0x88,%%xmm6,%%xmm2             \n"
    "shufps    $0xdd,%%xmm6,%%xmm7             \n"
    "pavgb     %%xmm7,%%xmm2                   \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "movdqa    %%xmm2,%%xmm6                   \n"
    "movdqa    %%xmm0,%%xmm7                   \n"
    "punpckldq %%xmm7,%%xmm0                   \n"
    "punpckhdq %%xmm7,%%xmm1                   \n"
    "movdqa    %%xmm2,%%xmm7                   \n"
    "punpckldq %%xmm7,%%xmm2                   \n"
    "punpckhdq %%xmm7,%%xmm6                   \n"
    "pmaddubsw %%xmm4,%%xmm0                   \n"
    "pmaddubsw %%xmm4,%%xmm1                   \n"
    "pmaddubsw %%xmm4,%%xmm2                   \n"
    "pmaddubsw %%xmm4,%%xmm6                   \n"
    "phaddw    %%xmm1,%%xmm0                   \n"
    "phaddw    %%xmm6,%%xmm2                   \n"
    "psraw     $0x8,%%xmm0                     \n"
    "psraw     $0x8,%%xmm2                     \n"
    "packsswb  %%xmm2,%%xmm0                   \n"
    "paddb     %%xmm5,%%xmm0                   \n"
    "sub       $0x10,%2                        \n"
    "movdqa    %%xmm0," MEMACCESS(0x00,1) "    \n"
    "lea       " MEMACCESS(0x10,1) ",%1        \n"
    "jg        1b                              \n"
  : "+r"(src_rgba),        // %0
    "+r"(dst_uv),          // %1
    "+rm"(width)           // %2
  : "r"((intptr_t)(src_stride_rgba)) // %3
  : "memory", "cc", "xmm0", "xmm1", "xmm2", "xmm6", "xmm7"
  );
}

void RGBAToUVRow_Unaligned_SSSE3(const uint8_t* src_rgba, int src_stride_rgba,
                                 uint8_t* dst_uv, int width) {
  asm volatile (
    "movdqa    %0,%%xmm4                       \n"
    "movdqa    %1,%%xmm5                       \n"
  :
  : "m"(kRGBAToUV),        // %0
    "m"(kAddUV128)         // %1
  );
  asm volatile (
    LABELALIGN
  "1:                                          \n"
    "movdqu    " MEMACCESS(0x00,0) ",%%xmm0    \n"
    "movdqu    " MEMACCESS(0x10,0) ",%%xmm1    \n"
    "movdqu    " MEMACCESS(0x20,0) ",%%xmm2    \n"
    "movdqu    " MEMACCESS(0x30,0) ",%%xmm6    \n"
    MEMOPREG(movdqu,0x00,0,3,1,xmm7)           //  movdqu  (%0,%4,1),%%xmm7
    "pavgb     %%xmm7,%%xmm0                   \n"
    MEMOPREG(movdqu,0x10,0,3,1,xmm7)           //  movdqu  0x10(%0,%4,1),%%xmm7
    "pavgb     %%xmm7,%%xmm1                   \n"
    MEMOPREG(movdqu,0x20,0,3,1,xmm7)           //  movdqu  0x20(%0,%4,1),%%xmm7
    "pavgb     %%xmm7,%%xmm2                   \n"
    MEMOPREG(movdqu,0x30,0,3,1,xmm7)           //  movdqu  0x30(%0,%4,1),%%xmm7
    "pavgb     %%xmm7,%%xmm6                   \n"
    "lea       " MEMACCESS(0x40,0) ",%0        \n"
    "movdqa    %%xmm0,%%xmm7                   \n"
    "shufps    $0x88,%%xmm1,%%xmm0             \n"
    "shufps    $0xdd,%%xmm1,%%xmm7             \n"
    "pavgb     %%xmm7,%%xmm0                   \n"
    "movdqa    %%xmm2,%%xmm7                   \n"
    "shufps    $0x88,%%xmm6,%%xmm2             \n"
    "shufps    $0xdd,%%xmm6,%%xmm7             \n"
    "pavgb     %%xmm7,%%xmm2                   \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "movdqa    %%xmm2,%%xmm6                   \n"
    "movdqa    %%xmm0,%%xmm7                   \n"
    "punpckldq %%xmm7,%%xmm0                   \n"
    "punpckhdq %%xmm7,%%xmm1                   \n"
    "movdqa    %%xmm2,%%xmm7                   \n"
    "punpckldq %%xmm7,%%xmm2                   \n"
    "punpckhdq %%xmm7,%%xmm6                   \n"
    "pmaddubsw %%xmm4,%%xmm0                   \n"
    "pmaddubsw %%xmm4,%%xmm1                   \n"
    "pmaddubsw %%xmm4,%%xmm2                   \n"
    "pmaddubsw %%xmm4,%%xmm6                   \n"
    "phaddw    %%xmm1,%%xmm0                   \n"
    "phaddw    %%xmm6,%%xmm2                   \n"
    "psraw     $0x8,%%xmm0                     \n"
    "psraw     $0x8,%%xmm2                     \n"
    "packsswb  %%xmm2,%%xmm0                   \n"
    "paddb     %%xmm5,%%xmm0                   \n"
    "sub       $0x10,%2                        \n"
    "movdqa    %%xmm0," MEMACCESS(0x00,1) "    \n"
    "lea       " MEMACCESS(0x10,1) ",%1        \n"
    "jg        1b                              \n"
  : "+r"(src_rgba),        // %0
    "+r"(dst_uv),          // %1
    "+rm"(width)           // %2
  : "r"((intptr_t)(src_stride_rgba)) // %3
  : "memory", "cc", "xmm0", "xmm1", "xmm2", "xmm6", "xmm7"
  );
}
#endif

void RGBAToUVRow_SSSE3_NV12(const uint8_t* src_rgba, int src_stride_rgba,
                       uint8_t* dst_uv, int width) {
  asm volatile (
    "movdqa    %0,%%xmm4                       \n"
    "movdqa    %1,%%xmm3                       \n"
    "movdqa    %2,%%xmm5                       \n"
  :
  : "m"(kRGBAToU),         // %0
    "m"(kRGBAToV),         // %1
    "m"(kAddUV128)         // %2
  );
  asm volatile (
    LABELALIGN
  "1:                                          \n"
    "movdqa    " MEMACCESS(0x00,0) ",%%xmm0    \n"
    "movdqa    " MEMACCESS(0x10,0) ",%%xmm1    \n"
    "movdqa    " MEMACCESS(0x20,0) ",%%xmm2    \n"
    "movdqa    " MEMACCESS(0x30,0) ",%%xmm6    \n"
    MEMOPREG(pavgb,0x00,0,3,1,xmm0)            //  pavgb   (%0,%4,1),%%xmm0
    MEMOPREG(pavgb,0x10,0,3,1,xmm1)            //  pavgb   0x10(%0,%4,1),%%xmm1
    MEMOPREG(pavgb,0x20,0,3,1,xmm2)            //  pavgb   0x20(%0,%4,1),%%xmm2
    MEMOPREG(pavgb,0x30,0,3,1,xmm6)            //  pavgb   0x30(%0,%4,1),%%xmm6
    "lea       " MEMACCESS(0x40,0) ",%0        \n"
    "movdqa    %%xmm0,%%xmm7                   \n"
    "shufps    $0x88,%%xmm1,%%xmm0             \n"
    "shufps    $0xdd,%%xmm1,%%xmm7             \n"
    "pavgb     %%xmm7,%%xmm0                   \n"
    "movdqa    %%xmm2,%%xmm7                   \n"
    "shufps    $0x88,%%xmm6,%%xmm2             \n"
    "shufps    $0xdd,%%xmm6,%%xmm7             \n"
    "pavgb     %%xmm7,%%xmm2                   \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "movdqa    %%xmm2,%%xmm6                   \n"
    "pmaddubsw %%xmm4,%%xmm0                   \n"
    "pmaddubsw %%xmm4,%%xmm2                   \n"
    "pmaddubsw %%xmm3,%%xmm1                   \n"
    "pmaddubsw %%xmm3,%%xmm6                   \n"
    "phaddw    %%xmm2,%%xmm0                   \n"
    "phaddw    %%xmm6,%%xmm1                   \n"
    "psraw     $0x8,%%xmm0                     \n"
    "psraw     $0x8,%%xmm1                     \n"
    "packsswb  %%xmm7,%%xmm0                   \n"
    "packsswb  %%xmm7,%%xmm1                   \n"
    "punpcklbw %%xmm1,%%xmm0                   \n"
    "paddb     %%xmm5,%%xmm0                   \n"
    "sub       $0x10,%2                        \n"
    "movdqa    %%xmm0," MEMACCESS(0x00,1) "    \n"
    "lea       " MEMACCESS(0x10,1) ",%1        \n"
    "jg        1b                              \n"
  : "+r"(src_rgba),        // %0
    "+r"(dst_uv),          // %1
    "+rm"(width)           // %2
  : "r"((intptr_t)(src_stride_rgba)) // %3
  : "memory", "cc", "xmm0", "xmm1", "xmm2", "xmm6", "xmm7"
  );
}

void RGBAToUVRow_Unaligned_SSSE3_NV12(const uint8_t* src_rgba, int src_stride_rgba,
                                 uint8_t* dst_uv, int width) {
  asm volatile (
    "movdqa    %0,%%xmm4                       \n"
    "movdqa    %1,%%xmm3                       \n"
    "movdqa    %2,%%xmm5                       \n"
  :
  : "m"(kRGBAToU),         // %0
    "m"(kRGBAToV),         // %1
    "m"(kAddUV128)         // %2
  );
  asm volatile (
    LABELALIGN
  "1:                                          \n"
    "movdqu    " MEMACCESS(0x00,0) ",%%xmm0    \n"
    "movdqu    " MEMACCESS(0x10,0) ",%%xmm1    \n"
    "movdqu    " MEMACCESS(0x20,0) ",%%xmm2    \n"
    "movdqu    " MEMACCESS(0x30,0) ",%%xmm6    \n"
    MEMOPREG(movdqu,0x00,0,3,1,xmm7)           //  movdqu  (%0,%4,1),%%xmm7
    "pavgb     %%xmm7,%%xmm0                   \n"
    MEMOPREG(movdqu,0x10,0,3,1,xmm7)           //  movdqu  0x10(%0,%4,1),%%xmm7
    "pavgb     %%xmm7,%%xmm1                   \n"
    MEMOPREG(movdqu,0x20,0,3,1,xmm7)           //  movdqu  0x20(%0,%4,1),%%xmm7
    "pavgb     %%xmm7,%%xmm2                   \n"
    MEMOPREG(movdqu,0x30,0,3,1,xmm7)           //  movdqu  0x30(%0,%4,1),%%xmm7
    "pavgb     %%xmm7,%%xmm6                   \n"
    "lea       " MEMACCESS(0x40,0) ",%0        \n"
    "movdqa    %%xmm0,%%xmm7                   \n"
    "shufps    $0x88,%%xmm1,%%xmm0             \n"
    "shufps    $0xdd,%%xmm1,%%xmm7             \n"
    "pavgb     %%xmm7,%%xmm0                   \n"
    "movdqa    %%xmm2,%%xmm7                   \n"
    "shufps    $0x88,%%xmm6,%%xmm2             \n"
    "shufps    $0xdd,%%xmm6,%%xmm7             \n"
    "pavgb     %%xmm7,%%xmm2                   \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "movdqa    %%xmm2,%%xmm6                   \n"
    "pmaddubsw %%xmm4,%%xmm0                   \n"
    "pmaddubsw %%xmm4,%%xmm2                   \n"
    "pmaddubsw %%xmm3,%%xmm1                   \n"
    "pmaddubsw %%xmm3,%%xmm6                   \n"
    "phaddw    %%xmm2,%%xmm0                   \n"
    "phaddw    %%xmm6,%%xmm1                   \n"
    "psraw     $0x8,%%xmm0                     \n"
    "psraw     $0x8,%%xmm1                     \n"
    "packsswb  %%xmm7,%%xmm0                   \n"
    "packsswb  %%xmm7,%%xmm1                   \n"
    "punpcklbw %%xmm1,%%xmm0                   \n"
    "paddb     %%xmm5,%%xmm0                   \n"
    "sub       $0x10,%2                        \n"
    "movdqa    %%xmm0," MEMACCESS(0x00,1) "    \n"
    "lea       " MEMACCESS(0x10,1) ",%1        \n"
    "jg        1b                              \n"
  : "+r"(src_rgba),        // %0
    "+r"(dst_uv),          // %1
    "+rm"(width)           // %2
  : "r"((intptr_t)(src_stride_rgba)) // %3
  : "memory", "cc", "xmm0", "xmm1", "xmm2", "xmm6", "xmm7"
  );
}

void RGBAToUVRow_SSSE3_I420(const uint8_t* src_rgba, int src_stride_rgba,
                       uint8_t* dst_u, uint8_t* dst_v, int width) {
  asm volatile (
    "movdqa    %0,%%xmm4                       \n"
    "movdqa    %1,%%xmm3                       \n"
    "movdqa    %2,%%xmm5                       \n"
  :
  : "m"(kRGBAToU),         // %0
    "m"(kRGBAToV),         // %1
    "m"(kAddUV128)         // %2
  );
  asm volatile (
    "sub       %1,%2                           \n"
    LABELALIGN
  "1:                                          \n"
    "movdqa    " MEMACCESS(0x00,0) ",%%xmm0    \n"
    "movdqa    " MEMACCESS(0x10,0) ",%%xmm1    \n"
    "movdqa    " MEMACCESS(0x20,0) ",%%xmm2    \n"
    "movdqa    " MEMACCESS(0x30,0) ",%%xmm6    \n"
    MEMOPREG(pavgb,0x00,0,4,1,xmm0)            //  pavgb   (%0,%4,1),%%xmm0
    MEMOPREG(pavgb,0x10,0,4,1,xmm1)            //  pavgb   0x10(%0,%4,1),%%xmm1
    MEMOPREG(pavgb,0x20,0,4,1,xmm2)            //  pavgb   0x20(%0,%4,1),%%xmm2
    MEMOPREG(pavgb,0x30,0,4,1,xmm6)            //  pavgb   0x30(%0,%4,1),%%xmm6
    "lea       " MEMACCESS(0x40,0) ",%0        \n"
    "movdqa    %%xmm0,%%xmm7                   \n"
    "shufps    $0x88,%%xmm1,%%xmm0             \n"
    "shufps    $0xdd,%%xmm1,%%xmm7             \n"
    "pavgb     %%xmm7,%%xmm0                   \n"
    "movdqa    %%xmm2,%%xmm7                   \n"
    "shufps    $0x88,%%xmm6,%%xmm2             \n"
    "shufps    $0xdd,%%xmm6,%%xmm7             \n"
    "pavgb     %%xmm7,%%xmm2                   \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "movdqa    %%xmm2,%%xmm6                   \n"
    "pmaddubsw %%xmm4,%%xmm0                   \n"
    "pmaddubsw %%xmm4,%%xmm2                   \n"
    "pmaddubsw %%xmm3,%%xmm1                   \n"
    "pmaddubsw %%xmm3,%%xmm6                   \n"
    "phaddw    %%xmm2,%%xmm0                   \n"
    "phaddw    %%xmm6,%%xmm1                   \n"
    "psraw     $0x8,%%xmm0                     \n"
    "psraw     $0x8,%%xmm1                     \n"
    "packsswb  %%xmm1,%%xmm0                   \n"
    "paddb     %%xmm5,%%xmm0                   \n"
    "sub       $0x10,%3                        \n"
    "movlps    %%xmm0,(%1)                     \n"
    "movhps    %%xmm0,(%1,%2,1)                \n"
    "lea       0x8(%1),%1                      \n"
    "jg        1b                              \n"
  : "+r"(src_rgba),        // %0
    "+r"(dst_u),           // %1
    "+r"(dst_v),           // %2
    "+rm"(width)           // %3
  : "r"((intptr_t)(src_stride_rgba)) // %4
  : "memory", "cc", "xmm0", "xmm1", "xmm2", "xmm6", "xmm7"
  );
}

void RGBAToUVRow_Unaligned_SSSE3_I420(const uint8_t* src_rgba, int src_stride_rgba,
                                 uint8_t* dst_u, uint8_t* dst_v, int width) {
  asm volatile (
    "movdqa    %0,%%xmm4                       \n"
    "movdqa    %1,%%xmm3                       \n"
    "movdqa    %2,%%xmm5                       \n"
  :
  : "m"(kRGBAToU),         // %0
    "m"(kRGBAToV),         // %1
    "m"(kAddUV128)         // %2
  );
  asm volatile (
    "sub       %1,%2                           \n"
    LABELALIGN
  "1:                                          \n"
    "movdqu    " MEMACCESS(0x00,0) ",%%xmm0    \n"
    "movdqu    " MEMACCESS(0x10,0) ",%%xmm1    \n"
    "movdqu    " MEMACCESS(0x20,0) ",%%xmm2    \n"
    "movdqu    " MEMACCESS(0x30,0) ",%%xmm6    \n"
    MEMOPREG(movdqu,0x00,0,4,1,xmm7)           //  movdqu  (%0,%4,1),%%xmm7
    "pavgb     %%xmm7,%%xmm0                   \n"
    MEMOPREG(movdqu,0x10,0,4,1,xmm7)           //  movdqu  0x10(%0,%4,1),%%xmm7
    "pavgb     %%xmm7,%%xmm1                   \n"
    MEMOPREG(movdqu,0x20,0,4,1,xmm7)           //  movdqu  0x20(%0,%4,1),%%xmm7
    "pavgb     %%xmm7,%%xmm2                   \n"
    MEMOPREG(movdqu,0x30,0,4,1,xmm7)           //  movdqu  0x30(%0,%4,1),%%xmm7
    "pavgb     %%xmm7,%%xmm6                   \n"
    "lea       " MEMACCESS(0x40,0) ",%0        \n"
    "movdqa    %%xmm0,%%xmm7                   \n"
    "shufps    $0x88,%%xmm1,%%xmm0             \n"
    "shufps    $0xdd,%%xmm1,%%xmm7             \n"
    "pavgb     %%xmm7,%%xmm0                   \n"
    "movdqa    %%xmm2,%%xmm7                   \n"
    "shufps    $0x88,%%xmm6,%%xmm2             \n"
    "shufps    $0xdd,%%xmm6,%%xmm7             \n"
    "pavgb     %%xmm7,%%xmm2                   \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "movdqa    %%xmm2,%%xmm6                   \n"
    "pmaddubsw %%xmm4,%%xmm0                   \n"
    "pmaddubsw %%xmm4,%%xmm2                   \n"
    "pmaddubsw %%xmm3,%%xmm1                   \n"
    "pmaddubsw %%xmm3,%%xmm6                   \n"
    "phaddw    %%xmm2,%%xmm0                   \n"
    "phaddw    %%xmm6,%%xmm1                   \n"
    "psraw     $0x8,%%xmm0                     \n"
    "psraw     $0x8,%%xmm1                     \n"
    "packsswb  %%xmm1,%%xmm0                   \n"
    "paddb     %%xmm5,%%xmm0                   \n"
    "sub       $0x10,%3                        \n"
    "movlps    %%xmm0,(%1)                     \n"
    "movhps    %%xmm0,(%1,%2,1)                \n"
    "lea       0x8(%1),%1                      \n"
    "jg        1b                              \n"
  : "+r"(src_rgba),        // %0
    "+r"(dst_u),           // %1
    "+r"(dst_v),           // %2
    "+rm"(width)           // %3
  : "r"((intptr_t)(src_stride_rgba)) // %4
  : "memory", "cc", "xmm0", "xmm1", "xmm2", "xmm6", "xmm7"
  );
}

#else
#ifdef TARGET_ARCH_ARMV7ABI
#pragma message "Current platform is arm"

void RGBAToYRow_NEON(const uint8_t* src_rgba, uint8_t* dst_y, int pix) {
  asm volatile (
    "vmov.u8    d4, #13                        \n"  // B * 0.1016 coefficient
    "vmov.u8    d5, #65                        \n"  // G * 0.5078 coefficient
    "vmov.u8    d6, #33                        \n"  // R * 0.2578 coefficient
    "vmov.u8    d7, #16                        \n"  // Add 16 constant
    LABELALIGN
  "1:                                          \n"
    "vld4.8     {d0, d1, d2, d3}, [%0]!        \n"  // load 8 pixels of RGBA.
    "subs       %2, %2, #8                     \n"  // 8 processed per loop.
    "vmull.u8   q8, d0, d4                     \n"  // R
    "vmlal.u8   q8, d1, d5                     \n"  // G
    "vmlal.u8   q8, d2, d6                     \n"  // B
    "vqrshrun.s16 d0, q8, #7                   \n"  // 16 bit to 8 bit Y
    "vqadd.u8   d0, d7                         \n"
    "vst1.8     {d0}, [%1]!                    \n"  // store 8 pixels Y.
    "bgt        1b                             \n"
  : "+r"(src_rgba),  // %0
    "+r"(dst_y),     // %1
    "+r"(pix)        // %2
  :
  : "cc", "memory", "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7", "q8"
  );
}

// 16x2 pixels -> 8x1.  pix is number of argb pixels. e.g. 16.
#define RGBTOUV(QR, QG, QB) \
    "vmul.s16   q8, " #QB ", q10               \n"  /* B                    */ \
    "vmls.s16   q8, " #QG ", q11               \n"  /* G                    */ \
    "vmls.s16   q8, " #QR ", q12               \n"  /* R                    */ \
    "vadd.u16   q8, q8, q15                    \n"  /* +128 -> unsigned     */ \
    "vmul.s16   q9, " #QR ", q10               \n"  /* R                    */ \
    "vmls.s16   q9, " #QG ", q14               \n"  /* G                    */ \
    "vmls.s16   q9, " #QB ", q13               \n"  /* B                    */ \
    "vadd.u16   q9, q9, q15                    \n"  /* +128 -> unsigned     */ \
    "vqshrn.u16  d0, q8, #8                    \n"  /* 16 bit to 8 bit U    */ \
    "vqshrn.u16  d1, q9, #8                    \n"  /* 16 bit to 8 bit V    */


void RGBAToUVRow_NEON_NV12(const uint8_t* src_rgba, int src_stride_rgba,
                      uint8_t* dst_uv, int pix) {
  asm volatile (
    "add        %1, %0, %1                     \n"  // src_stride + src_rgba
    "vmov.s16   q10, #112 / 2                  \n"  // UB / VR 0.875 coefficient
    "vmov.s16   q11, #74 / 2                   \n"  // UG -0.5781 coefficient
    "vmov.s16   q12, #38 / 2                   \n"  // UR -0.2969 coefficient
    "vmov.s16   q13, #18 / 2                   \n"  // VB -0.1406 coefficient
    "vmov.s16   q14, #94 / 2                   \n"  // VG -0.7344 coefficient
    "vmov.u16   q15, #0x8080                   \n"  // 128.5
    LABELALIGN
  "1:                                          \n"
    "vld4.8     {d0, d2, d4, d6}, [%0]!        \n"  // load 8 RGBA pixels. R -> d0, G -> d2, B ->d4, A -> d6
    "vld4.8     {d1, d3, d5, d7}, [%0]!        \n"  // load next 8 RGBA pixels.
    "vpaddl.u8  q0, q0                         \n"  // R 16 bytes -> 8 shorts.
    "vpaddl.u8  q1, q1                         \n"  // G 16 bytes -> 8 shorts.
    "vpaddl.u8  q2, q2                         \n"  // B 16 bytes -> 8 shorts.
    "vld4.8     {d8, d10, d12, d14}, [%1]!     \n"  // load 8 more RGBA pixels.
    "vld4.8     {d9, d11, d13, d15}, [%1]!     \n"  // load last 8 RGBA pixels.
    "vpadal.u8  q0, q4                         \n"  // R 16 bytes -> 8 shorts.
    "vpadal.u8  q1, q5                         \n"  // G 16 bytes -> 8 shorts.
    "vpadal.u8  q2, q6                         \n"  // B 16 bytes -> 8 shorts.

    "vrshr.u16  q0, q0, #1                     \n"  // 2x average
    "vrshr.u16  q1, q1, #1                     \n"
    "vrshr.u16  q2, q2, #1                     \n"

    "subs       %3, %3, #16                    \n"  // 32 processed per loop.
    RGBTOUV(q0, q1, q2)
    "vst2.8     {d0, d1}, [%2]!                \n"  // store 8 pixels U and 8 pixels V.
    "bgt        1b                             \n"
  : "+r"(src_rgba),  // %0
    "+r"(src_stride_rgba),  // %1
    "+r"(dst_uv),    // %2
    "+r"(pix)        // %3
  :
  : "cc", "memory", "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q7",
    "q8", "q9", "q10", "q11", "q12", "q13", "q14", "q15"
  );
}

void RGBAToUVRow_NEON_I420(const uint8_t* src_rgba, int src_stride_rgba,
                      uint8_t* dst_u, uint8_t* dst_v, int pix) {
  asm volatile (
    "add        %1, %0, %1                     \n"  // src_stride + src_rgba
    "vmov.s16   q10, #112 / 2                  \n"  // UB / VR 0.875 coefficient
    "vmov.s16   q11, #74 / 2                   \n"  // UG -0.5781 coefficient
    "vmov.s16   q12, #38 / 2                   \n"  // UR -0.2969 coefficient
    "vmov.s16   q13, #18 / 2                   \n"  // VB -0.1406 coefficient
    "vmov.s16   q14, #94 / 2                   \n"  // VG -0.7344 coefficient
    "vmov.u16   q15, #0x8080                   \n"  // 128.5
    LABELALIGN
  "1:                                          \n"
    "vld4.8     {d0, d2, d4, d6}, [%0]!        \n"  // load 8 RGBA pixels. R -> d0, G -> d2, B ->d4, A -> d6
    "vld4.8     {d1, d3, d5, d7}, [%0]!        \n"  // load next 8 RGBA pixels.
    "vpaddl.u8  q0, q0                         \n"  // R 16 bytes -> 8 shorts.
    "vpaddl.u8  q1, q1                         \n"  // G 16 bytes -> 8 shorts.
    "vpaddl.u8  q2, q2                         \n"  // B 16 bytes -> 8 shorts.
    "vld4.8     {d8, d10, d12, d14}, [%1]!     \n"  // load 8 more RGBA pixels.
    "vld4.8     {d9, d11, d13, d15}, [%1]!     \n"  // load last 8 RGBA pixels.
    "vpadal.u8  q0, q4                         \n"  // R 16 bytes -> 8 shorts.
    "vpadal.u8  q1, q5                         \n"  // G 16 bytes -> 8 shorts.
    "vpadal.u8  q2, q6                         \n"  // B 16 bytes -> 8 shorts.

    "vrshr.u16  q0, q0, #1                     \n"  // 2x average
    "vrshr.u16  q1, q1, #1                     \n"
    "vrshr.u16  q2, q2, #1                     \n"

    "subs       %4, %4, #16                    \n"  // 32 processed per loop.
    RGBTOUV(q0, q1, q2)
    "vst1.8     {d0}, [%2]!                    \n"  // store 8 pixels U .
    "vst1.8     {d1}, [%3]!                    \n"  // store  8 pixels V.
    "bgt        1b                             \n"
  : "+r"(src_rgba),  // %0
    "+r"(src_stride_rgba),  // %1
    "+r"(dst_u),     // %2
    "+r"(dst_v),     // %3
    "+r"(pix)        // %4
  :
  : "cc", "memory", "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q7",
    "q8", "q9", "q10", "q11", "q12", "q13", "q14", "q15"
  );
}
#endif
#endif

#define R 0
#define G 1
#define B 2
#define BPP 4
#define SBPP 1
#define MASK 15

static __inline int RGBToY(uint8_t r, uint8_t g, uint8_t b) {
    return (66 * r + 129 * g +  25 * b + 0x1080) >> 8;
}

static __inline int RGBToU(uint8_t r, uint8_t g, uint8_t b) {
    return (112 * b - 74 * g - 38 * r + 0x8080) >> 8;
}

static __inline int RGBToV(uint8_t r, uint8_t g, uint8_t b) {
    return (112 * r - 94 * g - 18 * b + 0x8080) >> 8;
}

void RGBAToYRow_C(const uint8_t* src_rgba, uint8_t* dst_y, int width) {
    int x = 0;
    for (; x < width; ++x) {
        dst_y[0] = RGBToY(src_rgba[R], src_rgba[G], src_rgba[B]);
        src_rgba += BPP;
        dst_y++;
    }
}
void RGBAToUVRow_C_NV12(const uint8_t* src_rgba, int src_stride_rgba, uint8_t* dst_uv,int width) {
    const uint8_t* src_rgba1 = src_rgba + src_stride_rgba;
    int x = 0;
    for (; x < width - 1; x += 2) {
        uint8_t ab = (src_rgba[B] + src_rgba[B + BPP] +
                src_rgba1[B] + src_rgba1[B + BPP]) >> 2;
        uint8_t ag = (src_rgba[G] + src_rgba[G + BPP] +
                   src_rgba1[G] + src_rgba1[G + BPP]) >> 2;
        uint8_t ar = (src_rgba[R] + src_rgba[R + BPP] +
                   src_rgba1[R] + src_rgba1[R + BPP]) >> 2;
        dst_uv[0] = RGBToU(ar, ag, ab);
        dst_uv[1] = RGBToV(ar, ag, ab);
        src_rgba += BPP * 2;
        src_rgba1 += BPP * 2;
        dst_uv += 2;
    }
    if (width & 1) {
        uint8_t ab = (src_rgba[B] + src_rgba1[B]) >> 1;
        uint8_t ag = (src_rgba[G] + src_rgba1[G]) >> 1;
        uint8_t ar = (src_rgba[R] + src_rgba1[R]) >> 1;
        dst_uv[0] = RGBToU(ar, ag, ab);
        dst_uv[1] = RGBToV(ar, ag, ab);
    }
}

void RGBAToUVRow_C_I420(const uint8_t* src_rgba, int src_stride_rgba, uint8_t* dst_u, uint8_t* dst_v,int width) {
    const uint8_t* src_rgba1 = src_rgba + src_stride_rgba;
    int x = 0;
    for (; x < width - 1; x += 2) {
        uint8_t ab = (src_rgba[B] + src_rgba[B + BPP] +
                src_rgba1[B] + src_rgba1[B + BPP]) >> 2;
        uint8_t ag = (src_rgba[G] + src_rgba[G + BPP] +
                   src_rgba1[G] + src_rgba1[G + BPP]) >> 2;
        uint8_t ar = (src_rgba[R] + src_rgba[R + BPP] +
                   src_rgba1[R] + src_rgba1[R + BPP]) >> 2;
        *dst_u = RGBToU(ar, ag, ab);
        *dst_v = RGBToV(ar, ag, ab);
        src_rgba += BPP * 2;
        src_rgba1 += BPP * 2;
        dst_u++;
        dst_v++;
    }
    if (width & 1) {
        uint8_t ab = (src_rgba[B] + src_rgba1[B]) >> 1;
        uint8_t ag = (src_rgba[G] + src_rgba1[G]) >> 1;
        uint8_t ar = (src_rgba[R] + src_rgba1[R]) >> 1;
        *dst_u = RGBToU(ar, ag, ab);
        *dst_v = RGBToV(ar, ag, ab);
    }
}

#ifdef TARGET_ARCH_X86
void RGBAToYRow_Any_SSSE3(const uint8_t* src_rgba, uint8_t* dst_y, int width) {
    RGBAToYRow_Unaligned_SSSE3(src_rgba, dst_y, width - 16);
    RGBAToYRow_Unaligned_SSSE3(src_rgba + (width - 16) * BPP, dst_y + (width - 16) * SBPP, 16);
}

void RGBAToUVRow_Any_SSSE3_NV12(const uint8_t* src_rgba, int src_stride_rgba, uint8_t* dst_uv, int width) {
    int n = width & ~MASK;
    RGBAToUVRow_Unaligned_SSSE3_NV12(src_rgba, src_stride_rgba, dst_uv, n);
    RGBAToUVRow_C_NV12(src_rgba  + n * BPP, src_stride_rgba, dst_uv + n, width & MASK);
}
void RGBAToUVRow_Any_SSSE3_I420(const uint8_t* src_rgba, int src_stride_rgba, uint8_t* dst_u, uint8_t* dst_v, int width) {
    int n = width & ~MASK;
    RGBAToUVRow_Unaligned_SSSE3_I420(src_rgba, src_stride_rgba, dst_u, dst_v, n);
    RGBAToUVRow_C_I420(src_rgba  + n * BPP, src_stride_rgba, dst_u + (n>>1), dst_v + (n>>1), width & MASK);
}

void (*RGBAToUVRow_NV12)(const uint8_t* src_rgba, int src_stride_rgba,
                    uint8_t* dst_uv, int width) = RGBAToUVRow_Any_SSSE3_NV12;
void (*RGBAToUVRow_I420)(const uint8_t* src_rgba, int src_stride_rgba,
                    uint8_t* dst_u, uint8_t* dst_v, int width) = RGBAToUVRow_Any_SSSE3_I420;
void (*RGBAToYRow)(const uint8_t* src_rgba, uint8_t* dst_y, int pix) = RGBAToYRow_Any_SSSE3;
#elif defined(TARGET_ARCH_ARM64)
void (*RGBAToUVRow_NV12)(const uint8_t* src_rgba, int src_stride_rgba,
                    uint8_t* dst_uv, int width) = RGBAToUVRow_C_NV12;
void (*RGBAToUVRow_I420)(const uint8_t* src_rgba, int src_stride_rgba,
                    uint8_t* dst_u, uint8_t* dst_v, int width) = RGBAToUVRow_C_I420;
void (*RGBAToYRow)(const uint8_t* src_rgba, uint8_t* dst_y, int pix) = RGBAToYRow_C;
#elif defined(TARGET_ARCH_ARMV7ABI)
void (*RGBAToUVRow_NV12)(const uint8_t* src_rgba, int src_stride_rgba,
                    uint8_t* dst_uv, int width) = RGBAToUVRow_NEON_NV12;
void (*RGBAToUVRow_I420)(const uint8_t* src_rgba, int src_stride_rgba,
                    uint8_t* dst_u, uint8_t* dst_v, int width) = RGBAToUVRow_NEON_I420;
void (*RGBAToYRow)(const uint8_t* src_rgba, uint8_t* dst_y, int pix) = RGBAToYRow_NEON;
#endif

void ConvertRGB32ToNV12(const uint8_t* src_rgba,
                        uint8_t* dst_y,
                        uint8_t* dst_uv,
                        int width,
                        int height,
                        int src_stride_rgba,
                        int dst_stride_y,
                        int dst_stride_uv) {
#ifdef TARGET_ARCH_X86
    if (IS_ALIGNED(width, 16)) {
        RGBAToUVRow_NV12 = RGBAToUVRow_Unaligned_SSSE3_NV12;
        RGBAToYRow = RGBAToYRow_Unaligned_SSSE3;
        if (IS_ALIGNED(src_rgba, 16) && IS_ALIGNED(src_stride_rgba, 16)) {
            RGBAToUVRow_NV12 = RGBAToUVRow_SSSE3_NV12;
            if (IS_ALIGNED(dst_y, 16) && IS_ALIGNED(dst_stride_y, 16)) {
                RGBAToYRow = RGBAToYRow_SSSE3;
            }
        }
    }
#endif

    for (int y = 0; y < height - 1; y += 2) {
        RGBAToUVRow_NV12(src_rgba, src_stride_rgba, dst_uv, width);
        RGBAToYRow(src_rgba, dst_y, width);
        RGBAToYRow(src_rgba + src_stride_rgba, dst_y + dst_stride_y, width);
        src_rgba += src_stride_rgba * 2;
        dst_y += dst_stride_y * 2;
        dst_uv += dst_stride_uv;
    }
    if (height & 1) {
        RGBAToUVRow_NV12(src_rgba, 0, dst_uv, width);
        RGBAToYRow(src_rgba, dst_y, width);
    }
}

void ConvertRGB32ToI420(const uint8_t* src_rgba,
                        uint8_t* dst_y,
                        uint8_t* dst_u,
                        uint8_t* dst_v,
                        int width,
                        int height,
                        int src_stride_rgba,
                        int dst_stride_y,
                        int dst_stride_u,
                        int dst_stride_v) {
#ifdef TARGET_ARCH_X86
    if (IS_ALIGNED(width, 16)) {
        RGBAToUVRow_I420 = RGBAToUVRow_Unaligned_SSSE3_I420;
        RGBAToYRow = RGBAToYRow_Unaligned_SSSE3;
        if (IS_ALIGNED(src_rgba, 16) && IS_ALIGNED(src_stride_rgba, 16)) {
            RGBAToUVRow_I420 = RGBAToUVRow_SSSE3_I420;
            if (IS_ALIGNED(dst_y, 16) && IS_ALIGNED(dst_stride_y, 16)) {
                RGBAToYRow = RGBAToYRow_SSSE3;
            }
        }
    }
#endif

    for (int y = 0; y < height - 1; y += 2) {
        RGBAToUVRow_I420(src_rgba, src_stride_rgba, dst_u, dst_v, width);
        RGBAToYRow(src_rgba, dst_y, width);
        RGBAToYRow(src_rgba + src_stride_rgba, dst_y + dst_stride_y, width);
        src_rgba += src_stride_rgba * 2;
        dst_y += dst_stride_y * 2;
        dst_u += dst_stride_u;
        dst_v += dst_stride_v;
    }
    if (height & 1) {
        RGBAToUVRow_I420(src_rgba, 0, dst_u, dst_v, width);
        RGBAToYRow(src_rgba, dst_y, width);
    }
}


status_t parseUint16Value(const char* str, uint16_t* pValue) {
    long value;
    char* endptr;
    value = strtol(str, &endptr, 10);
    if (*endptr == '\0') {
        *pValue = value;
        return NO_ERROR;
    }
    return BAD_VALUE;
}

void DumpRawData(const char *path, const void *data, int data_len, int append)
{
    int fd = -1;
    int write_len = 0;
    int file_flags = O_CREAT | O_WRONLY;

    AUTO_LOG();

    if (append)
        file_flags |= O_APPEND;
    fd = open(path, file_flags, 0644);
    if (fd < 0) {
        DWLOGE("Cannot create/open file %s", path);
        return;
    }

    write_len = write(fd, data, data_len);
    if (write_len != data_len) {
        DWLOGE("data_len:%d write_len:%d", data_len, write_len);
    }

    close(fd);
}

uint64_t GetCurrentTimeStamp()
{
    struct timespec tv = {0, 0};
    int ret = clock_gettime(CLOCK_REALTIME, &tv);
    if (ret == -1) {
        DWLOGE("Failed in getting time of day: %s\n", strerror(errno));
    }
    return ((uint64_t)(tv.tv_sec))*1000000LL + (uint64_t)(tv.tv_nsec / 1000);
}

struct timeval TimeStampToTimeval(uint64_t ts)
{
    struct timeval tv;
    tv.tv_sec  = (time_t)(ts / 1000000);
    tv.tv_usec = (long int)(ts % 1000000);
    return tv;
}

uint64_t TimevalToTimeStamp(struct timeval tv)
{
    return ((uint64_t)tv.tv_sec)*1000000LL + tv.tv_usec;
}

}
