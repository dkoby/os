/*
 *     Yet another operating system for microcontrollers.
 *     This file provide functions for recursive mutex.
 *
 * Copyright (c) 2013, Dmitry Kobylin
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met: 
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer. 
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution. 
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 */
#ifndef _OS_RMUTEX_H_
#define _OS_RMUTEX_H_

#include <types.h>

#ifdef OS_CONFIG_USE_RMUTEX

struct os_rmutex_t {
    volatile struct os_taskcb_t *owner;
    BASE_TYPE nlock;
};

BASE_TYPE os_rmutex_lock(struct os_rmutex_t *rm, BASE_TYPE flags, BASE_TYPE timeout);
BASE_TYPE os_rmutex_lock_tm(struct os_rmutex_t *rm, BASE_TYPE flags, BASE_TYPE *timeout);

void _os_rmutex_unlock(struct os_rmutex_t *rm, BASE_TYPE susp);

#define os_rmutex_unlock(rm)    _os_rmutex_unlock(rm, 1)
#define os_rmutex_unlock_ns(rm) _os_rmutex_unlock(rm, 0)

#endif

#endif

