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
#include "os_private.h"
#include "os_rmutex.h"
#include "os_sched.h"
#include "os_flags.h"

/*
 * NOTE Be care - each call to lock should be followed by call of unlock.
 */

#ifdef OS_CONFIG_USE_RMUTEX
/*
 * lock mutex
 *
 * ARGS
 *     rm          pointer to recursive mutex structure
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
BASE_TYPE os_rmutex_lock(struct os_rmutex_t *rm, BASE_TYPE flags, BASE_TYPE timeout)
{
    return os_rmutex_lock_tm(rm, flags, &timeout);
}

BASE_TYPE os_rmutex_lock_tm(struct os_rmutex_t *rm, BASE_TYPE flags, BASE_TYPE *timeout)
{
    BASE_TYPE ret;

    ret = OS_ERR_NONE;

    OS_VYIELD();

    OS_DISABLE_IRQ();
    {
        PORT_DATA_BARIER();
        if (rm->nlock == 0 || rm->owner == os_current_taskcb)
        {
            /* lock mutex */
            rm->nlock++;
            rm->owner = os_current_taskcb;
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
        *timeout = os_lock_task(OS_TASK_STATE_LOCKED_RMUTEX, rm, 0, *timeout);
        if (*timeout == OS_TIMEOUT_EXPIRED)
        {
            return OS_ERR_TIMEOUT;
        } else {
            OS_DISABLE_IRQ();
            PORT_DATA_BARIER();
            if (rm->nlock == 0)
            {
                /* lock mutex */
                rm->nlock++;
                rm->owner = os_current_taskcb;
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
void _os_rmutex_unlock(struct os_rmutex_t *rm, BASE_TYPE susp)
{
    OS_DISABLE_IRQ();
    {
        rm->nlock--;
        if (rm->nlock == 0)
            rm->owner = NULL;
        PORT_DATA_BARIER();
        /* suspend task to give other tasks capability to lock mutex */
        if (susp)
            os_sched_suspend_task();
    }
    OS_ENABLE_IRQ();
}

#endif

