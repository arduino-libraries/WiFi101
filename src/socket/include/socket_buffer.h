/**
 *
 * \file
 *
 * \brief BSD compatible socket interface.
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

#ifndef __SOCKET_BUFFER_H__
#define __SOCKET_BUFFER_H__

#include <stdint.h>

#include "socket/include/socket.h"

#ifdef  __cplusplus
extern "C" {
#endif

#define SOCKET_BUFFER_UDP_HEADER_SIZE			(8)

#if defined LIMITED_RAM_DEVICE
#define SOCKET_BUFFER_MTU						(16u)
#define SOCKET_BUFFER_UDP_SIZE					(SOCKET_BUFFER_UDP_HEADER_SIZE + 64u)
#define SOCKET_BUFFER_TCP_SIZE					(64u)
#else
#define SOCKET_BUFFER_MTU						(1400u)
#define SOCKET_BUFFER_UDP_SIZE					(SOCKET_BUFFER_MTU * 2)
#define SOCKET_BUFFER_TCP_SIZE					(SOCKET_BUFFER_MTU * 2)
#endif

void socketBufferInit(void);
void socketBufferCb(SOCKET sock, uint8 u8Msg, void *pvMsg);

sint8 socketBufferIsFull(SOCKET sock);
sint8 socketBufferIsConnected(SOCKET sock);
sint8 socketBufferIsSpawned(SOCKET sock);
sint8 socketBufferHasParent(SOCKET sock, SOCKET parent);
sint16 socketBufferDataAvailable(SOCKET sock);

void socketBufferSetupBuffer(SOCKET sock);
sint8 socketBufferBindWait(SOCKET sock);
sint8 socketBufferConnectWait(SOCKET sock);
void socketBufferSendWait(SOCKET sock);
void socketBufferReadUdpHeader(SOCKET sock, uint16_t* rcvSize, uint16_t* rcvPort, uint32_t* rcvIP);
uint8 socketBufferPeek(SOCKET sock);
sint16 socketBufferRead(SOCKET sock, uint8 *buf, uint16 len);
void socketBufferClearSpawned(SOCKET sock);
void socketBufferClose(SOCKET sock);

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* __SOCKET_BUFFER_H__ */
