/*
 *     Yet another operating system for microcontrollers.
 *     This file contain basic functions of OS and functions that does not fit to
 *     other category.
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
#include "os.h"
#include "os_private.h"
#include "os_sched.h"

/* NOTE bss should be initialized with zero */

BASE_TYPE os_taskidx;                           /* number of current initialized tasks */
struct os_trap_info_t os_trapinfo;              /* trap inforamtion */
struct os_taskcb_t os_tasks[OS_CONFIG_TASK_COUNT + 1]; /* control block of task, NOTE os_task[0] is control block of idle task */
volatile struct os_taskcb_t *os_current_taskcb; /* pointer to current task's control block */
#ifdef OS_CONFIG_USE_SCHEDLOCK
volatile struct os_taskcb_t *os_schedlock_taskcb; /* pointer to task that possessed scheduler  */
#endif


#define IDLE_STACK       idle_stack
#define IDLE_STACK_SIZE  (OS_STACK_MINSIZE + 16 /* XXX */)
#define IDLE_PROCESS     os_idle_process
STATIC uint8 idle_stack[IDLE_STACK_SIZE] __attribute__((section("tstack")));
STATIC void os_idle_process();

BASE_TYPE irqd; /* interrupt disable counter */

/*
 *
 */
void os_init()
{
    OS_DISABLE_IRQ(); /* NOTE disable interrupts during initialization */
    port_init();

#ifdef OS_CONFIG_TASK_NAME_SIZE
    OS_TASK_INIT("IDLE", IDLE_STACK, IDLE_STACK_SIZE, 0, IDLE_PROCESS, NULL);
#else
    OS_TASK_INIT(NULL, IDLE_STACK, IDLE_STACK_SIZE, 0, IDLE_PROCESS, NULL);
#endif
    os_current_taskcb = &os_tasks[0];
}

/*
 *
 */
void os_start()
{
    port_start();
    /* UNREACHED */
}

/*
 * initialize task
 *
 * ARGS
 *   stack       pointer to stack
 *   ssize       stack size
 *   priority    task's priority (if used, lower number - higher priority)
 *   process     task routine
 *
 * RETURN
 *   pointer to task's control block
 *
 */
