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

#include <stdlib.h>	
#include <string.h>
#include "socket/include/socket.h"
#include "driver/source/m2m_hif.h"
#include "socket/source/socket_internal.h"
#include "socket/include/socket_buffer.h"
#include "driver/include/m2m_periph.h"
#include "driver/include/m2m_wifi.h"

#define SOCKET_BUFFER_FLAG_CONNECTING			(0x1 << 0)
#define SOCKET_BUFFER_FLAG_CONNECTED			(0x1 << 1)
#define SOCKET_BUFFER_FLAG_FULL					(0x1 << 2)
#define SOCKET_BUFFER_FLAG_BIND					(0x1 << 4)
#define SOCKET_BUFFER_FLAG_BINDING				(0x1 << 5)
#define SOCKET_BUFFER_FLAG_SPAWN				(0x1 << 6)
#define SOCKET_BUFFER_FLAG_SENDING				(0x1 << 7)

typedef struct{
	uint8		*buffer;
	uint32		flag;
	SOCKET		parent;
	uint32		head;
	uint32		tail;
}tstrSocketBuffer;

tstrSocketBuffer gastrSocketBuffer[MAX_SOCKET];

extern uint8 hif_small_xfer;

void socketBufferInit(void)
{
	memset(gastrSocketBuffer, 0, sizeof(gastrSocketBuffer));

	SOCKET s;

	for (s = 0; s < MAX_SOCKET; s++) {
		gastrSocketBuffer[s].parent = -1;
	}
}

sint8 socketBufferIsFull(SOCKET sock)
{
	return (gastrSocketBuffer[sock].flag & SOCKET_BUFFER_FLAG_FULL) != 0;
}

sint8 socketBufferIsConnected(SOCKET sock)
{
	return (gastrSocketBuffer[sock].flag & SOCKET_BUFFER_FLAG_CONNECTED) != 0;
}

sint8 socketBufferIsSpawned(SOCKET sock)
{
	return (gastrSocketBuffer[sock].flag & SOCKET_BUFFER_FLAG_SPAWN) != 0;
}

sint8 socketBufferHasParent(SOCKET sock, SOCKET parent)
{
	return (gastrSocketBuffer[sock].parent == parent);
}

void socketBufferSetupBuffer(SOCKET sock)
{
	uint32 size = (sock < TCP_SOCK_MAX) ? SOCKET_BUFFER_TCP_SIZE : SOCKET_BUFFER_UDP_SIZE;

	gastrSocketBuffer[sock].buffer = (uint8*)realloc(gastrSocketBuffer[sock].buffer, size);

	gastrSocketBuffer[sock].head = gastrSocketBuffer[sock].tail = 0;
}

sint16 socketBufferDataAvailable(SOCKET sock)
{
	return (gastrSocketBuffer[sock].head - gastrSocketBuffer[sock].tail);
}

sint8 socketBufferBindWait(SOCKET sock)
{
	gastrSocketBuffer[sock].parent = -1;
	gastrSocketBuffer[sock].flag = SOCKET_BUFFER_FLAG_BINDING;

	while (gastrSocketBuffer[sock].flag & SOCKET_BUFFER_FLAG_BINDING) {
		m2m_wifi_handle_events(NULL);
	}

	return (gastrSocketBuffer[sock].flag & SOCKET_BUFFER_FLAG_BIND) != 0;
}

sint8 socketBufferConnectWait(SOCKET sock)
{
	gastrSocketBuffer[sock].parent = -1;
	gastrSocketBuffer[sock].flag = SOCKET_BUFFER_FLAG_CONNECTING;

	while (gastrSocketBuffer[sock].flag & SOCKET_BUFFER_FLAG_CONNECTING) {
		m2m_wifi_handle_events(NULL);
	}

	return (gastrSocketBuffer[sock].flag & SOCKET_BUFFER_FLAG_CONNECTED) != 0;
}

void socketBufferSendWait(SOCKET sock)
{
	gastrSocketBuffer[sock].flag = SOCKET_BUFFER_FLAG_SENDING;

	while (gastrSocketBuffer[sock].flag & SOCKET_BUFFER_FLAG_SENDING) {
		m2m_wifi_handle_events(NULL);
	}
}

void socketBufferReadUdpHeader(SOCKET sock, uint16_t* rcvSize, uint16_t* rcvPort, uint32_t* rcvIP)
{
	*rcvSize = ((uint16)gastrSocketBuffer[sock].buffer[gastrSocketBuffer[sock].tail] << 8) + (uint16)gastrSocketBuffer[sock].buffer[gastrSocketBuffer[sock].tail + 1];
	*rcvPort = ((uint16)gastrSocketBuffer[sock].buffer[gastrSocketBuffer[sock].tail + 2] << 8) + (uint16)gastrSocketBuffer[sock].buffer[gastrSocketBuffer[sock].tail + 3];
	*rcvIP =   ((uint32)gastrSocketBuffer[sock].buffer[gastrSocketBuffer[sock].tail + 4] << 24) + ((uint32)gastrSocketBuffer[sock].buffer[gastrSocketBuffer[sock].tail + 5] << 16) +
				((uint32)gastrSocketBuffer[sock].buffer[gastrSocketBuffer[sock].tail + 6] << 8) + (uint32)gastrSocketBuffer[sock].buffer[gastrSocketBuffer[sock].tail + 7];

	gastrSocketBuffer[sock].tail += SOCKET_BUFFER_UDP_HEADER_SIZE;
}

