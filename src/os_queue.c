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
#include <string.h> /* for memcpy, TODO write own function? */
#include "os_private.h"
#include "os_queue.h"
#include "os_bitobj.h"
#include "os_sched.h"
#include "os_flags.h"
#include "os_mem.h"

#if (defined OS_CONFIG_USE_QUEUE)

struct os_qmsg_head_t {
    BASE_TYPE len;
};

struct os_qmsg_t {
    struct os_qmsg_head_t head;
    uint8 data[1];
};

/*
 * initialize queue
 *
 * ARGS
 *     midx     mutex associated with queue
 *     len      number of messages in queue
 *     msize    size of message in bytes (only size of data field)
 *
 * RETURN
 *     pointer to allocated queue
 */
struct os_queue_t *os_queue_init(BASE_TYPE len, BASE_TYPE msize)
{
    struct os_queue_t *q;

    OS_DISABLE_IRQ();
    q = os_malloc(sizeof(struct os_queue_t));

    msize += (sizeof(BASE_TYPE) - (msize % sizeof(BASE_TYPE)));

    q->mutex = 0;
    q->msize = msize + sizeof(struct os_qmsg_head_t);
    q->qsize = len * q->msize;
    q->count = 0;
    q->wp    = 0;
    q->spool = os_malloc(q->qsize);
    OS_ENABLE_IRQ();

    return q;
}

/*
 * ARGS
 *     q           pointer to queue structure
 *     flags       
 *                 OS_FLAG_NONE      no flag specified
 *                 OS_FLAG_NOWAIT    don't wait if queue is full, return
 *                                   immediately with OS_ERR_WOULDLOCK error code
 *     timeout     if specified wait with timeout when queue become not full
 *     data        pointer to data that should be placed to queue
 *     len         length of data to place
 *     pe          pointer to event bit-object to set for not empty event,
 *                 should be NULL if not used
 *     ne_mask     not empty mask for event
 *
 * NOTE
 *     * OS_TIME2TICK_MS() and OS_TIME2TICK_US() macro should be
 *       used to specify time interval for timeout in milliseconds and microsecnds.
 *     * should not be called from ISR
 *
 * RETURN
 *     OS_ERR_NONE       if message sucessuflly was added to queue
 *     OS_ERR_TIMEOUT    timeout occured before space appeared in queue
 *     OS_ERR_WOULDLOCK  queue was full but OS_FLAG_NOWAIT was specified
 */
BASE_TYPE os_queue_add_with_event(struct os_queue_t *q, BASE_TYPE flags,
        BASE_TYPE timeout, void *data, BASE_TYPE len,
        BASE_TYPE *pe, BASE_TYPE ne_mask)
{
    BASE_TYPE ret;

    if (len == 0)
        return OS_ERR_NONE;

#ifdef OS_CONFIG_TRAP_QUEUE
    if ((len + sizeof(struct os_qmsg_head_t)) > q->msize)
    {
        os_trapinfo.err = OS_TRAP_ERR_QUEUE_MSIZE;
        os_trap();
    }
#endif
    ret = OS_ERR_NONE;
    while (1)
    {
        /* lock mutex with timeout */
        if (os_mutex_lock_tm(&q->mutex, 0x01, OS_FLAG_NONE, &timeout) == OS_ERR_TIMEOUT)
            return OS_ERR_TIMEOUT;

        /* check that there is enough space in queue */
        if (q->count < q->qsize)
        {
            struct os_qmsg_t *qmsg;

            qmsg = (struct os_qmsg_t *)&q->spool[q->wp];
            qmsg->head.len = len;
            memcpy(qmsg->data, data, len);

            q->count += q->msize;
            q->wp    += q->msize;
            if (q->wp >= q->qsize)
                q->wp = 0;
            goto out;
        } else {
            if (flags & OS_FLAG_NOWAIT)
            {
                ret = OS_ERR_WOULDLOCK;
                goto out;
            }

            os_mutex_unlock(&q->mutex, 0x01);
            timeout = os_lock_task(OS_TASK_STATE_LOCKED_QUEUE_FULL, q, 0, timeout);
            if (timeout == OS_TIMEOUT_EXPIRED)
                return OS_ERR_TIMEOUT;
        }
    }
out:
    if (pe && q->count > 0)
        os_event_raise_ns(pe, ne_mask);
    os_mutex_unlock(&q->mutex, 0x01);
    return ret;
}

