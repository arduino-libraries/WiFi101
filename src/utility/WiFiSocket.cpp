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
	#include "driver/source/nmbus.h"
}

#include "WiFiSocket.h"

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
}

WiFiSocketClass::~WiFiSocketClass()
{
}

SOCKET WiFiSocketClass::create(uint16 u16Domain, uint8 u8Type, uint8 u8Flags)
{
	SOCKET sock = socket(u16Domain, u8Type, u8Flags);

	if (sock > 0) {
		_info[sock].state = SOCKET_STATE_IDLE;
		_info[sock].parent = -1;
	}

	return sock;
}

sint8 WiFiSocketClass::bind(SOCKET sock, struct sockaddr *pstrAddr, uint8 u8AddrLen)
{
	if (bindSocket(sock, pstrAddr, u8AddrLen) < 0) {
		return 0;
	}

	_info[sock].state = SOCKET_STATE_BINDING;

	unsigned long start = millis();

	while (_info[sock].state == SOCKET_STATE_BINDING && millis() - start < 2000) {
		m2m_wifi_handle_events(NULL);
	}

	if (_info[sock].state != SOCKET_STATE_BOUND) {
		_info[sock].state = SOCKET_STATE_IDLE;
		return 0;
	}

	_info[sock].recvReply.s16RecvStatus = 0;
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
	if (listenSocket(sock, backlog) < 0) {
		return 0;
	}

	_info[sock].state = SOCKET_STATE_LISTEN;

	unsigned long start = millis();

	while (_info[sock].state == SOCKET_STATE_LISTEN && millis() - start < 2000) {
		m2m_wifi_handle_events(NULL);
	}

	if (_info[sock].state != SOCKET_STATE_LISTENING) {
		_info[sock].state = SOCKET_STATE_IDLE;
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
	if (connectSocket(sock, pstrAddr, u8AddrLen) < 0) {
		return 0;
	}

	_info[sock].state = SOCKET_STATE_CONNECTING;

	unsigned long start = millis();

	while (_info[sock].state == SOCKET_STATE_CONNECTING && millis() - start < 20000) {
		m2m_wifi_handle_events(NULL);
	}

	if (_info[sock].state != SOCKET_STATE_CONNECTED) {
		_info[sock].state = SOCKET_STATE_IDLE;
		return 0;
	}

	_info[sock].recvReply.s16RecvStatus = 0;
	recv(sock, NULL, 0, 0);

	return 1;
}

uint8 WiFiSocketClass::connected(SOCKET sock)
{
	m2m_wifi_handle_events(NULL);

	return (_info[sock].state == SOCKET_STATE_CONNECTED);
}

int WiFiSocketClass::available(SOCKET sock)
{
	m2m_wifi_handle_events(NULL);

	if (_info[sock].state != SOCKET_STATE_CONNECTED && _info[sock].state != SOCKET_STATE_BOUND) {
		return 0;
	}

	return _info[sock].recvReply.s16RecvStatus;
}

int WiFiSocketClass::peek(SOCKET sock)
{
	m2m_wifi_handle_events(NULL);

	if (_info[sock].state != SOCKET_STATE_CONNECTED && _info[sock].state != SOCKET_STATE_BOUND) {
		return -1;
	}

	if (_info[sock].recvReply.s16RecvStatus <= 1) {
		return -1;
	}

	byte b;

	if (nm_read_block(_info[sock].recvAddress, &b, (uint16)sizeof(b)) != M2M_SUCCESS) {
		return -1;
	}

	return b;
}

int WiFiSocketClass::read(SOCKET sock, uint8_t* buf, size_t size)
{
	m2m_wifi_handle_events(NULL);

	if (_info[sock].state != SOCKET_STATE_CONNECTED && _info[sock].state != SOCKET_STATE_BOUND) {
		return 0;
	}

	if (_info[sock].recvReply.s16RecvStatus <= 0) {
		return 0;
	}

	if ((sint16)size > _info[sock].recvReply.s16RecvStatus) {
		size = _info[sock].recvReply.s16RecvStatus;
	}

	uint8 lastTransfer = ((sint16)size == _info[sock].recvReply.s16RecvStatus);

	if (nm_read_block(_info[sock].recvAddress, buf, (sint16)size) != M2M_SUCCESS) {
		return 0;
	}

	_info[sock].recvAddress += size;
	_info[sock].recvReply.s16RecvStatus -= size;

	if (lastTransfer) {
		_info[sock].recvReply.s16RecvStatus = 0;

		if (sock < TCP_SOCK_MAX) {
			// TCP
			recv(sock, NULL, 0, 0);
		} else {
			// UDP
			recvfrom(sock, NULL, 0, 0);
		}
	}

	return size;
}

IPAddress WiFiSocketClass::remoteIP(SOCKET sock)
{
	return _info[sock].recvReply.strRemoteAddr.u32IPAddr;
}

uint16_t WiFiSocketClass::remotePort(SOCKET sock)
{
	return _info[sock].recvReply.strRemoteAddr.u16Port;
}

size_t WiFiSocketClass::write(SOCKET sock, const uint8_t *buf, size_t size)
{
	m2m_wifi_handle_events(NULL);

	if (_info[sock].state != SOCKET_STATE_CONNECTED) {
		return 0;
	}

	sint16 err;

	while ((err = send(sock, (void *)buf, size, 0)) < 0) {
		// Exit on fatal error, retry if buffer not ready.
		if (err != SOCK_ERR_BUFFER_FULL) {
			return 0;
		}
		m2m_wifi_handle_events(NULL);
	}

	return size;
}

sint16 WiFiSocketClass::sendto(SOCKET sock, void *pvSendBuffer, uint16 u16SendLength, uint16 flags, struct sockaddr *pstrDestAddr, uint8 u8AddrLen)
{
	m2m_wifi_handle_events(NULL);

	if (_info[sock].state != SOCKET_STATE_BOUND) {
		return 0;
	}

	return sendtoSocket(sock, pvSendBuffer, u16SendLength, flags, pstrDestAddr, u8AddrLen);
}

sint8 WiFiSocketClass::close(SOCKET sock)
{
	m2m_wifi_handle_events(NULL);

	_info[sock].state = SOCKET_STATE_INVALID;
	_info[sock].parent = -1;

	return closeSocket(sock);
}

int WiFiSocketClass::hasParent(SOCKET sock, SOCKET child)
{
	if (_info[child].parent != sock) {
		return 0;
	}

	return 1;
}

SOCKET WiFiSocketClass::accepted(SOCKET sock)
{
	m2m_wifi_handle_events(NULL);

	for (SOCKET s = 0; s < TCP_SOCK_MAX; s++) {
		if (_info[s].parent == sock && _info[s].state == SOCKET_STATE_ACCEPTED) {
			_info[s].state = SOCKET_STATE_CONNECTED;

			_info[s].recvReply.s16RecvStatus = 0;
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
				_info[sock].state = SOCKET_STATE_BOUND;
			} else {
				_info[sock].state = SOCKET_STATE_IDLE;
			}
		}
		break;

		/* Socket listen. */
		case SOCKET_MSG_LISTEN: {
			tstrSocketListenMsg *pstrListen = (tstrSocketListenMsg *)pvMsg;

			if (pstrListen && pstrListen->status == 0) {
				_info[sock].state = SOCKET_STATE_LISTENING;
			} else {
				_info[sock].state = SOCKET_STATE_IDLE;
			}
		}
		break;

		/* Socket accept. */
		case SOCKET_MSG_ACCEPT: {
			tstrSocketAcceptMsg *pstrAccept = (tstrSocketAcceptMsg*)pvMsg;

			if (pstrAccept && pstrAccept->sock > -1) {
				_info[pstrAccept->sock].state = SOCKET_STATE_ACCEPTED;
				_info[pstrAccept->sock].parent = sock;
			}
		}
		break;

		/* Socket connected. */
		case SOCKET_MSG_CONNECT: {
			tstrSocketConnectMsg *pstrConnect = (tstrSocketConnectMsg *)pvMsg;
			if (pstrConnect && pstrConnect->s8Error >= 0) {
				_info[sock].state = SOCKET_STATE_CONNECTED;
			} else {
				_info[sock].state = SOCKET_STATE_IDLE;
			}
		}
		break;

		/* Socket data received. */
		case SOCKET_MSG_RECV:
		case SOCKET_MSG_RECVFROM: {
			tstrRecvReply* recvReply = (tstrRecvReply *)pvMsg;

			if (recvReply->s16RecvStatus <= 0) {
				_info[sock].state = SOCKET_STATE_IDLE;
			} else {
				_info[sock].recvReply = *recvReply;
			}

			hif_receive(0, NULL, 0, 1);
		}
		break;

		/* Socket data sent. */
		case SOCKET_MSG_SEND: {
			// sint16 *s16Sent = (sint16 *)pvMsg;
		}
		break;

		/* Socket data address. (Arduino added) */ 
		case SOCKET_MSG_RECV_ADDRESS: {
			_info[sock].recvAddress = *((uint32_t*)pvMsg);
		}
		break;

		default:
			break;
	}
}

WiFiSocketClass WiFiSocket;
