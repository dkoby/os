/*
 *     Yet another operating system for microcontrollers.
 *     This file contain schdeler routines.
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
#include <debug.h>
#include <LPC177x_8x.h> /* XXX */
#include "os_sched.h"

#ifdef OS_CONFIG_USE_QUEUE
    #include "os_queue.h"
#endif

#ifdef OS_CONFIG_USE_MULTI
    #include "os_multi.h"
#endif

#ifdef OS_CONFIG_USE_RMUTEX
    #include "os_rmutex.h"
#endif

#if OS_USE_LOCK
STATIC volatile struct os_taskcb_t *sqhead; /* pointer to head of scheduler queue */
STATIC volatile struct os_taskcb_t *sqtail; /* pointer to end of scheduler queue */
STATIC inline BASE_TYPE os_sched_task_ready(volatile struct os_taskcb_t *pt);
STATIC inline BASE_TYPE os_sched_task_unlocked(struct os_task_lock_t *lock);
#endif

#if (OS_USE_LOCK)
/*
 * Move task to tail of queue. Mark it's state as locked.
 *
 * NOTE timeout of task reset to zero by this funciton after task was unlocked
 *
 * RETURN
 *     OS_TIMEOUT_EXPIRED if timeout occured before task was unlocked by condition of lock object.
 *     If task was unlocked by accomplish of object condition then number of ticks remained returned.
 */
BASE_TYPE os_lock_task(BASE_TYPE type, void *pobj, BASE_TYPE mask, BASE_TYPE timeout)
{
    BASE_TYPE ret;

    OS_DISABLE_IRQ();
    {
        if (os_current_taskcb->timeout == OS_TIMEOUT_EXPIRED)
        {
            os_current_taskcb->timeout = 0; /* NOTE */
            OS_ENABLE_IRQ();
            return OS_TIMEOUT_EXPIRED;
        }
#ifdef DEBUG_OS_PRINT
        DEBUG_OS_STR("lock task ");
        DEBUG_OS_HEX(&task, 4);
        DEBUG_OS_HEX_WRAP(", type = ", &type, 4);
#endif
        os_current_taskcb->timeout      = timeout;
        os_current_taskcb->lock.state   = type;
        os_current_taskcb->lock.pobj    = pobj;
        os_current_taskcb->lock.mask    = mask;
#ifdef OS_CONFIG_USE_TASK_SLICE
        /* task was locked, respawn it's time slice */
        os_respawn_tslice(os_current_taskcb);
#endif
    }
    OS_ENABLE_IRQ();

    /* switch task */
    os_task_switch();

    /* scheduler returned control to task */
    OS_DISABLE_IRQ();
    {
#ifdef DEBUG_OS_PRINT
        DEBUG_OS_STR("unlock task ");
        DEBUG_OS_HEX(&task, 4);
        DEBUG_OS_HEX_WRAP(", state = ", &os_current_taskcb->state, 4);
#endif
        ret                = os_current_taskcb->timeout;
        os_current_taskcb->timeout      = 0; /* NOTE */
        os_current_taskcb->lock.state   = OS_TASK_STATE_RUN;
    }
    OS_ENABLE_IRQ();

    return ret;
}

/*
 * Add task to scheduler queue.
 */
void os_squeue_addtask(volatile struct os_taskcb_t *task)
{
    if (sqhead == NULL)
    {
        sqhead = task;
        sqtail = task;
    } else {
        task->prev   = sqtail;
        task->next   = NULL;
        sqtail->next = task;
        sqtail       = task;
    }
}

/*
 * move task to tail of queue (last task that will be checked by scheduler)
 */
void os_squeue_task2tail(volatile struct os_taskcb_t *task)
{
    /* nothing to do if task already in tail */
    if (task == sqtail)
        return;

    if (task == sqhead)
    {
        sqhead = sqhead->next;
        sqhead->prev = NULL;
    } else {
        task->prev->next = task->next;
        task->next->prev = task->prev;
    }

    sqtail->next = task;
    task->prev   = sqtail;
    task->next   = NULL;
    sqtail       = task;
}

/*
 * suspend current task execution
 *
 * NOTE this funciton should be call during interrupts disabled
 */