#ifdef OS_CONFIG_USE_PRIORITY
struct os_taskcb_t* os_task_init(char *name, uint8 *stack, BASE_TYPE ssize, BASE_TYPE priority, void (*process)(), void *context)
#else
struct os_taskcb_t* os_task_init(char *name, uint8 *stack, BASE_TYPE ssize, void (*process)(), void *context)
#endif
{
    struct os_taskcb_t *task;
    struct os_task_context_t *ct;

    if (os_taskidx > OS_CONFIG_TASK_COUNT)
    {
        os_trapinfo.err = OS_TRAP_ERR_TASKCOUNT;
        os_trap();
    }

    if (ssize < OS_STACK_MINSIZE)
    {
        os_trapinfo.err = OS_TRAP_ERR_STACKMIN;
        os_trap();
    }

    task = &os_tasks[os_taskidx];

    DEBUG_OS_HEX_WRAP("task = ", (uint32*)&task, 4);
    /* move pointer to next TCB */
    os_taskidx++;

    /* save context to control block */
    task->context = context;

#ifdef OS_CONFIG_USE_TRACE
    task->process = process;
    task->stack   = stack;
    task->ssize   = ssize;
#endif

#ifdef OS_CONFIG_TASK_NAME_SIZE
    {
        BASE_TYPE len;
        char *s, *d;

        if (name)
        {
            s = name;
            d = task->name;

            len = OS_CONFIG_TASK_NAME_SIZE - 1;
            while (len-- && *s)
            {
                *d++ = *s++;
            }
            *d = 0;
        }
    }
#endif

#ifdef OS_CONFIG_USE_PRIORITY
    task->priority = priority;
#endif
#if OS_USE_LOCK
    /* if task is not idle task (first) then add it to scheduler queue */
    if (os_taskidx > 1) 
        os_squeue_addtask(task);
#endif
#ifdef OS_STACK_DIR_DECREASE
    task->tos = (uint8*)((BASE_TYPE)stack + ssize);
    if (os_taskidx > 1) 
        task->tos -= OS_CONTEXT_SIZE;
#else
    task->tos = stack;
#endif
    if (os_taskidx > 1) 
    {
        BASE_TYPE clen;
        uint8 *pcontext, *picontext;

        /* fill task stack with initial context */
        ct = (struct os_task_context_t *)task->tos;

        pcontext  = task->tos;
        picontext = (uint8*)&icontext;
        clen = sizeof(struct os_task_context_t);

        while (clen--) 
            *(pcontext++) = *(picontext++);

        /* place address of task's process to context */
#if PORT_ARMV7M
        ct->pc   = (BASE_TYPE)process | 0x01;
        ct->lr   = ct->pc;
#endif
    }

//#ifdef XC16 /* XXX means 16-bit PIC microcontrollers */
//    ct->pc15_0  = (uint16)process;
//    /* XXX if task address exceed 16-bit then compiller should use jump table */
//    ct->pc22_16 = 0;
//#else
//    #error (E) Architecture not supported.
//#endif
//

#ifndef OS_STACK_DIR_DECREASE
    task->tos = stack + sizeof(struct os_task_context_t);
#endif

#ifdef OS_CONFIG_USE_TRACE
    #ifdef OS_STACK_DIR_DECREASE
        task->tos_max = (uint8*)BASE_TYPE_MAX;
    #else
        task->tos_max = (uint8*)0;
    #endif
#endif

#ifdef OS_CONFIG_USE_TASK_SLICE
    if (os_taskidx > 1) 
    {
        task->tslice = OS_CONFIG_DEFAULT_TASK_SLICE;
    #ifdef OS_CONFIG_USE_VARIABLE_TASK_SLICE
        task->dslice = OS_CONFIG_DEFAULT_TASK_SLICE;
    #endif
    }
#endif

    return task;
}

/*
 * obtain context of current task
 */
void *os_task_get_context()
{
    return os_current_taskcb->context;
}

#ifdef OS_CONFIG_TICK_PERIOD
/*
 * for preemption and functions with timeout
 *
 * NOTE os_tick should be performed on timer with maximum priority
 */
void os_tick()
{
    BASE_TYPE suspend;
    BASE_TYPE *timeout;

#ifdef OS_CONFIG_USE_TRACE
    os_current_taskcb->ticks++;     /* time of task in running state (system ticks) */
#endif

    suspend = 0;
#ifdef OS_USE_TIMEOUT
    {
        struct os_taskcb_t *ptask;

        ptask = &os_tasks[1];
        while (ptask < &os_tasks[os_taskidx])
        {
            timeout = &ptask->timeout;
            if ((*timeout > 0) && (*timeout < OS_TIMEOUT_EXPIRED))
            {
                (*timeout)--;
                if ((*timeout) == 0)
                {
                    *timeout = OS_TIMEOUT_EXPIRED;
#ifdef OS_CONFIG_USE_VPREEMPT
                    /*
                     * NOTE idle task preempted, be care of critical sections
                     * in idle task if present
                     */
                    if (os_current_taskcb == OS_IDLE_TASKCB)
                        suspend = 1;
                    else
                        os_current_taskcb->vpreempt = 1;
#else
                    suspend = 1;
#endif
                }
            }
            ptask++;
        }
    }
#endif

#ifdef OS_CONFIG_USE_TASK_SLICE
    /* 
     * Check for expire of task's time slice (if not idle task).
     * Tick slice only if task is running.
     */
    if (
#ifdef OS_CONFIG_USE_VPREEMPT
        !os_current_taskcb->vpreempt &&
#endif
        os_current_taskcb != OS_IDLE_TASKCB &&
        os_current_taskcb->lock.state == OS_TASK_STATE_RUN)
    {
        timeout = (BASE_TYPE*)&os_current_taskcb->tslice;
        if (*timeout)
        {
            (*timeout)--;
            if ((*timeout) == 0)
            {
                os_respawn_tslice(os_current_taskcb);
#ifdef OS_CONFIG_USE_VPREEMPT
                os_current_taskcb->vpreempt = 1;
#else
#if OS_USE_LOCK
                os_current_taskcb->lock.state = OS_TASK_STATE_SUSPEND;
#endif
                os_task_switch();
                /* 
                 * Task is suspended now. Nothing to do anymore - return.
                 */
                return;
#endif /* OS_CONFIG_USE_VPREEMPT */
            }
        }
    }
#endif

    if (suspend)
    {
#if OS_USE_LOCK
        os_sched_suspend_task();
#else
        os_task_switch();
#endif
    }
}

