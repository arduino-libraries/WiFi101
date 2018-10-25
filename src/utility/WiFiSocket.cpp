/*
  WiFiSocket.cpp - Library for Arduino Wifi shield.
  Copyright (c) 2011-2014 Arduino.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

extern "C" {
	#include "driver/include/m2m_wifi.h"
	#include "socket/include/m2m_socket_host_if.h"
	#include "driver/source/m2m_hif.h"
	#include "driver/include/m2m_periph.h"
}

#include "WiFiSocket.h"

extern uint8 hif_receive_blocked;

enum {
	SOCKET_STATE_INVALID,
	SOCKET_STATE_IDLE,
	SOCKET_STATE_CONNECTING,
	SOCKET_STATE_CONNECTED,
	SOCKET_STATE_BINDING,
	SOCKET_STATE_BOUND,
	SOCKET_STATE_LISTEN,
	SOCKET_STATE_LISTENING,
	SOCKET_STATE_ACCEPTED
};

WiFiSocketClass::WiFiSocketClass()
{
	for (int i = 0; i < MAX_SOCKET_ALLOCATED; i++) {
		_info[i].state = SOCKET_STATE_INVALID;
		_info[i].parent = -1;
		_info[i].recvMsg.s16BufferSize = 0;
#ifdef USE_STATIC_ALLOCATION
		_info[i].buffer.head = _info[i].buffer.data;
#else
		_info[i].buffer.data = NULL;
		_info[i].buffer.head = NULL;
#endif
		_info[i].buffer.length = 0;
	}
	for (int i = 0; i < MAX_SOCKET; i++) {
		_handleMap[i] = -1;
	}
}

WiFiSocketClass::~WiFiSocketClass()
{
}

SOCKET WiFiSocketClass::create(uint16 u16Domain, uint8 u8Type, uint8 u8Flags)
{
	SOCKET sock = socket(u16Domain, u8Type, u8Flags);

	if (sock >= 0) {
		_handleMap[sock] = sock;
#if TCP_SOCK_MAX > TCP_SOCK_ALLOCATED
		if (sock >= TCP_SOCK_MAX) _handleMap[sock] -= (TCP_SOCK_MAX - TCP_SOCK_ALLOCATED);
#endif
		_info[_handleMap[sock]].state = SOCKET_STATE_IDLE;
		_info[_handleMap[sock]].parent = -1;
	}

	return sock;
}

sint8 WiFiSocketClass::bind(SOCKET sock, struct sockaddr *pstrAddr, uint8 u8AddrLen)
{
	if (::bind(sock, pstrAddr, u8AddrLen) < 0) {
		return 0;
	}

	_info[_handleMap[sock]].state = SOCKET_STATE_BINDING;

	unsigned long start = millis();

	while (_info[_handleMap[sock]].state == SOCKET_STATE_BINDING && millis() - start < 2000) {
		m2m_wifi_handle_events(NULL);
	}

	if (_info[_handleMap[sock]].state != SOCKET_STATE_BOUND) {
		_info[_handleMap[sock]].state = SOCKET_STATE_IDLE;
		return 0;
	}

	_info[_handleMap[sock]].recvMsg.s16BufferSize = 0;
	if (sock < TCP_SOCK_MAX) {
		// TCP
	} else {
		// UDP
		recvfrom(sock, NULL, 0, 0);
	}

	return 1;
}

sint8 WiFiSocketClass::listen(SOCKET sock, uint8 backlog)
{
	if (::listen(sock, backlog) < 0) {
		return 0;
	}

	_info[_handleMap[sock]].state = SOCKET_STATE_LISTEN;

	unsigned long start = millis();

	while (_info[_handleMap[sock]].state == SOCKET_STATE_LISTEN && millis() - start < 2000) {
		m2m_wifi_handle_events(NULL);
	}

	if (_info[_handleMap[sock]].state != SOCKET_STATE_LISTENING) {
		_info[_handleMap[sock]].state = SOCKET_STATE_IDLE;
		return 0;
	}

	return 1;
}

sint8 WiFiSocketClass::setopt(SOCKET socket, uint8 u8Level, uint8 option_name, const void *option_value, uint16 u16OptionLen)
{
	return setsockopt(socket, u8Level, option_name, option_value, u16OptionLen);
}

sint8 WiFiSocketClass::connect(SOCKET sock, struct sockaddr *pstrAddr, uint8 u8AddrLen)
{
	if (::connect(sock, pstrAddr, u8AddrLen) < 0) {
		return 0;
	}

	_info[_handleMap[sock]].state = SOCKET_STATE_CONNECTING;

	unsigned long start = millis();

	while (_info[_handleMap[sock]].state == SOCKET_STATE_CONNECTING && millis() - start < 20000) {
		m2m_wifi_handle_events(NULL);
	}

	if (_info[_handleMap[sock]].state != SOCKET_STATE_CONNECTED) {
		_info[_handleMap[sock]].state = SOCKET_STATE_IDLE;
		return 0;
	}

	_info[_handleMap[sock]].recvMsg.s16BufferSize = 0;
	_info[_handleMap[sock]].recvMsg.strRemoteAddr.sin_port = ((struct sockaddr_in*)pstrAddr)->sin_port;
	_info[_handleMap[sock]].recvMsg.strRemoteAddr.sin_addr.s_addr = ((struct sockaddr_in*)pstrAddr)->sin_addr.s_addr;
	recv(sock, NULL, 0, 0);

	return 1;
}

uint8 WiFiSocketClass::connected(SOCKET sock)
{
	m2m_wifi_handle_events(NULL);

	return (_info[_handleMap[sock]].state == SOCKET_STATE_CONNECTED);
}

uint8 WiFiSocketClass::listening(SOCKET sock)
{
	m2m_wifi_handle_events(NULL);

	return (_info[_handleMap[sock]].state == SOCKET_STATE_LISTENING);
}

uint8 WiFiSocketClass::bound(SOCKET sock)
{
	m2m_wifi_handle_events(NULL);

	return (_info[_handleMap[sock]].state == SOCKET_STATE_BOUND);
}

int WiFiSocketClass::available(SOCKET sock)
{
	m2m_wifi_handle_events(NULL);

	if (_info[_handleMap[sock]].state != SOCKET_STATE_CONNECTED && _info[_handleMap[sock]].state != SOCKET_STATE_BOUND) {
		return 0;
	}

	return (_info[_handleMap[sock]].buffer.length + _info[_handleMap[sock]].recvMsg.s16BufferSize);
}

int WiFiSocketClass::peek(SOCKET sock)
{
	m2m_wifi_handle_events(NULL);

	if (_info[_handleMap[sock]].state != SOCKET_STATE_CONNECTED && _info[_handleMap[sock]].state != SOCKET_STATE_BOUND) {
		return -1;
	}

	if (available(sock) == 0) {
		return -1;
	}

	if (_info[_handleMap[sock]].buffer.length == 0 && _info[_handleMap[sock]].recvMsg.s16BufferSize) {
		if (!fillRecvBuffer(sock)) {
			return -1;
		}
	}

	return *_info[_handleMap[sock]].buffer.head;
}

int WiFiSocketClass::read(SOCKET sock, uint8_t* buf, size_t size)
{
	m2m_wifi_handle_events(NULL);

	if (_info[_handleMap[sock]].state != SOCKET_STATE_CONNECTED && _info[_handleMap[sock]].state != SOCKET_STATE_BOUND) {
		return 0;
	}

	int avail = available(sock);

	if (avail <= 0) {
		return 0;
	}

	if ((int)size > avail) {
		size = avail;
	}

	int bytesRead = 0;

	while (size) {
		if (_info[_handleMap[sock]].buffer.length == 0 && _info[_handleMap[sock]].recvMsg.s16BufferSize) {
			if (!fillRecvBuffer(sock)) {
				break;
			}
		}

		int toCopy = size;

		if (toCopy > _info[_handleMap[sock]].buffer.length) {
			toCopy = _info[_handleMap[sock]].buffer.length;
		}

		memcpy(buf, _info[_handleMap[sock]].buffer.head, toCopy);
		_info[_handleMap[sock]].buffer.head += toCopy;
		_info[_handleMap[sock]].buffer.length -= toCopy;

		buf += toCopy;
		size -= toCopy;
		bytesRead += toCopy;
	}

	if (_info[_handleMap[sock]].buffer.length == 0 && _info[_handleMap[sock]].recvMsg.s16BufferSize == 0) {
		if (sock < TCP_SOCK_MAX) {
			// TCP
			recv(sock, NULL, 0, 0);
		} else {
			// UDP
			recvfrom(sock, NULL, 0, 0);
		}
		m2m_wifi_handle_events(NULL);
	}

	return bytesRead;
}

IPAddress WiFiSocketClass::remoteIP(SOCKET sock)
{
	return _info[_handleMap[sock]].recvMsg.strRemoteAddr.sin_addr.s_addr;
}

uint16_t WiFiSocketClass::remotePort(SOCKET sock)
{
	return _info[_handleMap[sock]].recvMsg.strRemoteAddr.sin_port;
}

size_t WiFiSocketClass::write(SOCKET sock, const uint8_t *buf, size_t size)
{
	m2m_wifi_handle_events(NULL);

	if (_info[_handleMap[sock]].state != SOCKET_STATE_CONNECTED) {
		return 0;
	}

#ifdef CONF_PERIPH
	// Network led ON (rev A then rev B).
	m2m_periph_gpio_set_val(M2M_PERIPH_GPIO16, 0);
	m2m_periph_gpio_set_val(M2M_PERIPH_GPIO5, 0);
#endif

	sint16 err;

	while ((err = send(sock, (void *)buf, size, 0)) < 0) {
		// Exit on fatal error, retry if buffer not ready.
		if (err != SOCK_ERR_BUFFER_FULL) {
			size = 0;
			break;
		} else if (hif_receive_blocked) {
			size = 0;
			break;
		}
		m2m_wifi_handle_events(NULL);
	}

#ifdef CONF_PERIPH
	// Network led OFF (rev A then rev B).
	m2m_periph_gpio_set_val(M2M_PERIPH_GPIO16, 1);
	m2m_periph_gpio_set_val(M2M_PERIPH_GPIO5, 1);
#endif

	return size;
}

sint16 WiFiSocketClass::sendto(SOCKET sock, void *pvSendBuffer, uint16 u16SendLength, uint16 flags, struct sockaddr *pstrDestAddr, uint8 u8AddrLen)
{
	m2m_wifi_handle_events(NULL);

	if (_info[_handleMap[sock]].state != SOCKET_STATE_BOUND) {
		return -1;
	}

	return ::sendto(sock, pvSendBuffer, u16SendLength, flags, pstrDestAddr, u8AddrLen);
}

sint8 WiFiSocketClass::close(SOCKET sock)
{
	m2m_wifi_handle_events(NULL);

	if (_info[_handleMap[sock]].state == SOCKET_STATE_CONNECTED || _info[_handleMap[sock]].state == SOCKET_STATE_BOUND) {
		if (_info[_handleMap[sock]].recvMsg.s16BufferSize > 0) {
			_info[_handleMap[sock]].recvMsg.s16BufferSize = 0;

			// flush any data not processed
			hif_receive(0, NULL, 0, 1);
		}
	}

	_info[_handleMap[sock]].state = SOCKET_STATE_INVALID;
	_info[_handleMap[sock]].parent = -1;

#ifdef USE_STATIC_ALLOCATION
	_info[_handleMap[sock]].buffer.head = _info[_handleMap[sock]].buffer.data;
#else
	if (_info[_handleMap[sock]].buffer.data != NULL) {
		free(_info[_handleMap[sock]].buffer.data);
	}
	_info[_handleMap[sock]].buffer.data = NULL;
	_info[_handleMap[sock]].buffer.head = NULL;
#endif
	_info[_handleMap[sock]].buffer.length = 0;
	_info[_handleMap[sock]].recvMsg.s16BufferSize = 0;

	return ::close(sock);
}

int WiFiSocketClass::hasParent(SOCKET sock, SOCKET child)
{
	if (_info[_handleMap[child]].parent != sock) {
		return 0;
	}

	return 1;
}

SOCKET WiFiSocketClass::accepted(SOCKET sock)
{
	m2m_wifi_handle_events(NULL);

	for (SOCKET s = 0; s < TCP_SOCK_ALLOCATED; s++) {
		if (_info[_handleMap[s]].parent == sock && _info[_handleMap[s]].state == SOCKET_STATE_ACCEPTED) {
			_info[_handleMap[s]].state = SOCKET_STATE_CONNECTED;

			_info[_handleMap[s]].recvMsg.s16BufferSize = 0;
			recv(s, NULL, 0, 0);

			return s;
		}
	}

	return -1;
}

void WiFiSocketClass::eventCallback(SOCKET sock, uint8 u8Msg, void *pvMsg)
{
	WiFiSocket.handleEvent(sock, u8Msg, pvMsg);
}

void WiFiSocketClass::handleEvent(SOCKET sock, uint8 u8Msg, void *pvMsg)
{
	switch (u8Msg) {
		/* Socket bind. */
		case SOCKET_MSG_BIND: {
			tstrSocketBindMsg *pstrBind = (tstrSocketBindMsg *)pvMsg;

			if (pstrBind && pstrBind->status == 0) {
				_info[_handleMap[sock]].state = SOCKET_STATE_BOUND;
			} else {
				_info[_handleMap[sock]].state = SOCKET_STATE_IDLE;
			}
		}
		break;

		/* Socket listen. */
		case SOCKET_MSG_LISTEN: {
			tstrSocketListenMsg *pstrListen = (tstrSocketListenMsg *)pvMsg;

			if (pstrListen && pstrListen->status == 0) {
				_info[_handleMap[sock]].state = SOCKET_STATE_LISTENING;
			} else {
				_info[_handleMap[sock]].state = SOCKET_STATE_IDLE;
			}
		}
		break;

		/* Socket accept. */
		case SOCKET_MSG_ACCEPT: {
			tstrSocketAcceptMsg *pstrAccept = (tstrSocketAcceptMsg*)pvMsg;

			if (pstrAccept && pstrAccept->sock > -1) {
				_info[_handleMap[pstrAccept->sock]].state = SOCKET_STATE_ACCEPTED;
				_info[_handleMap[pstrAccept->sock]].parent = sock;
				_info[_handleMap[pstrAccept->sock]].recvMsg.strRemoteAddr = pstrAccept->strAddr;
			}
		}
		break;

		/* Socket connected. */
		case SOCKET_MSG_CONNECT: {
			tstrSocketConnectMsg *pstrConnect = (tstrSocketConnectMsg *)pvMsg;

			if (pstrConnect && pstrConnect->s8Error >= 0) {
				_info[_handleMap[sock]].state = SOCKET_STATE_CONNECTED;
			} else {
				_info[_handleMap[sock]].state = SOCKET_STATE_IDLE;
			}
		}
		break;

		/* Socket data received. */
		case SOCKET_MSG_RECV:
		case SOCKET_MSG_RECVFROM: {
			tstrSocketRecvMsg *pstrRecvMsg = (tstrSocketRecvMsg *)pvMsg;

#ifdef CONF_PERIPH
			// Network led ON (rev A then rev B).
			m2m_periph_gpio_set_val(M2M_PERIPH_GPIO16, 0);
			m2m_periph_gpio_set_val(M2M_PERIPH_GPIO5, 0);
#endif

			if (pstrRecvMsg->s16BufferSize <= 0) {
				close(sock);
			} else if (_info[_handleMap[sock]].state == SOCKET_STATE_CONNECTED || _info[_handleMap[sock]].state == SOCKET_STATE_BOUND) {
				_info[_handleMap[sock]].recvMsg.pu8Buffer = pstrRecvMsg->pu8Buffer;
				_info[_handleMap[sock]].recvMsg.s16BufferSize = pstrRecvMsg->s16BufferSize;
				if (sock < TCP_SOCK_MAX) {
					// TCP
				} else {
					// UDP
					_info[_handleMap[sock]].recvMsg.strRemoteAddr = pstrRecvMsg->strRemoteAddr;
				}

				fillRecvBuffer(sock);
			} else {
				// not connected or bound, discard data
				hif_receive(0, NULL, 0, 1);
			}

#ifdef CONF_PERIPH
			// Network led OFF (rev A then rev B).
			m2m_periph_gpio_set_val(M2M_PERIPH_GPIO16, 1);
			m2m_periph_gpio_set_val(M2M_PERIPH_GPIO5, 1);
#endif
		}
		break;

		/* Socket data sent. */
		case SOCKET_MSG_SEND: {
			// sint16 *s16Sent = (sint16 *)pvMsg;
		}
		break;

		default:
			break;
	}
}

