/*
  WiFiUdp.cpp - Library for Arduino Wifi shield.
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
	#include "driver/include/m2m_periph.h"
	extern uint8 hif_small_xfer;
}

#include <string.h>
#include "WiFi101.h"
#include "WiFiUdp.h"
#include "WiFiClient.h"
#include "WiFiServer.h"

/* Constructor. */
WiFiUDP::WiFiUDP()
{
	_socket = -1;
	_rcvSize = 0;
	_rcvPort = 0;
	_rcvIP = 0;
	_sndSize = 0;
}

/* Start WiFiUDP socket, listening at local port PORT */
uint8_t WiFiUDP::begin(uint16_t port)
{
	struct sockaddr_in addr;
	uint32 u32EnableCallbacks = 0;

	_rcvSize = 0;
	_rcvPort = 0;
	_rcvIP = 0;
	_sndSize = 0;

	// Initialize socket address structure.
	addr.sin_family = AF_INET;
	addr.sin_port = _htons(port);
	addr.sin_addr.s_addr = 0;

	// Open UDP server socket.
	if ((_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		return 0;
	}

	setsockopt(_socket, SOL_SOCKET, SO_SET_UDP_SEND_CALLBACK, &u32EnableCallbacks, 0);

	gastrSocketBuffer[_socket].flag = SOCKET_BUFFER_FLAG_BINDING;
	gastrSocketBuffer[_socket].parent = -1;
	gastrSocketBuffer[_socket].buffer = (uint8*)realloc(gastrSocketBuffer[_socket].buffer, SOCKET_BUFFER_UDP_SIZE);

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

uint8_t WiFiUDP::beginMulti(IPAddress ip, uint16_t port)
{
	uint32_t multiIp = ip;

	if (!begin(port)) {
		return 0;
	}

	setsockopt(_socket, SOL_SOCKET, IP_ADD_MEMBERSHIP, &multiIp, sizeof(multiIp));

	return 1;
}

/* return number of bytes available in the current packet,
   will return zero if parsePacket hasn't been called yet */
int WiFiUDP::available()
{
	m2m_wifi_handle_events(NULL);

	if (_socket != -1) {
		return _rcvSize;
	}
	return 0;
 }

/* Release any resources being used by this WiFiUDP instance */
void WiFiUDP::stop()
{
	if (_socket < 0)
		return;

	close(_socket);
	_socket = -1;
}

int WiFiUDP::beginPacket(const char *host, uint16_t port)
{
	IPAddress ip;
	if (WiFi.hostByName(host, ip)) {
		return beginPacket(ip, port);
	}

	return 0;
}

int WiFiUDP::beginPacket(IPAddress ip, uint16_t port)
{
	_sndIP = ip;
	_sndPort = port;
	_sndSize = 0;

	return 1;
}

int WiFiUDP::endPacket()
{
	struct sockaddr_in addr;

	// Network led ON (rev A then rev B).
	m2m_periph_gpio_set_val(M2M_PERIPH_GPIO16, 0);
	m2m_periph_gpio_set_val(M2M_PERIPH_GPIO5, 0);

	addr.sin_family = AF_INET;
	addr.sin_port = _htons(_sndPort);
	addr.sin_addr.s_addr = _sndIP;

	if (sendto(_socket, (void *)_sndBuffer, _sndSize, 0,
			(struct sockaddr *)&addr, sizeof(addr)) < 0) {
		// Network led OFF (rev A then rev B).
		m2m_periph_gpio_set_val(M2M_PERIPH_GPIO16, 1);
		m2m_periph_gpio_set_val(M2M_PERIPH_GPIO5, 1);
		return 0;
	}

	// Network led OFF (rev A then rev B).
	m2m_periph_gpio_set_val(M2M_PERIPH_GPIO16, 1);
	m2m_periph_gpio_set_val(M2M_PERIPH_GPIO5, 1);

	return 1;
}

size_t WiFiUDP::write(uint8_t byte)
{
  return write(&byte, 1);
}

size_t WiFiUDP::write(const uint8_t *buffer, size_t size)
{
	if ((size + _sndSize) > sizeof(_sndBuffer)) {
		size = sizeof(_sndBuffer) - _sndSize;
	}

	memcpy(_sndBuffer + _sndSize, buffer, size);

	_sndSize += size;

	return size;
}

int WiFiUDP::parsePacket()
{
	m2m_wifi_handle_events(NULL);

	if (_socket != -1) {
		if (_rcvSize != 0) {
			return _rcvSize;
		}
		if (gastrSocketBuffer[_socket].head != gastrSocketBuffer[_socket].tail) {
			_rcvSize = ((uint16_t)gastrSocketBuffer[_socket].buffer[gastrSocketBuffer[_socket].tail] << 8) + (uint16_t)gastrSocketBuffer[_socket].buffer[gastrSocketBuffer[_socket].tail + 1];
			_rcvPort = ((uint16_t)gastrSocketBuffer[_socket].buffer[gastrSocketBuffer[_socket].tail + 2] << 8) + (uint16_t)gastrSocketBuffer[_socket].buffer[gastrSocketBuffer[_socket].tail + 3];
			_rcvIP =   ((uint32_t)gastrSocketBuffer[_socket].buffer[gastrSocketBuffer[_socket].tail + 4] << 24) + ((uint32_t)gastrSocketBuffer[_socket].buffer[gastrSocketBuffer[_socket].tail + 5] << 16) +
					((uint32_t)gastrSocketBuffer[_socket].buffer[gastrSocketBuffer[_socket].tail + 6] << 8) + (uint32_t)gastrSocketBuffer[_socket].buffer[gastrSocketBuffer[_socket].tail + 7];
			gastrSocketBuffer[_socket].tail += SOCKET_BUFFER_UDP_HEADER_SIZE;
			return _rcvSize;
		}
	}
	return 0;
}

int WiFiUDP::read()
{
	uint8_t b;

	if (read(&b, sizeof(b)) == -1) {
		return -1;
	}

	return b;
}

int WiFiUDP::read(unsigned char* buf, size_t size)
{
	// sizeof(size_t) is architecture dependent
	// but we need a 16 bit data type here
	uint16_t size_tmp = available();

	if (size_tmp == 0) {
		return -1;
	}

	if (size < size_tmp) {
		size_tmp = size;
	}

	for (uint32_t i = 0; i < size_tmp; ++i) {
		buf[i] = gastrSocketBuffer[_socket].buffer[gastrSocketBuffer[_socket].tail++];
		_rcvSize--;

		if (gastrSocketBuffer[_socket].tail == gastrSocketBuffer[_socket].head) {
			// the full buffered data has been read, reset head and tail for next transfer
			gastrSocketBuffer[_socket].tail = gastrSocketBuffer[_socket].head = 0;

			// clear the buffer full flag
			gastrSocketBuffer[_socket].flag &= ~SOCKET_BUFFER_FLAG_FULL;

			// setup buffer and buffer size to transfer the remainder of the current packet
			// or next received packet
			if (hif_small_xfer) {
				recvfrom(_socket, gastrSocketBuffer[_socket].buffer, SOCKET_BUFFER_MTU, 0);
			} else {
				recvfrom(_socket, gastrSocketBuffer[_socket].buffer + SOCKET_BUFFER_UDP_HEADER_SIZE, SOCKET_BUFFER_MTU, 0);
			}
			m2m_wifi_handle_events(NULL);
		}
	}

	return size_tmp;
}

int WiFiUDP::peek()
{
	if (!available())
		return -1;

	return gastrSocketBuffer[_socket].buffer[gastrSocketBuffer[_socket].tail];
}

void WiFiUDP::flush()
{
	while (available())
		read();
}

IPAddress  WiFiUDP::remoteIP()
{
	return _rcvIP;
}

uint16_t  WiFiUDP::remotePort()
{
	return _rcvPort;
}
