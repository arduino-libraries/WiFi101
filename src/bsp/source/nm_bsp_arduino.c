/**
 *
 * \file
 *
 * \brief This module contains SAMD21 BSP APIs implementation.
 *
 * Copyright (c) 2014 Atmel Corporation. All rights reserved.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * 4. This software may only be redistributed and used in connection with an
 *    Atmel microcontroller product.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \asf_license_stop
 *
 */

#include "bsp/include/nm_bsp.h"
#include "common/include/nm_common.h"
#include <Arduino.h>

/*
 * Arduino variants may redefine those pins.
 * If no pins are specified the following defaults are used:
 *  WINC1501_RESET_PIN   - pin 5
 *  WINC1501_INTN_PIN    - pin 7
 *  WINC1501_CHIP_EN_PIN - not connected (tied to VCC)
 */
#if !defined(WINC1501_RESET_PIN)
  #define WINC1501_RESET_PIN  5
#endif
#if !defined(WINC1501_INTN_PIN)
  #define WINC1501_INTN_PIN   7
#endif

static tpfNmBspIsr gpfIsr;

void __attribute__((weak)) attachInterruptMultiArch(uint32_t pin, void *chip_isr, uint32_t mode)
{
	attachInterrupt(pin, chip_isr, mode);
}

void __attribute__((weak)) detachInterruptMultiArch(uint32_t pin)
{
	detachInterrupt(pin);
}

static void chip_isr(void)
{
	if (gpfIsr) {
		gpfIsr();
	}
}

/*
 *	@fn		init_chip_pins
 *	@brief	Initialize reset, chip enable and wake pin
 *	@author	M.S.M
 *	@date	11 July 2012
 *	@version	1.0
 */
static void init_chip_pins(void)
{
	/* Configure RESETN D6 pins as output. */
	pinMode(WINC1501_RESET_PIN, OUTPUT);
	digitalWrite(WINC1501_RESET_PIN, HIGH);

#if defined(WINC1501_CHIP_EN_PIN)
	/* Configure CHIP_EN as pull-up */
	pinMode(WINC1501_CHIP_EN_PIN, INPUT_PULLUP);
#endif
}

/*
 *	@fn		nm_bsp_init
 *	@brief	Initialize BSP
 *	@return	0 in case of success and -1 in case of failure
 *	@author	M.S.M
 *	@date	11 July 2012
 *	@version	1.0
 */
sint8 nm_bsp_init(void)
{
	gpfIsr = NULL;

	init_chip_pins();

	nm_bsp_reset();

	return M2M_SUCCESS;
}

/**
 *	@fn		nm_bsp_deinit
 *	@brief	De-iInitialize BSP
 *	@return	0 in case of success and -1 in case of failure
 *	@author	M. Abdelmawla
 *	@date	11 July 2012
 *	@version	1.0
 */
sint8 nm_bsp_deinit(void)
{
	return M2M_SUCCESS;
}

/**
 *	@fn		nm_bsp_reset
 *	@brief	Reset NMC1500 SoC by setting CHIP_EN and RESET_N signals low,
 *           CHIP_EN high then RESET_N high
 *	@author	M. Abdelmawla
 *	@date	11 July 2012
 *	@version	1.0
 */
void nm_bsp_reset(void)
{
	digitalWrite(WINC1501_RESET_PIN, LOW);
	nm_bsp_sleep(100);
	digitalWrite(WINC1501_RESET_PIN, HIGH);
	nm_bsp_sleep(100);
}

/*
 *	@fn		nm_bsp_sleep
 *	@brief	Sleep in units of mSec
 *	@param[IN]	u32TimeMsec
 *				Time in milliseconds
 *	@author	M.S.M
 *	@date	28 OCT 2013
 *	@version	1.0
 */
void nm_bsp_sleep(uint32 u32TimeMsec)
{
	while (u32TimeMsec--) {
		delay(1);
	}
}

/*
 *	@fn		nm_bsp_register_isr
 *	@brief	Register interrupt service routine
 *	@param[IN]	pfIsr
 *				Pointer to ISR handler
 *	@author	M.S.M
 *	@date	28 OCT 2013
 *	@sa		tpfNmBspIsr
 *	@version	1.0
 */
void nm_bsp_register_isr(tpfNmBspIsr pfIsr)
{
	gpfIsr = pfIsr;
	attachInterruptMultiArch(WINC1501_INTN_PIN, chip_isr, FALLING);
}

/*
 *	@fn		nm_bsp_interrupt_ctrl
 *	@brief	Enable/Disable interrupts
 *	@param[IN]	u8Enable
 *				'0' disable interrupts. '1' enable interrupts
 *	@author	M.S.M
 *	@date	28 OCT 2013
 *	@version	1.0
 */
void nm_bsp_interrupt_ctrl(uint8 u8Enable)
{
	if (u8Enable) {
		attachInterruptMultiArch(WINC1501_INTN_PIN, chip_isr, FALLING);
	} else {
		detachInterruptMultiArch(WINC1501_INTN_PIN);
	}
}
