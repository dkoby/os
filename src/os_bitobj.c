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
#include "os_private.h"
#include "os_bitobj.h"
#include "os_sched.h"
#include "os_flags.h"


#define OS_BITMASK_SET(bitset, mask)   bitset |= mask
#define OS_BITMASK_CLEAR(bitset, mask) bitset &= ~(mask)

#ifdef OS_CONFIG_USE_MUTEX
/*
 * lock mutex
 *
 * ARGS
 *     pm          pointer to memory contained bits for mutexes
 *     mask        mask of mutex to lock
 *     flags       
 *                 OS_FLAG_NONE      no flag specified
 *                 OS_FLAG_NOWAIT    don't wait if mutex is locked, return
 *                                   immediately with OS_ERR_WOULDLOCK error code
 *
 *     timeout     if specified wait with timeout for mutex unlock
 *
 * NOTE
 *     * OS_TIME2TICK_MS() and OS_TIME2TICK_US() macro should be
 *       used to specify time interval for timeout in milliseconds and microsecnds.
 *     * should not be called from ISR
 *
 * RETURN
 *     OS_ERR_NONE       if mutex unlocked
 *     OS_ERR_TIMEOUT    timeout occured before mutex was unlocked
 *     OS_ERR_WOULDLOCK  mutex was locked but OS_FLAG_NOWAIT was specified
 */
BASE_TYPE os_mutex_lock(BASE_TYPE *pm, BASE_TYPE mask, BASE_TYPE flags, BASE_TYPE timeout)
{
    return os_mutex_lock_tm(pm, mask, flags, &timeout);
}

BASE_TYPE os_mutex_lock_tm(BASE_TYPE *pm, BASE_TYPE mask, BASE_TYPE flags, BASE_TYPE *timeout)
{
    BASE_TYPE ret;

    ret = OS_ERR_NONE;

    OS_VYIELD();

    OS_DISABLE_IRQ();
    {
        PORT_DATA_BARIER();
        if ((*pm & mask) == 0)
        {
            OS_BITMASK_SET(*pm, mask); /* lock mutex */
            PORT_DATA_BARIER();
            ret = OS_ERR_NONE;
            goto out;
        }
        if (flags & OS_FLAG_NOWAIT)
        {
            ret = OS_ERR_WOULDLOCK;
            goto out;
        }
    }
    OS_ENABLE_IRQ();

    while (1)
    {
        *timeout = os_lock_task(OS_TASK_STATE_LOCKED_MUTEX, pm, mask, *timeout);
        if (*timeout == OS_TIMEOUT_EXPIRED)
        {
            return OS_ERR_TIMEOUT;
        } else {
            OS_DISABLE_IRQ();
            PORT_DATA_BARIER();
            if ((*pm & mask) == 0)
            {
                OS_BITMASK_SET(*pm, mask); /* lock mutex */
                PORT_DATA_BARIER();
                ret = OS_ERR_NONE;
                goto out;
            }
            OS_ENABLE_IRQ();
        }
    }
out:
    OS_ENABLE_IRQ();
    return ret;
}

/*
 * unlock mutex
 *
 * NOTE
 *     * should not be called from ISR
 */
void os_mutex_unlock(BASE_TYPE *pm, BASE_TYPE mask)
{
    OS_DISABLE_IRQ();
    {
        OS_BITMASK_CLEAR(*pm, mask); /* unlock mutex */
        PORT_DATA_BARIER();
        /* suspend task to give other tasks capability to lock mutex */
        os_sched_suspend_task();
    }
    OS_ENABLE_IRQ();
}

/*
 * unlock mutex, but don't suspend task
 */
void os_mutex_unlock_ns(BASE_TYPE *pm, BASE_TYPE mask)
{
    OS_DISABLE_IRQ();
    OS_BITMASK_CLEAR(*pm, mask); /* unlock mutex */
    PORT_DATA_BARIER();
    OS_ENABLE_IRQ();
}
#endif /* OS_CONFIG_USE_MUTEX */

