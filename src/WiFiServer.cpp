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
	_opt = 0;
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

	_opt = opt;

	// Initialize socket address structure.
	addr.sin_family = AF_INET;
	addr.sin_port = _htons(_port);
	addr.sin_addr.s_addr = 0;

	// Open TCP server socket.
	if ((_socket = socket(AF_INET, SOCK_STREAM, opt)) < 0) {
		return 0;
	}

	// Bind socket:
	if (bind(_socket, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0) {
		socketBufferClose(_socket);
		close(_socket);
		_socket = -1;
		return 0;
	}

	if (socketBufferBindWait(_socket) == 0) {
		socketBufferClose(_socket);
		close(_socket);
		_socket = -1;
		return 0;
	}

	return 1;
}

WiFiClient WiFiServer::available(uint8_t* status)
{
	SOCKET newSock = -1;
	SOCKET fullSock = -1;
	SOCKET dataSock = -1;
	SOCKET sock = -1;

	m2m_wifi_handle_events(NULL);

	if (_socket == -1 || !socketBufferIsBind(_socket)) {
		_socket = -1;
		begin(_opt);
	}

	// search for existing connecion with data or new connection
	for (SOCKET s = 0; s < TCP_SOCK_MAX; s++) {
		if (socketBufferHasParent(s, _socket)) {
			if (socketBufferIsFull(s)) {
				fullSock = s;
			} else if (socketBufferIsSpawned(s)) {
				newSock = s;
			} else if (socketBufferDataAvailable(s) > 0) {
				dataSock = s;
			}
		}
	}

	if (fullSock != -1) {
		sock = fullSock; // give highest priority to full sockets
	} else if (newSock != -1) {
		sock = newSock; // give 2nd priority to new sockets

		if (status != NULL) {
			*status = 0;
		}
	} else if (dataSock != -1) {
		sock = dataSock; // give last priority to sockets with data
	}

	if (sock != -1) {
		if (socketBufferIsSpawned(sock)) {
			socketBufferSetupBuffer(sock);
			socketBufferClearSpawned(sock);
			
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
		if (socketBufferHasParent(s, _socket)) {
			WiFiClient client(s);

			n += client.write(buffer, size);
		}
	}

	return n;
}