#ifdef OS_CONFIG_USE_TASK_SLICE
/*
 * respawn time slice of task
 */
inline void os_respawn_tslice(volatile struct os_taskcb_t *task)
{
    #ifdef OS_CONFIG_USE_VARIABLE_TASK_SLICE
        task->tslice = task->dslice;
    #else
        task->tslice = OS_CONFIG_DEFAULT_TASK_SLICE;
    #endif
}
#endif /* OS_CONFIG_USE_TASK_SLICE */

#endif /* OS_CONFIG_TICK_PERIOD */

/*
 * uncoditionally give control to other task
 */
void os_yield()
{
#ifdef OS_CONFIG_USE_VPREEMPT
    OS_DISABLE_IRQ();
    os_current_taskcb->vpreempt = 0;
    OS_ENABLE_IRQ();
#endif

#if OS_USE_LOCK
    os_lock_task(OS_TASK_STATE_SUSPEND, NULL, 0, 0);
#else
    os_task_switch();
#endif
}

#ifdef OS_CONFIG_USE_VPREEMPT
/*
 * voulantary yield
 */
void os_vyield()
{
    BASE_TYPE yield;
    yield = 0;
    OS_DISABLE_IRQ();
    {
        if (os_current_taskcb->vpreempt)
            yield = 1;
    }
    OS_ENABLE_IRQ();
    if (yield)
        os_yield();
}
#endif

#ifdef OS_CONFIG_USE_WAIT
/*
 * suspend task for some time interval
 *
 * ARGS
 *     ticks    specify time to wait
 */
void os_wait(BASE_TYPE ticks)
{
    os_lock_task(OS_TASK_STATE_LOCKED_WAIT, NULL, 0, ticks);
}
#endif

#if (defined OS_CONFIG_USE_TASK_SLICE) && (defined OS_CONFIG_USE_VARIABLE_TASK_SLICE)
/*
 * set reload value of time slice of current task
 * NOTE also refresh current remain slice of task
 *
 * ARGS
 *     ticks    new reaload value of time slice
 */
void os_set_slice(BASE_TYPE ticks)
{
    OS_DISABLE_IRQ();
    {
        os_current_taskcb->tslice = ticks;
        os_current_taskcb->dslice = ticks;
    }
    OS_ENABLE_IRQ();
}
#endif

extern uint32 tdmatimer;
/*
 *
 */
STATIC void os_idle_process()
{
    os_task_switch();
    while (1)
    {
#if OS_USE_LOCK
        asm volatile ("wfi\r\n");
#else
        os_task_switch();
#endif
    }
}

/*
 *
 */
inline void os_disable_irq()
{
    OS_DISABLE_IRQ();
}

/*
 *
 */
inline void os_enable_irq()
{
    OS_ENABLE_IRQ();
}

/*
 *
 */
void os_disable_irq_r()
{
    OS_DISABLE_IRQ();
    irqd++;
}

/*
 *
 */
void os_enable_irq_r()
{
    irqd--;
    if (irqd == 0)
        OS_ENABLE_IRQ();
}

/*
 * 
 */
void os_trap()
{
    OS_DISABLE_IRQ();
    dprint("ns1xn", "(OS TRAP) err = ", os_trapinfo.err);
    while(1);
}