uint8 socketBufferPeek(SOCKET sock)
{
	return gastrSocketBuffer[sock].buffer[gastrSocketBuffer[sock].tail];
}

sint16 socketBufferRead(SOCKET sock, uint8 *buf, uint16 len)
{
	sint16 size_tmp = socketBufferDataAvailable(sock);

	if (size_tmp == 0) {
		return -1;
	}

	if ((sint16)len < size_tmp) {
		size_tmp = len;
	}

	sint16 i;

	for (i = 0; i < size_tmp; ++i) {
		buf[i] = gastrSocketBuffer[sock].buffer[gastrSocketBuffer[sock].tail++];

		if (gastrSocketBuffer[sock].tail == gastrSocketBuffer[sock].head) {
			// the full buffered data has been read, reset head and tail for next transfer
			gastrSocketBuffer[sock].tail = gastrSocketBuffer[sock].head = 0;

			// clear the buffer full flag
			gastrSocketBuffer[sock].flag &= ~SOCKET_BUFFER_FLAG_FULL;

			if (sock < TCP_SOCK_MAX) {
				// TCP
				recv(sock, gastrSocketBuffer[sock].buffer, SOCKET_BUFFER_MTU, 0);
			} else {
				// UDP

				// setup buffer and buffer size to transfer the remainder of the current packet
				// or next received packet
				if (hif_small_xfer) {
					recvfrom(sock, gastrSocketBuffer[sock].buffer, SOCKET_BUFFER_MTU, 0);
				} else {
					recvfrom(sock, gastrSocketBuffer[sock].buffer + SOCKET_BUFFER_UDP_HEADER_SIZE, SOCKET_BUFFER_MTU, 0);
				}
			}
			m2m_wifi_handle_events(NULL);
		}
	}

	return size_tmp;
}

void socketBufferClearSpawned(SOCKET sock)
{
	gastrSocketBuffer[sock].flag &= ~SOCKET_BUFFER_FLAG_SPAWN;

	recv(sock, gastrSocketBuffer[sock].buffer, SOCKET_BUFFER_MTU, 0);
}

void socketBufferClose(SOCKET sock)
{
	gastrSocketBuffer[sock].head = gastrSocketBuffer[sock].tail = 0;
	gastrSocketBuffer[sock].parent = -1;
	gastrSocketBuffer[sock].flag = 0;
}