void os_sched_suspend_task()
{
#ifdef OS_CONFIG_USE_PRIORITY
    volatile struct os_taskcb_t *pt;
#endif
    if (os_current_taskcb == OS_IDLE_TASKCB)
    {
        /* IDLE task not in scheduler queue switch it simply */
        os_task_switch();
    } else {
        /* 
         * This function can be called from ISR (os_tick() or other ISR
         * with os_event_raise()). State of task must be set to suspend
         * only if it is not already locked by os_lock_task().
         */
#ifdef OS_CONFIG_USE_PRIORITY
        /* 
         * Check priority of current task. If current task have higher priority
         * then one of ready task - don't suspend it.
         */
        if (os_current_taskcb->lock.state == OS_TASK_STATE_RUN)
        {
            for (pt = sqhead; pt != NULL; pt = pt->next)
            {
                /* don't check current task for priority */
                if (pt == os_current_taskcb)
                    continue;

                if (pt->lock.state == OS_TASK_STATE_SUSPEND ||
                    os_sched_task_ready(pt))
                {
                    /* 
                     * Suspend current task if there is ready task with
                     * the same or higher priority in the queue.
                     */
                    if (pt->priority <= os_current_taskcb->priority)
                    {
                        os_current_taskcb->lock.state = OS_TASK_STATE_SUSPEND;
                        os_task_switch();
                        return;
                    }
                }
            }
        }
#else
        if (os_current_taskcb->lock.state == OS_TASK_STATE_RUN)
        {
            os_current_taskcb->lock.state = OS_TASK_STATE_SUSPEND;
            os_task_switch();
        }
#endif
    }
}
#else /* !OS_USE_LOCK */
/*
 * find next task to task given in argument
 */
STATIC struct os_taskcb_t* os_task_next(volatile struct os_taskcb_t *task)
{
    task++;
    if ((BASE_TYPE)task > ((BASE_TYPE)&os_tasks[os_taskidx - 1]))
        task = os_tasks;

    return task;
}
#endif /* OS_USE_LOCK */

#ifdef OS_CONFIG_USE_SCHEDLOCK
/*
 * lock scheduler (prevent to switch to other task)
 */
void os_sched_lock()
{
    OS_DISABLE_IRQ();
    os_schedlock_taskcb = os_current_taskcb;
    OS_ENABLE_IRQ();
}
/*
 * unlock scheduler (permit to switch to other task)
 */
void os_sched_unlock()
{
    OS_DISABLE_IRQ();
    os_schedlock_taskcb = NULL;
    OS_ENABLE_IRQ();
}
#endif

/*
 *
 */
void os_scheduler()
{
#if OS_USE_LOCK
    volatile struct os_taskcb_t *pt;
    volatile struct os_taskcb_t *readytask;
    volatile struct os_taskcb_t *susptask;
    struct os_task_lock_t *lock;
#ifdef OS_CONFIG_USE_PRIORITY
    volatile struct os_taskcb_t *priotask;
#endif
    /*
     * Scheduler was called, so task was interrupted for some reason.
     * Move current task to end of queue.
     */
    if (os_current_taskcb != OS_IDLE_TASKCB)
        os_squeue_task2tail(os_current_taskcb); 

#ifdef DEBUG_OS_PRINT
    DEBUG_OS_STR("sched chain:\r\n");
    for (pt = sqhead; pt != NULL; pt = pt->next)
    {
        DEBUG_OS_STR("    task "); DEBUG_OS_HEX(&pt, 4);
        DEBUG_OS_HEX_WRAP(", state = ", &pt->state, 4);
    }
#endif

#ifdef OS_CONFIG_TRAP_SCHEDQ
    if (sqhead == NULL)
    {
        os_trapinfo.err = OS_TRAP_ERR_SCHEDQ_EMPTY_QUEUE;
        os_trap();
    }
#endif

    readytask = NULL;
    susptask  = NULL;
#ifdef OS_CONFIG_USE_PRIORITY
    priotask  = NULL;
#endif

#ifdef OS_CONFIG_USE_SCHEDLOCK
    if (os_schedlock_taskcb)
    {
        lock = (struct os_task_lock_t*)&os_schedlock_taskcb->lock;
        if (!(lock->state & OS_TASK_LOCKED))
            susptask = os_schedlock_taskcb;
        if (os_schedlock_taskcb->timeout == OS_TIMEOUT_EXPIRED)
            readytask = os_schedlock_taskcb;
        else if (os_sched_task_unlocked(lock))
            readytask = os_schedlock_taskcb;
    } else
#endif    
        for (pt = sqhead; pt != NULL; pt = pt->next)
        {
            lock = (struct os_task_lock_t*)&pt->lock;

            if (!(lock->state & OS_TASK_LOCKED))
            {
                if (susptask == NULL)
                    susptask = pt;
                continue;
            }
            if (pt->timeout == OS_TIMEOUT_EXPIRED)
                goto ready;
            if (os_sched_task_unlocked(lock))
                goto ready;

            continue;
ready:
            readytask = pt;
#ifdef OS_CONFIG_USE_PRIORITY
            if (priotask == NULL)
            {
                priotask = readytask;
            } else {
                /* lower number - higher priority */
                if (readytask->priority < priotask->priority)
                    priotask = readytask;
            }
#else
            break;
#endif
        }

#ifdef OS_CONFIG_USE_PRIORITY
    if (priotask)
        readytask = priotask;
#endif

    /* if no one task ready and there is suspended task in queue - resume suspended task */
    if (readytask == NULL && susptask)
    {
        susptask->lock.state = OS_TASK_STATE_RUN;
        readytask = susptask;
    }

    /* no one task ready - run idle task */
    if (readytask == NULL)
        readytask = OS_IDLE_TASKCB;

    os_current_taskcb = readytask;
#else /* !OS_USE_LOCK */
    os_current_taskcb = os_task_next(os_current_taskcb);
#endif /* OS_USE_LOCK */

#ifdef OS_CONFIG_USE_TRACE
    os_current_taskcb->schedhit++; /* count of scheduler hits of this task */
    #ifdef OS_STACK_DIR_DECREASE
    if (os_current_taskcb->tos_max <= os_current_taskcb->tos)
        os_current_taskcb->tos_max = os_current_taskcb->tos;
    #else
    if (os_current_taskcb->tos_max >= os_current_taskcb->tos)
        os_current_taskcb->tos_max = os_current_taskcb->tos;
    #endif
#endif

    /* XXX */
    if (os_current_taskcb == OS_IDLE_TASKCB)
        LPC_GPIO0->CLR = (1 << 26);
    else
        LPC_GPIO0->SET = (1 << 26);
}

