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
	#include "driver/include/m2m_periph.h"
}

#include "WiFi101.h"
#include "WiFiClient.h"

WiFiClient::WiFiClient()
{
	_socket = -1;
}

WiFiClient::WiFiClient(SOCKET sock)
{
	// Spawn connected TCP client from TCP server socket:
	_socket = sock;

	m2m_wifi_handle_events(NULL);
}

int WiFiClient::connectSSL(const char* host, uint16_t port)
{
	return connect(host, port, SOCKET_FLAGS_SSL);
}

int WiFiClient::connectSSL(IPAddress ip, uint16_t port)
{
	return connect(ip, port, SOCKET_FLAGS_SSL, 0);
}

int WiFiClient::connect(const char* host, uint16_t port)
{
	return connect(host, port, 0);
}

int WiFiClient::connect(IPAddress ip, uint16_t port)
{
	return connect(ip, port, 0, 0);	
}

int WiFiClient::connect(const char* host, uint16_t port, uint8_t opt)
{
	IPAddress remote_addr;
	if (WiFi.hostByName(host, remote_addr)) {
		return connect(remote_addr, port, opt, (const uint8_t *)host);
	}
	return 0;
}

int WiFiClient::connect(IPAddress ip, uint16_t port, uint8_t opt, const uint8_t *hostname)
{
	struct sockaddr_in addr;

	// Initialize socket address structure:
	addr.sin_family = AF_INET;
	addr.sin_port = _htons(port);
	addr.sin_addr.s_addr = ip;

	// Create TCP socket:
	if ((_socket = socket(AF_INET, SOCK_STREAM, opt)) < 0) {
		return 0;
	}

	if (opt & SOCKET_FLAGS_SSL && hostname) {
		setsockopt(_socket, SOL_SSL_SOCKET, SO_SSL_SNI, hostname, m2m_strlen((uint8_t *)hostname));
	}

	socketBufferSetupBuffer(_socket);

	// Connect to remote host:
	if (connectSocket(_socket, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0) {
		socketBufferClose(_socket);
		close(_socket);
		_socket = -1;
		return 0;
	}

	if (socketBufferConnectWait(_socket) == 0) {
		socketBufferClose(_socket);
		close(_socket);
		_socket = -1;
		return 0;
	}

	return 1;
}

size_t WiFiClient::write(uint8_t b)
{
	return write(&b, 1);
}

size_t WiFiClient::write(const uint8_t *buf, size_t size)
{
	sint16 err;

	if (_socket < 0 || size == 0) {
		setWriteError();
		return 0;
	}

	// Network led ON (rev A then rev B).
	m2m_periph_gpio_set_val(M2M_PERIPH_GPIO16, 0);
	m2m_periph_gpio_set_val(M2M_PERIPH_GPIO5, 0);

	m2m_wifi_handle_events(NULL);

	while ((err = send(_socket, (void *)buf, size, 0)) < 0) {
		// Exit on fatal error, retry if buffer not ready.
		// or can't wait for send event because a recv is pending
		if (err != SOCK_ERR_BUFFER_FULL || socketBufferSendWait(_socket) == 0) {
			setWriteError();
			m2m_periph_gpio_set_val(M2M_PERIPH_GPIO16, 1);
			m2m_periph_gpio_set_val(M2M_PERIPH_GPIO5, 1);
			return 0;
		}
	}

	// Network led OFF (rev A then rev B).
	m2m_periph_gpio_set_val(M2M_PERIPH_GPIO16, 1);
	m2m_periph_gpio_set_val(M2M_PERIPH_GPIO5, 1);

	return size;
}

int WiFiClient::available()
{
	m2m_wifi_handle_events(NULL);

	if (_socket != -1) {
		return socketBufferDataAvailable(_socket);
	}
	return 0;
}

int WiFiClient::read()
{
	uint8_t b;

	if (read(&b, sizeof(b)) == -1) {
		return -1;
	}

	return b;
}

int WiFiClient::read(uint8_t* buf, size_t size)
{
	// sizeof(size_t) is architecture dependent
	// but we need a 16 bit data type here
	uint16_t size_tmp = available();
	
	if (size_tmp == 0) {
		return -1;
	}

	return socketBufferRead(_socket, buf, size);
}

int WiFiClient::peek()
{
	if (!available())
		return -1;

	return socketBufferPeek(_socket);
}

void WiFiClient::flush()
{
	while (available())
		read();
}

void WiFiClient::stop()
{
	if (_socket < 0)
		return;

	close(_socket);

	// flush RX buffer, to unblock m2m_wifi_handle_events
	flush();

	socketBufferClose(_socket);
	_socket = -1;
}

uint8_t WiFiClient::connected()
{
	m2m_wifi_handle_events(NULL);

	if (available())
		return 1;

	if ((_socket == -1) || !socketBufferIsConnected(_socket)) {
		_socket = -1;
		return 0;
	}

	return 1;
}

uint8_t WiFiClient::status()
{
	// Deprecated.
	return 0;
}

WiFiClient::operator bool()
{
	return _socket != -1;
}