#ifdef OS_CONFIG_USE_EVENT
/*
 * raise event
 *
 * NOTE
 *     * can be called from ISR
 *
 * ARGS
 *     pe          pointer to memory contained bits for events
 *     mask        mask of events to raise
 */
void os_event_raise(BASE_TYPE *pe, BASE_TYPE mask)
{
    OS_DISABLE_IRQ();
    {
        OS_BITMASK_SET(*pe, mask); /* raise event */
        PORT_DATA_BARIER();
        /* suspend task to give other tasks capability to handle event */
        os_sched_suspend_task();
    }
    OS_ENABLE_IRQ();
}

/*
 * raise event, but don't switch task
 *
 * ARGS
 *     pe          pointer to memory contained bits for events
 *     mask        mask of events to raise
 */
void os_event_raise_ns(BASE_TYPE *pe, BASE_TYPE mask)
{
    OS_DISABLE_IRQ();
    {
        OS_BITMASK_SET(*pe, mask); /* raise event */
        PORT_DATA_BARIER();
    }
    OS_ENABLE_IRQ();
}

/*
 * simply clear event
 */
void os_event_clear(BASE_TYPE *pe, BASE_TYPE mask)
{
    OS_DISABLE_IRQ();
    {
        OS_BITMASK_CLEAR(*pe, mask); /* clear event */
    }
    OS_ENABLE_IRQ();
}

/*
 * wait of event occurence
 *
 * * ARGS
 *     pe          pointer to memory contained bits for events
 *     mask        mask of events to wait
 *     flags       
 *                 OS_FLAG_NONE      no flag specified
 *                 OS_FLAG_NOWAIT    don't wait if event not present, return
 *                                   immediately with OS_ERR_WOULDLOCK error code
 *                 OS_FLAG_CLEAR     clear event on catch
 *                                   (beware if more then one task waiting for event
 *                                    only first task catch event occurence)
 *
 *     timeout     if specified wait with timeout for event occurence
 *
 * NOTE
 *     * OS_TIME2TICK_MS() and OS_TIME2TICK_US() macro should be
 *       used to specify time interval for timeout in milliseconds and microseconds.
 *     * should not be called from ISR
 *
 * RETURN
 *     OS_ERR_NONE       if event was occured
 *     OS_ERR_TIMEOUT    timeout occured before event was occured
 *     OS_ERR_WOULDLOCK  event not present but OS_FLAG_NOWAIT was specified
 */
BASE_TYPE os_event_wait(BASE_TYPE *pe, BASE_TYPE mask, BASE_TYPE flags, BASE_TYPE timeout)
{
    return os_event_wait_tm(pe, mask, flags, &timeout);
}

BASE_TYPE os_event_wait_tm(BASE_TYPE *pe, BASE_TYPE mask, BASE_TYPE flags, BASE_TYPE *timeout)
{
    BASE_TYPE ret;

    ret = OS_ERR_NONE;

    OS_VYIELD();

    OS_DISABLE_IRQ();
    {
        if (*pe & mask)
        {
            ret = OS_ERR_NONE;
            if (flags & OS_FLAG_CLEAR)
                OS_BITMASK_CLEAR(*pe, mask /* XXX *pe & mask */);
            goto out;
        }

        if (flags & OS_FLAG_NOWAIT)
        {
            ret = OS_ERR_WOULDLOCK;
            goto out;
        }
    }
    OS_ENABLE_IRQ();

    while (1)
    {
        *timeout = os_lock_task(OS_TASK_STATE_LOCKED_EVENT, pe, mask, *timeout);
        if (*timeout == OS_TIMEOUT_EXPIRED)
        {
            return OS_ERR_TIMEOUT;
        } else {
            OS_DISABLE_IRQ();
            if (*pe & mask)
            {
                ret = OS_ERR_NONE;
                if (flags & OS_FLAG_CLEAR)
                    OS_BITMASK_CLEAR(*pe, mask /* XXX *pe & mask */);
                goto out;
            }
            OS_ENABLE_IRQ();
        }
    }
out:
    OS_ENABLE_IRQ();
    return ret;
}
#endif /* OS_CONFIG_USE_EVENT */