/*
 * ARGS
 *     q           pointer to queue structure
 *     flags       
 *                 OS_FLAG_NONE      no flag specified
 *                 OS_FLAG_NOWAIT    don't wait if queue is empty, return
 *                                   immediately with OS_ERR_WOULDLOCK error code
 *     timeout     if specified wait with timeout when queue become not full
 *     data        pointer to data where data from queue will be placed
 *     len         pointer to length of data that was removed from queue, NULL if not care
 *     pe          pointer to event bit-object to clear for not empty event,
 *                 should be NULL if not used
 *     ne_mask     not empty mask for event
 *
 * NOTE
 *     * OS_TIME2TICK_MS() and OS_TIME2TICK_US() macro should be
 *       used to specify time interval for timeout in milliseconds and microseconds.
 *     * should not be called from ISR
 *
 * RETURN
 *     OS_ERR_NONE       if message sucessuflly was added to queue
 *     OS_ERR_TIMEOUT    timeout occured before space appeared in queue
 *     OS_ERR_WOULDLOCK  queue was empty but OS_FLAG_NOWAIT was specified
 */
BASE_TYPE os_queue_remove_with_event(struct os_queue_t *q, BASE_TYPE flags,
        BASE_TYPE timeout, void *data, BASE_TYPE *len,
        BASE_TYPE *pe, BASE_TYPE ne_mask)
{
    BASE_TYPE ret;

    ret = OS_ERR_NONE;
    while (1)
    {
        /* lock mutex with timeout */
        if (os_mutex_lock_tm(&q->mutex, 0x01, OS_FLAG_NONE, &timeout) == OS_ERR_TIMEOUT)
            return OS_ERR_TIMEOUT;

        /* check that queue not empty */
        if (q->count > 0)
        {
            struct os_qmsg_t *qmsg;

            if (q->wp >= q->count)
                qmsg = (struct os_qmsg_t *)&q->spool[q->wp - q->count];
            else
                qmsg = (struct os_qmsg_t *)&q->spool[q->qsize - q->count + q->wp];

            if (len)
                *len = qmsg->head.len;
            memcpy(data, qmsg->data, qmsg->head.len);

            q->count -= q->msize;
            goto out;
        } else {
            if (flags & OS_FLAG_NOWAIT)
            {
                ret = OS_ERR_WOULDLOCK;
                goto out;
            }

            os_mutex_unlock(&q->mutex, 0x01);
            timeout = os_lock_task(OS_TASK_STATE_LOCKED_QUEUE_EMPTY, q, 0, timeout);
            if (timeout == OS_TIMEOUT_EXPIRED)
                return OS_ERR_TIMEOUT;
        }
    }
out:
    if (pe && q->count == 0)
        os_event_clear(pe, ne_mask);
    os_mutex_unlock(&q->mutex, 0x01);
    return ret;
}
#endif /* defined OS_CONFIG_USE_QUEUE */

/*
 * remove all entries from queue
 *
 * ARGS
 *     q           pointer to queue structure
 */
void os_queue_flush(struct os_queue_t *q)
{
    os_mutex_lock_tm(&q->mutex, 0x01, OS_FLAG_NONE, OS_WAIT_FOREVER);
    q->count = 0;
    q->wp = 0;
    os_mutex_unlock(&q->mutex, 0x01);
}

/*
 * check if queue is empty
 */
int os_queue_isempty(struct os_queue_t *q)
{
    int ret;

    os_mutex_lock_tm(&q->mutex, 0x01, OS_FLAG_NONE, OS_WAIT_FOREVER);
    {
        if (q->count)
            ret = 0;
        else
            ret = 1;
    }
    os_mutex_unlock(&q->mutex, 0x01);

    return ret;
}

