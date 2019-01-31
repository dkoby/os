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
#include "os_private.h"
#include "os_multi.h"
#include "os_sched.h"
#include "os_flags.h"
#include "os_mem.h"

#ifdef OS_CONFIG_USE_MULTI
STATIC void os_multi_add(struct os_multi_event_t *m, BASE_TYPE state, void *pobj, BASE_TYPE mask);

/*
 * initialize multiple event
 *
 * ARGS
 *     count    number of events serviced by this multiple event
 *
 * RETURN
 *     pointer to allocated multiple event
 */
struct os_multi_event_t *os_multi_init(BASE_TYPE count)
{
    struct os_multi_event_t *m;

    OS_DISABLE_IRQ();
    m = os_malloc(sizeof(struct os_multi_event_t));

    m->count  = count;
    m->n      = 0;
    m->events = os_malloc(sizeof(struct os_task_lock_t) * count);

    OS_ENABLE_IRQ();
    return m;
}

/*
 * reset all events to unhandled state
 */
void os_multi_reset(struct os_multi_event_t *m)
{
    m->n = 0;
}

/*
 * add event to multiple event
 */
void os_multi_add_event(struct os_multi_event_t *m, BASE_TYPE *pe, BASE_TYPE mask)
{
    os_multi_add(m, OS_TASK_STATE_LOCKED_EVENT, pe, mask);
}

/*
 * add queue to multiple event
 * ARGS
 *      type
 *          OS_MULTI_QUEUE_NOT_EMPTY    whait when queue become not empty
 *          OS_MULTI_QUEUE_NOT_FULL     whait when queue become not full
 */
void os_multi_add_queue(struct os_multi_event_t *m, struct os_queue_t *q, BASE_TYPE type)
{
    os_multi_add(m,
            type == OS_MULTI_QUEUE_NOT_EMPTY ? OS_TASK_STATE_LOCKED_QUEUE_EMPTY : OS_TASK_STATE_LOCKED_QUEUE_FULL,
            q, 0);
}

/*
 *
 */
STATIC void os_multi_add(struct os_multi_event_t *m, BASE_TYPE state, void *pobj, BASE_TYPE mask)
{
    struct os_task_lock_t *event;

    OS_DISABLE_IRQ();
#ifdef OS_CONFIG_TRAP_MULTI
    if (m->n >= m->count)
    {
        os_trapinfo.err = OS_TRAP_ERR_MULTI_ECOUNT;
        os_trap();
    }
#endif
    event = &m->events[m->n];

    event->state = state;
    event->pobj  = pobj;
    event->mask  = mask;

    m->n++;

    OS_ENABLE_IRQ();
}

/*
 * wait for multiple events
 *
 * ARGS
 *     m        pointer to multiple event structure
 *     type
 *         OS_MULTI_LOCK_OR    wait for one of event to trigger
 *         OS_MULTI_LOCK_AND   wait for all events to trigger
 *     timeout     if specified wait with timeout for events to trigger
 *
 * NOTE
 *     * should not be called from ISR
 *     * this funciton for one multi lock should be called only from one task
 *
 * RETURN
 *     OS_ERR_NONE       if one or all events are triggered (depends on "type" field)
 *     OS_ERR_TIMEOUT    timeout occured before events occured
 */
BASE_TYPE os_multi_wait(struct os_multi_event_t *m, BASE_TYPE type, BASE_TYPE timeout)
{
    m->type = type;

    if (os_lock_task(OS_TASK_STATE_LOCKED_MULTI, m, 0, timeout) == OS_TIMEOUT_EXPIRED)
        return OS_ERR_TIMEOUT;
    else
        return OS_ERR_NONE;
}

#endif /* OS_CONFIG_USE_MULTI */

