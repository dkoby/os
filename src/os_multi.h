/*
 *     Yet another operating system for microcontrollers.
 *     This file provide definitions of multiple event.
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
#ifndef OS_MULTI_H
#define OS_MULTI_H

#include <types.h>
#include "os_bitobj.h"
#include "os_queue.h"

struct os_multi_event_t {
    BASE_TYPE count;  /* number of events serviced by this multiple event */
    BASE_TYPE n;      /* number of currently initialized events */
#define OS_MULTI_LOCK_OR    0
#define OS_MULTI_LOCK_AND   1
    BASE_TYPE type;
    struct os_task_lock_t *events;
};

#define OS_MULTI_QUEUE_NOT_EMPTY    0
#define OS_MULTI_QUEUE_NOT_FULL     1

struct os_multi_event_t *os_multi_init(BASE_TYPE count);
void os_multi_reset(struct os_multi_event_t *m);

void os_multi_add_event(struct os_multi_event_t *m, BASE_TYPE *pe, BASE_TYPE mask);
void os_multi_add_queue(struct os_multi_event_t *m, struct os_queue_t *q, BASE_TYPE type);

BASE_TYPE os_multi_wait(struct os_multi_event_t *m, BASE_TYPE type, BASE_TYPE timeout);

#endif /* OS_MULTI_H */

