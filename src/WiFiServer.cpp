/*
  WiFiClient.cpp - Library for Arduino Wifi shield.
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
	#include "socket/include/socket.h"
}

#include "WiFi101.h"
#include "WiFiClient.h"
#include "WiFiServer.h"

WiFiServer::WiFiServer(uint16_t port)
{
	_port = port;
}

void WiFiServer::begin()
{
	begin(0);
}

uint8_t WiFiServer::beginSSL()
{
	return begin(SOCKET_FLAGS_SSL);
}

uint8_t WiFiServer::begin(uint8_t opt)
{
	struct sockaddr_in addr;

	// Initialize socket address structure.
	addr.sin_family = AF_INET;
	addr.sin_port = _htons(_port);
	addr.sin_addr.s_addr = 0;

	// Open TCP server socket.
	if ((_socket = socket(AF_INET, SOCK_STREAM, opt)) < 0) {
		return 0;
	}

	gastrSocketBuffer[_socket].flag = SOCKET_BUFFER_FLAG_BINDING;
	gastrSocketBuffer[_socket].parent = -1;

	// Bind socket:
	if (bind(_socket, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0) {
		gastrSocketBuffer[_socket].flag = 0;
		close(_socket);
		_socket = -1;
		return 0;
	}

	while (gastrSocketBuffer[_socket].flag & SOCKET_BUFFER_FLAG_BINDING) {
		m2m_wifi_handle_events(NULL);
	}

	if ((gastrSocketBuffer[_socket].flag & SOCKET_BUFFER_FLAG_BIND) == 0) {
		close(_socket);
		_socket = -1;
		return 0;
	}

	return 1;
}

WiFiClient WiFiServer::available(uint8_t* status)
{
	SOCKET sock = -1;

	m2m_wifi_handle_events(NULL);

	// search for new connection
	for (SOCKET s = 0; s < TCP_SOCK_MAX; s++) {
		if (gastrSocketBuffer[s].parent == _socket && (gastrSocketBuffer[s].flag & SOCKET_BUFFER_FLAG_SPAWN)) {
			sock = s;

			if (status != NULL) {
				*status = 0;
			}
			break;
		}
	}

	if (sock == -1) {
		// search for old connection with new data
		for (SOCKET s = 0; s < TCP_SOCK_MAX; s++) {
			if (gastrSocketBuffer[s].parent == _socket && (gastrSocketBuffer[s].head - gastrSocketBuffer[s].tail) > 0) {
				sock = s;
				break;
			}
		}
	}

	if (sock != -1) {
		if (gastrSocketBuffer[sock].flag & SOCKET_BUFFER_FLAG_SPAWN) {
			gastrSocketBuffer[sock].flag &= ~SOCKET_BUFFER_FLAG_SPAWN;

			gastrSocketBuffer[sock].buffer = (uint8*)realloc(gastrSocketBuffer[sock].buffer, SOCKET_BUFFER_TCP_SIZE);
			gastrSocketBuffer[sock].head = gastrSocketBuffer[sock].tail = 0;

			recv(sock, gastrSocketBuffer[sock].buffer, SOCKET_BUFFER_MTU, 0);
			m2m_wifi_handle_events(NULL);
		}

		return WiFiClient(sock);
	}

	return WiFiClient();
}

uint8_t WiFiServer::status() {
	// Deprecated.
	return 0;
}

size_t WiFiServer::write(uint8_t b)
{
	return write(&b, 1);
}

size_t WiFiServer::write(const uint8_t *buffer, size_t size)
{
	size_t n = 0;

	for (SOCKET s = 0; s < TCP_SOCK_MAX; s++) {
		if (gastrSocketBuffer[s].parent == _socket) {
			WiFiClient client(s);

			n += client.write(buffer, size);
		}
	}

	return n;
}