int WiFiSocketClass::fillRecvBuffer(SOCKET sock)
{
#ifndef USE_STATIC_ALLOCATION
	if (_info[_handleMap[sock]].buffer.data == NULL) {
		_info[_handleMap[sock]].buffer.data = (uint8_t*)malloc(SOCKET_BUFFER_SIZE);
		_info[_handleMap[sock]].buffer.head = _info[_handleMap[sock]].buffer.data;
		_info[_handleMap[sock]].buffer.length = 0;
	}
#endif

	int size = _info[_handleMap[sock]].recvMsg.s16BufferSize;

	if (size > SOCKET_BUFFER_SIZE) {
		size = SOCKET_BUFFER_SIZE;
	}

	uint8 lastTransfer = ((sint16)size == _info[_handleMap[sock]].recvMsg.s16BufferSize);

	if (hif_receive(_info[_handleMap[sock]].recvMsg.pu8Buffer, _info[_handleMap[sock]].buffer.data, (sint16)size, lastTransfer) != M2M_SUCCESS) {
		return 0;
	}

	_info[_handleMap[sock]].buffer.head = _info[_handleMap[sock]].buffer.data;
	_info[_handleMap[sock]].buffer.length = size;
	_info[_handleMap[sock]].recvMsg.pu8Buffer += size;
	_info[_handleMap[sock]].recvMsg.s16BufferSize -= size;

	return 1;
}

WiFiSocketClass WiFiSocket;
