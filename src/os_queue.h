/*
 *     Yet another operating system for microcontrollers.
 *     This file contains routines that implement queues mechanism.
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
#ifndef OS_QUEUE_H
#define OS_QUEUE_H

#include <types.h>

struct os_queue_t {
    BASE_TYPE mutex; /* mutex associated with queue */
    BASE_TYPE qsize; /* size of queue in bytes (size of occupied area of spool) */
    BASE_TYPE msize; /* size of message in bytes */
    BASE_TYPE count; /* count of bytes contained in queue */
    BASE_TYPE wp;    /* write pointer */

    uint8_t *spool;    /* pointer to spool */
};

struct os_queue_t *os_queue_init(BASE_TYPE len, BASE_TYPE msize);
#define os_queue_add(q, flags, timeout, data, len) \
    os_queue_add_with_event(q, flags, timeout, data, len, NULL, 0)
BASE_TYPE os_queue_add_with_event(struct os_queue_t *q, BASE_TYPE flags, BASE_TYPE timeout, void *data, BASE_TYPE len, BASE_TYPE *pe, BASE_TYPE ne_mask);

#define os_queue_remove(q, flags, timeout, data, len) \
    os_queue_remove_with_event(q, flags, timeout, data, len, NULL, 0)
BASE_TYPE os_queue_remove_with_event(struct os_queue_t *q, BASE_TYPE flags, BASE_TYPE timeout, void *data, BASE_TYPE *len, BASE_TYPE *pe, BASE_TYPE ne_mask);
void os_queue_flush(struct os_queue_t *q);
int os_queue_isempty(struct os_queue_t *q);

#endif