void socketBufferCb(SOCKET sock, uint8 u8Msg, void *pvMsg)
{
	switch (u8Msg) {
		/* Socket connected. */
		case SOCKET_MSG_CONNECT:
		{
			tstrSocketConnectMsg *pstrConnect = (tstrSocketConnectMsg *)pvMsg;
			if (pstrConnect) {
				if (pstrConnect->s8Error >= 0) {
					gastrSocketBuffer[sock].flag = SOCKET_BUFFER_FLAG_CONNECTED;

					// Enable receive buffer:
					recv(sock, gastrSocketBuffer[sock].buffer, SOCKET_BUFFER_MTU, 0);
				} else {
					gastrSocketBuffer[sock].flag = 0;
				}
			}
		}
		break;
		
		/* TCP Data receive. */
		case SOCKET_MSG_RECV:
		{
			// Network led ON (rev A then rev B).
			m2m_periph_gpio_set_val(M2M_PERIPH_GPIO16, 0);
			m2m_periph_gpio_set_val(M2M_PERIPH_GPIO5, 0);
			
			tstrSocketRecvMsg *pstrRecv = (tstrSocketRecvMsg *)pvMsg;
			if (pstrRecv && pstrRecv->s16BufferSize > 0) {
				/* Protect against overflow. */
				if (gastrSocketBuffer[sock].head + pstrRecv->s16BufferSize > SOCKET_BUFFER_TCP_SIZE) {
					gastrSocketBuffer[sock].flag |= SOCKET_BUFFER_FLAG_FULL;
					break;
				}

				/* Add data size. */
				gastrSocketBuffer[sock].head += pstrRecv->s16BufferSize;
				
				/* Buffer full, stop reception. */
				if ((SOCKET_BUFFER_TCP_SIZE - gastrSocketBuffer[sock].head) < SOCKET_BUFFER_MTU) {
					if (pstrRecv->u16RemainingSize != 0) {
						gastrSocketBuffer[sock].flag |= SOCKET_BUFFER_FLAG_FULL;
					}
				}
				else {
					recv(sock, gastrSocketBuffer[sock].buffer + gastrSocketBuffer[sock].head, SOCKET_BUFFER_MTU, 0);
				}
			}
			/* Test EOF (Socket closed) condition for TCP socket. */
			else {
				gastrSocketBuffer[sock].flag = 0;
				gastrSocketBuffer[sock].parent = -1;
				close(sock);
			}
			
			// Network led OFF (rev A then rev B).
			m2m_periph_gpio_set_val(M2M_PERIPH_GPIO16, 1);
			m2m_periph_gpio_set_val(M2M_PERIPH_GPIO5, 1);
		}
		break;

		/* TCP Data send. */
		case SOCKET_MSG_SEND:
		{
			gastrSocketBuffer[sock].flag &= ~SOCKET_BUFFER_FLAG_SENDING;
		}
		break;

		/* UDP Data receive. */
		case SOCKET_MSG_RECVFROM:
		{
			// Network led ON (rev A then rev B).
			m2m_periph_gpio_set_val(M2M_PERIPH_GPIO16, 0);
			m2m_periph_gpio_set_val(M2M_PERIPH_GPIO5, 0);
			
			tstrSocketRecvMsg *pstrRecv = (tstrSocketRecvMsg *)pvMsg;
			if (pstrRecv && pstrRecv->s16BufferSize > 0) {

				if (hif_small_xfer < 2) {
					uint32 h = gastrSocketBuffer[sock].head;
					uint8 *buf = gastrSocketBuffer[sock].buffer;
					uint16 sz = pstrRecv->s16BufferSize + pstrRecv->u16RemainingSize;
				
					/* Store packet size. */
					buf[h++] = sz >> 8;
					buf[h++] = sz;

					/* Store remote host port. */
					buf[h++] = pstrRecv->strRemoteAddr.sin_port;
					buf[h++] = pstrRecv->strRemoteAddr.sin_port >> 8;

					/* Store remote host IP. */
					buf[h++] = pstrRecv->strRemoteAddr.sin_addr.s_addr >> 24;
					buf[h++] = pstrRecv->strRemoteAddr.sin_addr.s_addr >> 16;
					buf[h++] = pstrRecv->strRemoteAddr.sin_addr.s_addr >> 8;
					buf[h++] = pstrRecv->strRemoteAddr.sin_addr.s_addr;
				
					/* Data received. */
					gastrSocketBuffer[sock].head = h + pstrRecv->s16BufferSize;
				}
				else {
					/* Data received. */
					gastrSocketBuffer[sock].head += pstrRecv->s16BufferSize;
				}
				
				/* Buffer full, stop reception. */
				if (SOCKET_BUFFER_UDP_SIZE - gastrSocketBuffer[sock].head < SOCKET_BUFFER_MTU + SOCKET_BUFFER_UDP_HEADER_SIZE) {
					if (pstrRecv->u16RemainingSize != 0) {
						gastrSocketBuffer[sock].flag |= SOCKET_BUFFER_FLAG_FULL;
					}
				}
				else {
					if (hif_small_xfer && hif_small_xfer != 3) {
						recvfrom(sock, gastrSocketBuffer[sock].buffer + gastrSocketBuffer[sock].head, SOCKET_BUFFER_MTU, 0);
					}
					else {
						recvfrom(sock, gastrSocketBuffer[sock].buffer + gastrSocketBuffer[sock].head + SOCKET_BUFFER_UDP_HEADER_SIZE, SOCKET_BUFFER_MTU, 0);
					}
				}
			}
			
			// Network led OFF (rev A then rev B).
			m2m_periph_gpio_set_val(M2M_PERIPH_GPIO16, 1);
			m2m_periph_gpio_set_val(M2M_PERIPH_GPIO5, 1);
		}
		break;

		/* Socket bind. */
		case SOCKET_MSG_BIND:
		{
			tstrSocketBindMsg *pstrBind = (tstrSocketBindMsg *)pvMsg;
			if (pstrBind) {
				if (pstrBind->status == 0) {
					gastrSocketBuffer[sock].flag = SOCKET_BUFFER_FLAG_BIND;
					/* TCP socket needs to enter Listen state. */
					if (sock < TCP_SOCK_MAX) {
						listen(sock, 0);
					}
					/* UDP socket only needs to supply the receive buffer. */
					/* +8 is used to store size, port and IP of incoming data. */
					else {
						recvfrom(sock, gastrSocketBuffer[sock].buffer + SOCKET_BUFFER_UDP_HEADER_SIZE, SOCKET_BUFFER_MTU, 0);
					}
				} else {
					gastrSocketBuffer[sock].flag = 0;
				}
			}
		}
		break;

		/* Connect accept. */
		case SOCKET_MSG_ACCEPT:
		{
			tstrSocketAcceptMsg *pstrAccept = (tstrSocketAcceptMsg *)pvMsg;
			if (pstrAccept && pstrAccept->sock >= 0) {
				gastrSocketBuffer[pstrAccept->sock].flag = SOCKET_BUFFER_FLAG_CONNECTED | SOCKET_BUFFER_FLAG_SPAWN;
				gastrSocketBuffer[pstrAccept->sock].parent = sock;
			}
		}
		break;
	
	}
}
