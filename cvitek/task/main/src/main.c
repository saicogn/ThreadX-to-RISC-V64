/*
 * FreeRTOS Kernel V10.3.0
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
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
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

/******************************************************************************
 *
 * See http://www.FreeRTOS.org/RTOS-Xilinx-UltraScale_MPSoC_64-bit.html for
 * additional information on this demo.
 *
 * NOTE 1:  This project provides two demo applications.  A simple blinky
 * style project, and a more comprehensive test and demo application.  The
 * RUN_TYPE in build.sh setting in main.c is used to select between the two.
 * See the notes on using RUN_TYPE in build.sh where it is defined below.
 *
 * NOTE 2:  This file only contains the source code that is not specific to
 * either the simply blinky or full demos - this includes initialisation code
 * and callback functions.
 *
 * NOTE 3:  This project builds the FreeRTOS source code, so is expecting the
 * BSP project to be configured as a 'standalone' bsp project rather than a
 * 'FreeRTOS' bsp project.  However the BSP project MUST still be build with
 * the FREERTOS_BSP symbol defined (-DFREERTOS_BSP must be added to the
 * command line in the BSP configuration).
 */
#define USE_THREADX

#ifdef __riscv
/* Standard includes. */
#include <stdio.h>
/* Scheduler include files. */
//#include "FreeRTOS.h"
//#include "task.h"

#else
#include "linux/types.h"
/* Scheduler include files. */
//#include "FreeRTOS_POSIX.h"
//#include "task.h"

/* Xilinx includes. */
//#include "xscugic.h"
//#if ( configUSE_TRACE_FACILITY == 1 )
//#include "trcRecorder.h"
//#endif

/* The interrupt controller is initialised in this file, and made available to
other modules. */
XScuGic xInterruptController;

#endif
// #include "sleep.h"

/* RUN_TYPE in build.sh is used to select between two demo applications,
 * as described at the top of this file.
 *
 * When RUN_TYPE is set to BLINKY_DEMO the simple blinky example will
 * be run.
 *
 * When RUN_TYPE is set to FULL_DEMO the comprehensive test and demo
 * application will be run.
 */

/*-----------------------------------------------------------*/

/*
 * Configure the hardware as necessary to run this demo.
 */
static void prvSetupHardware(void);

/*
 * See the comments at the top of this file and above the
 * RUN_TYPE in build.sh definition.
 */
#ifdef CVIRTOS
extern void main_cvirtos(void);
#else
#error Invalid RUN_TYPE setting in build.sh.  See the comments at the top of this file and above the RUN_TYPE definition.
#endif

int main(void)
{
	pre_system_init();
	printf("CVIRTOS Build Date:%s  (Time :%s) \n", __DATE__, __TIME__);
#ifndef __riscv
	mmu_enable();
	printf("enable I/D cache & MMU done\n");
#endif
	/* Configure the hardware ready to run the demo. */
	prvSetupHardware();
	post_system_init();

#ifdef CVIRTOS
	{
		main_cvirtos();
	}
#else
#error "Not correct running definition"
#endif

	/* Don't expect to reach here. */
	return 0;
}
/*-----------------------------------------------------------*/

static void prvSetupHardware(void)
{
#ifdef __riscv
#else
	BaseType_t xStatus;
	XScuGic_Config *pxGICConfig;

	/* Ensure no interrupts execute while the scheduler is in an inconsistent
	state.  Interrupts are automatically enabled when the scheduler is
	started. */
	portDISABLE_INTERRUPTS();

	/* Obtain the configuration of the GIC. */
	pxGICConfig = XScuGic_LookupConfig(XPAR_SCUGIC_SINGLE_DEVICE_ID);

	/* Sanity check the FreeRTOSConfig.h settings are correct for the
	hardware. */
	configASSERT(pxGICConfig);
	configASSERT(pxGICConfig->CpuBaseAddress ==
		     (configINTERRUPT_CONTROLLER_BASE_ADDRESS +
		      configINTERRUPT_CONTROLLER_CPU_INTERFACE_OFFSET));
	configASSERT(pxGICConfig->DistBaseAddress ==
		     configINTERRUPT_CONTROLLER_BASE_ADDRESS);

	/* Install a default handler for each GIC interrupt. */
	xStatus = XScuGic_CfgInitialize(&xInterruptController, pxGICConfig,
					pxGICConfig->CpuBaseAddress);
	configASSERT(xStatus == XST_SUCCESS);
	(void)xStatus; /* Remove compiler warning if configASSERT() is not defined. */
#endif
}
/*-----------------------------------------------------------*/