#if OS_USE_LOCK
/*
 * Check if task is ready to run
 *
 * RETURN
 *     zero if task not ready to run, one otherwise
 */
STATIC inline BASE_TYPE os_sched_task_ready(volatile struct os_taskcb_t *pt)
{
    if ((pt->lock.state & OS_TASK_LOCKED) && pt->timeout == OS_TIMEOUT_EXPIRED)
        return 1;
    if (os_sched_task_unlocked((struct os_task_lock_t*)&pt->lock))
        return 1;

    return 0;
}

/*
 * Check if task's lock condition is released
 *
 * RETURN
 *     1 if lock condition is released, zero othwerwise
 */
STATIC inline BASE_TYPE os_sched_task_unlocked(struct os_task_lock_t *lock)
{
#ifdef OS_CONFIG_USE_MULTI
    BASE_TYPE n, nready;
    struct os_multi_event_t *multi;
    struct os_task_lock_t *event;
#endif

    /* TODO 
     *
     * make jump table and checks in assembler
     */

    switch (lock->state)
    {
#ifdef OS_CONFIG_USE_MUTEX
        case OS_TASK_STATE_LOCKED_MUTEX:
            {
                if (!((*(BASE_TYPE*)lock->pobj) & lock->mask))
                    return 1;
                else
                    return 0;
            }
            break;
#endif
#ifdef OS_CONFIG_USE_EVENT
        case OS_TASK_STATE_LOCKED_EVENT:
            {
                if ((*(BASE_TYPE*)lock->pobj) & lock->mask)
                    return 1;
                else
                    return 0;
            }
            break;
#endif
#ifdef OS_CONFIG_USE_QUEUE
        case OS_TASK_STATE_LOCKED_QUEUE_FULL:
            {
                struct os_queue_t *q;
               
                q = (struct os_queue_t*)lock->pobj;
                if (q->count < q->qsize)
                    return 1;
                else
                    return 0;
            }
            break;
        case OS_TASK_STATE_LOCKED_QUEUE_EMPTY:
            {
                struct os_queue_t *q;

                q = (struct os_queue_t*)lock->pobj;
                if (q->count > 0)
                    return 1;
                else
                    return 0;
            }
            break;
#endif
#ifdef OS_CONFIG_USE_MULTI
        case OS_TASK_STATE_LOCKED_MULTI:
            {
                multi  = (struct os_multi_event_t *)lock->pobj;
                event  = multi->events;
                n      = multi->n;
                nready = 0;
                while (n--)
                {
                    /* NOTE called recursively, be care of stack size */
                    if (os_sched_task_unlocked(event))
                    {
                        if (multi->type == OS_MULTI_LOCK_OR)
                            return 1;
                        nready++;
                    }
                    event++;
                }
                if (nready == multi->n)
                    return 1;
                else
                    return 0;
            }
            break;
#endif
#ifdef OS_CONFIG_USE_RMUTEX
        case OS_TASK_STATE_LOCKED_RMUTEX:
            {
                if (((struct os_rmutex_t *)lock->pobj)->nlock == 0)
                    return 1;
                else
                    return 0;
            }
            break;
#endif
        default:
            return 0;
    }
    return 0;
}
#endif /* OS_USE_LOCK */

