/*
 *     Yet another operating system for microcontrollers.
 *     This file provide functions to implement support of ARMv7-M architecture
 *     (currently only Cortex-M3 tested).
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
#include <cm3.h>
#include <clk_cfg.h>
#include <debug.h>
#include "port.h"
#include "../../os.h" /* XXX remove ? */
#include "../../os_private.h"
#include "../../os_config.h"

void os_scheduler();

/* initial context of task */
const struct os_task_context_t icontext = {
    0x00004444, /* r4   */
    0x00005555, /* r5   */
    0x00006666, /* r6   */
    0x00007777, /* r7   */
    0x00008888, /* r8   */
    0x00009999, /* r9   */
    0x0000aaaa, /* r10  */
    0x0000bbbb, /* r11  */
    0x00000000, /* r0   */
    0x00001111, /* r1   */
    0x00002222, /* r2   */
    0x00003333, /* r3   */
    0x0000cccc, /* r12  */
    0x0000dddd, /* lr   */
    0x00000000, /* pc   */
    0x01000000, /* xpsr, NOTE set T bit */
};

/*
 *
 */
void port_init()
{
#ifdef OS_CONFIG_TICK_PERIOD
    /* initialize system tick timer */
    {
        SysTick->CTRL = 0; /* disable timer */
        /* NOTE CLK_CCLK should be multiple of MHz */
        #define SYSTICK_RELOAD ((OS_CONFIG_TICK_PERIOD * (CLK_CCLK / 1000000)) - 1)
        #if (SYSTICK_RELOAD < 1) || (SYSTICK_RELOAD > 0x00ffffff)
                #error "Imposible configuration of systick"
        #endif
        SysTick->RELOAD = (BASE_TYPE)SYSTICK_RELOAD;
        SysTick->CURR   = (BASE_TYPE)SYSTICK_RELOAD;
    }
#endif /* OS_CONFIG_TICK_PERIOD */

    /*
     * set priority groups, priority of interrupts
     *
     * NOTE interrupts already disabled in os_init()
     */
    {
        /*
         * group priorities  8  (bits [7:5])
         * subprioties       32 (bits [4:0])
         *
         * LPC17xx subpriorities 4 (bits [4:3], bits [2:0] are not used)
         *
         * XXX check that implementation support this value */
        SCB->AIRCR = (4 << 8) | (0x05FA << 16);

        /* set PendSV lowest priority, SysTick highest priority (but not higher than debug port)  */
        SCB->SHP[10] = 0xff;
#ifdef OS_CONFIG_TICK_PERIOD
        SCB->SHP[11] = (1 << 5) | (0 << 3);
#endif
    }
}

/*
 *
 */
void port_start()
{
    asm volatile (
        /* set PSP to point to idle task's TOS */
        "ldr r0, current_taskcb                  \n"
        "ldr r1, [r0]                            \n"
        "ldr r1, [r1, #0]                        \n"
        "msr psp, r1                             \n"
        /* set PSP as thread stack pointer */
        "mov r1,#2                               \n"
        "msr control, r1                         \n"
    );
#ifdef OS_CONFIG_TICK_PERIOD
    /*
     * Start system tick timer, enable it's interrupt.
     * NOTE RELOAD value should be enough to not generate
     * interrupt before we go to "os_idle_process".
     */
    SysTick->CTRL = SysTick_CTRL_ENABLE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_CLKSOURCE_Msk; 
#endif
    /* enable interrupts previosly disabled in os_init() */
    PORT_ENABLE_IRQ();
    asm volatile (
        "b os_idle_process                       \n"
      	".align 2                                \n"
	"current_taskcb: .word os_current_taskcb \n"
    );
}

/*
 * XXX should macro be used instead?
 */
void __attribute__((naked)) port_task_switch()
{
    /* set PendSV to pending */
    asm volatile (
        ".equ ICSR, 0xE000ED04  \n"
        ".equ PENDBIT, (1 << 28)\n"
        "ldr r0,=ICSR           \n"
        "ldr r1,=PENDBIT        \n"
        "str r1,[r0]            \n"
        "dsb                    \n"
        "isb                    \n"
        "bx  lr                 \n"
    );
}

/*
 *
 */
void __attribute__((naked)) PendSV_Handler(void)
{
    PORT_DISABLE_IRQ();
    asm volatile (
        /* store software context */
        "mrs r0, psp        \n"
        "stmdb r0!,{r4-r11} \n"
        /* save new TOS */
        "ldr r2, current_taskcb \n"
        "ldr r2, [r2, #0]       \n"
        "str r0, [r2, #0]       \n"

        "push {lr}              \n"

        /* call scheduler */
        "bl os_scheduler  \n"

        "pop {lr}              \n"

        /* restore context of new task */
        "ldr r2, current_taskcb \n"
        "ldr r2, [r2, #0]       \n"
        "ldr r0, [r2, #0]       \n"
        "ldmia r0!, {r4-r11}    \n"
        /* move PSP to new task's stack */
        "msr psp, r0            \n"
    );
    PORT_ENABLE_IRQ();
    asm volatile (
        "bx lr\n"
    );
}

/*
 * check whether call done from irq or not
 *
 * RETURN
 *     0   thread mode
 *     >0  exception number in active state
 */
BASE_TYPE port_in_irq()
{
    return (SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk);
} 

/*
 *
 */
void SVCall_Handler(void)
{


}

/*
 *
 */
void SysTick_Handler(void)
{
#ifdef OS_CONFIG_TICK_PERIOD
    os_tick();
#endif
}

