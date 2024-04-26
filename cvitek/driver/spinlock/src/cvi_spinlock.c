// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2022. All rights reserved.
 *
 * File Name: cvi_spinlock.c
 * Description:
 */

#ifdef FREERTOS_BSP
#include "stdint.h"
#include "types.h"
#include "csr.h"
#include "csi_rv64_gcc.h"
#include "core_rv64.h"
#include "arch_time.h"
#include "top_reg.h"
#include "cvi_spinlock.h"
#include "delay.h"
#include "mmio.h"
#else
#include <linux/time.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/of_reserved_mem.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/io.h>
#include "cvi_spinlock.h"
#endif

#ifndef FREERTOS_BSP
static unsigned long reg_base;
#else
static unsigned long reg_base = SPINLOCK_REG_BASE;
#endif

// FREERTOS_BSP defined in cvitek/scripts/toolchain-riscv64-elf.cmake, set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DFREERTOS_BSP" )
#define USE_THREADX_SEMAPHORE 1
#if USE_THREADX_SEMAPHORE == 0
#include "semphr.h"
SemaphoreHandle_t reg_write_lock = NULL;
#else
#include "tx_api.h"
TX_SEMAPHORE reg_write_lock;
#endif

static unsigned char lockCount[SPIN_MAX+1] = {0};

#if USE_THREADX_SEMAPHORE == 0
void cvi_spinlock_init()
{
	
	reg_write_lock = xSemaphoreCreateBinary();
	if(reg_write_lock == NULL)
		printf("xSemaphoreCreateBinary failed!\n");

	xSemaphoreGive(reg_write_lock);
	printf("[%s] succeess\n" , __func__);
}

void cvi_spinlock_deinit(void)
{
    vSemaphoreDelete(reg_write_lock);
}
#else

void cvi_spinlock_init()
{
	if (tx_semaphore_create(&reg_write_lock, "cvi_spinlock", 1) != TX_SUCCESS)
	{
		//printf("tx_semaphore_create failed!\n");
		uart_puts("tx_semaphore_create failed\n");
	}
	else
	{
		uart_puts("tx_semaphore_create succeess from threadx\n");
	}
}

void cvi_spinlock_deinit(void)
{
	tx_semaphore_delete(&reg_write_lock);
}

#endif

void spinlock_base(unsigned long mb_base)
{
	reg_base = mb_base;
}

static inline int hw_spin_trylock(hw_raw_spinlock_t *lock)
{
#ifndef RISCV_QEMU

	writew(lock->locks, reg_base + sizeof(int) * lock->hw_field);
	if (readw(reg_base + sizeof(int) * lock->hw_field) == lock->locks)
		return MAILBOX_LOCK_SUCCESS;
	return MAILBOX_LOCK_FAILED;
#else
	return MAILBOX_LOCK_SUCCESS;
#endif
}

int hw_spin_lock(hw_raw_spinlock_t *lock)
{
	u64 i;
	u64 loops = 1000000;
	hw_raw_spinlock_t _lock = {.hw_field = lock->hw_field, .locks = lock->locks};

	if (lock->hw_field >= SPIN_LINUX_RTOS)
	{
		#if USE_THREADX_SEMAPHORE == 0
		xSemaphoreTakeFromISR(reg_write_lock , NULL);
		#else
		if (tx_semaphore_get(&reg_write_lock, TX_NO_WAIT) != TX_SUCCESS)
		{
			uart_puts("tx_semaphore_get failed\n");
		}
		#endif
		
		if (lockCount[lock->hw_field] == 0) {
			lockCount[lock->hw_field]++;
		}
		_lock.locks = (lockCount[lock->hw_field] << 8);
		lockCount[lock->hw_field]++;
		
		#if USE_THREADX_SEMAPHORE == 0
		xSemaphoreGiveFromISR(reg_write_lock , NULL);
		#else
		if (tx_semaphore_put(&reg_write_lock) != TX_SUCCESS)
		{
			uart_puts("tx_semaphore_put failed\n");
		}
		#endif
	}
	else {
		unsigned long systime = GetSysTime();
		/* lock ID can not be 0, so set it to 1 at least */
		if ((systime & 0xFFFF) == 0)
			systime = 1;
		lock->locks = (unsigned short) (systime & 0xFFFF);
	}
	for (i = 0; i < loops; i++) {
		if (hw_spin_trylock(&_lock) == MAILBOX_LOCK_SUCCESS)
		{
			lock->locks = _lock.locks;
			return MAILBOX_LOCK_SUCCESS;
		}
		udelay(1);
	}

#ifdef FREERTOS_BSP
	uart_puts("__spin_lock_debug fail\n");
#else
	pr_err("__spin_lock_debug fail\n");
#endif
	return MAILBOX_LOCK_FAILED;
}

int _hw_raw_spin_lock_irqsave(hw_raw_spinlock_t *lock)
{
	int flag = 0;

#ifdef FREERTOS_BSP
	// save and disable irq
	flag = (__get_MSTATUS() & 8);
	__disable_irq();
#endif

	// lock
	if (hw_spin_lock(lock) == MAILBOX_LOCK_FAILED) {
		#ifdef FREERTOS_BSP
		// if spinlock failed , restore irq
		if (flag) {
			__enable_irq();
		}
		#endif
		uart_puts("spin lock fail! reg_val=0x%x, lock->locks=0x%x\n",
				readw(reg_base + sizeof(int) * lock->hw_field), lock->locks);
		return MAILBOX_LOCK_FAILED;
	}
	return flag;
}

void _hw_raw_spin_unlock_irqrestore(hw_raw_spinlock_t *lock, int flag)
{

#ifndef RISCV_QEMU
	// unlock
	if (readw(reg_base + sizeof(int) * lock->hw_field) == lock->locks) {
		writew(lock->locks, reg_base + sizeof(int) * lock->hw_field);

		#ifdef FREERTOS_BSP
		// restore irq
		if (flag) {
			__enable_irq();
		}
		#endif
	} else {

	#ifdef FREERTOS_BSP
		uart_puts("spin unlock fail! reg_val=0x%x, lock->locks=0x%x\n",
				readw(reg_base + sizeof(int) * lock->hw_field), lock->locks);
	#else
		pr_err("spin unlock fail\n");
	#endif
	}
#else
	if (flag) {
		__enable_irq();
	}

#endif
}
