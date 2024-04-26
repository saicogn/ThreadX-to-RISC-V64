/*
 * FreeRTOS Kernel V10.4.6
 * Copyright (C) 2021 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 *
 */

// setup timer interrupt(mtime)
#include "tx_port.h"

#include "printf.h"

uint64_t ullNextTime = 0ULL;
const uint64_t *pullNextTime = &ullNextTime;

const size_t uxTimerIncrementsForOneTick = (size_t)((configSYS_CLOCK_HZ) / (configTICK_RATE_HZ)); /* Assumes increment won't go over 32-bits. */

uint32_t const ullMachineTimerCompareRegisterBase = configMTIMECMP_BASE_ADDRESS;

#if (defined THEAD_C906) && (!defined configMTIME_BASE_ADDRESS) && (configMTIMECMP_BASE_ADDRESS != 0)
volatile uint32_t *pulMachineTimerCompareRegisterL = NULL;
volatile uint32_t *pulMachineTimerCompareRegisterH = NULL;
#else
volatile uint64_t *pullMachineTimerCompareRegister = NULL;
#error define failed---------------------------------
#endif

void port_specific_pre_initialization(void)
{

    asm volatile("csrc mstatus, %0" ::"r"(0x08));  // disable enterrupt

// setup time
#ifdef THEAD_C906
    /* If there is a CLINT then it is ok to use the default implementation
    in this file, otherwise vPortSetupTimerInterrupt() must be implemented to
    configure whichever clock is to be used to generate the tick interrupt. */
    uint64_t ullCurrentTime;
    volatile uint32_t ulHartId;
    asm volatile("csrr %0, mhartid" : "=r"(ulHartId));

    // 32bit IO bus, need to get hi/lo seperately
    pulMachineTimerCompareRegisterL =
        (volatile uint32_t *)(ullMachineTimerCompareRegisterBase + (ulHartId * sizeof(uint64_t)));
    pulMachineTimerCompareRegisterH =
        (volatile uint32_t *)(ullMachineTimerCompareRegisterBase + sizeof(uint32_t) + (ulHartId * sizeof(uint64_t)));

    asm volatile("rdtime %0" : "=r"(ullCurrentTime));

    ullNextTime = (uint64_t)ullCurrentTime;
    ullNextTime += (uint64_t)uxTimerIncrementsForOneTick;
    *pulMachineTimerCompareRegisterL = (uint32_t)(ullNextTime & 0xFFFFFFFF);
    *pulMachineTimerCompareRegisterH = (uint32_t)(ullNextTime >> 32);

    /* Prepare the time to use after the next tick interrupt. */
    ullNextTime += (uint64_t)uxTimerIncrementsForOneTick;
#endif

// enable mtime
#if ((configMTIME_BASE_ADDRESS != 0) && (configMTIMECMP_BASE_ADDRESS != 0) || (defined THEAD_C906))
    {
        /* Enable mtime and external interrupts.  1<<7 for timer interrupt, 1<<11
        for external interrupt.  _RB_ What happens here when mtime is not present as
        with pulpino? */
        asm volatile("csrs mie, %0" ::"r"(0x880));
    }
#else
    {
        /* Enable external interrupts. */
        __asm volatile("csrs mie, %0" ::"r"(0x800));
    }
#endif /* ( configMTIME_BASE_ADDRESS != 0 ) && ( configMTIMECMP_BASE_ADDRESS != 0 ) */

    printf("pre-initialization\n");
}
