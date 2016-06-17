/*
 *     Yet another operating system for microcontrollers.
 *     This file provide functions for bitobjects manipulation (mutexes, events).
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
#ifndef OS_BITOBJ_H
#define OS_BITOBJ_H

#include <types.h>

#ifdef OS_CONFIG_USE_MUTEX
    #define OS_MUTEX_UNLOCKED 0
    #define OS_MUTEX_LOCKED   1
    
    BASE_TYPE os_mutex_lock(BASE_TYPE *pm, BASE_TYPE mask, BASE_TYPE flags, BASE_TYPE timeout);
    BASE_TYPE os_mutex_lock_tm(BASE_TYPE *pm, BASE_TYPE mask, BASE_TYPE flags, BASE_TYPE *timeout);
    void os_mutex_unlock(BASE_TYPE *pm, BASE_TYPE mask);
    void os_mutex_unlock_ns(BASE_TYPE *pm, BASE_TYPE mask);
#endif

#ifdef OS_CONFIG_USE_EVENT
    void os_event_raise(BASE_TYPE *pe, BASE_TYPE mask);
    void os_event_raise_ns(BASE_TYPE *pe, BASE_TYPE mask);
    void os_event_clear(BASE_TYPE *pe, BASE_TYPE mask);
    BASE_TYPE os_event_wait(BASE_TYPE *pe, BASE_TYPE mask, BASE_TYPE flags, BASE_TYPE timeout);
    BASE_TYPE os_event_wait_tm(BASE_TYPE *pe, BASE_TYPE mask, BASE_TYPE flags, BASE_TYPE *timeout);
#endif

#endif

