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

#define IS_CONNECTED	(m_impl && (m_impl->_flag & SOCKET_BUFFER_FLAG_CONNECTED))

struct WiFiClientImpl
{
    uint32_t _flag;
    int8_t _socket;
    uint32_t _head;
    uint32_t _tail;
    uint8_t _buffer[SOCKET_BUFFER_TCP_SIZE];
    WiFiClientImpl():_socket(-1), _flag(0), _head(0), _tail(0){}

    WiFiClientImpl(uint8_t sock, uint8_t parentsock = 0):
    _socket(sock),
    _flag(SOCKET_BUFFER_FLAG_CONNECTED), _head(0), _tail(0)
    {
        if(parentsock)
            _flag |= ((uint32_t)parentsock) << SOCKET_BUFFER_FLAG_PARENT_SOCKET_POS;

        // Add socket buffer handler:
    	socketBufferRegister(_socket, &_flag, &_head, &_tail, _buffer);

    	// Enable receive buffer:
    	recv(_socket, _buffer, SOCKET_BUFFER_MTU, 0);

    	m2m_wifi_handle_events(NULL);
    }

    ~WiFiClientImpl()
    {
        stop();
    }

    void stop()
    {
        if(_socket > 0)
        {
            socketBufferUnregister(_socket);
            close(_socket);
            _socket = -1;
            _flag = 0;
        }
    }
};

WiFiClient::WiFiClient(){}

WiFiClient::WiFiClient(uint8_t sock, uint8_t parentsock)
{
    m_impl.reset(new WiFiClientImpl(sock, parentsock));
	WiFi._client[sock].m_impl = m_impl;
}

WiFiClient::WiFiClient(const WiFiClient& other)
{
    m_impl = other.m_impl;
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
	if(WiFi.hostByName(host, remote_addr)){
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
    int8_t sock = socket(AF_INET, SOCK_STREAM, opt);

	if(sock < 0){
        m_impl.reset();
        return 0;
    }

	if(opt & SOCKET_FLAGS_SSL && hostname){
		setsockopt(sock, SOL_SSL_SOCKET, SO_SSL_SNI, hostname, m2m_strlen((uint8_t *)hostname));
	}

    // create new impl object
	m_impl.reset(new WiFiClientImpl(sock));

	// Connect to remote host:
	if (connectSocket(m_impl->_socket, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0) {
        m_impl.reset();
		return 0;
	}

	// Wait for connection or timeout:
	unsigned long start = millis();
	while (!IS_CONNECTED && millis() - start < 20000) {
		m2m_wifi_handle_events(NULL);
	}
	if (!IS_CONNECTED){
        m_impl.reset();
		return 0;
	}

	WiFi._client[sock].m_impl = m_impl;
	return 1;
}

size_t WiFiClient::write(uint8_t b)
{
	return write(&b, 1);
}

size_t WiFiClient::write(const uint8_t *buf, size_t size)
{
	sint16 err;

	if (!m_impl || size == 0 || !IS_CONNECTED) {
		setWriteError();
		return 0;
	}

	// Network led ON (rev A then rev B).
	m2m_periph_gpio_set_val(M2M_PERIPH_GPIO16, 0);
	m2m_periph_gpio_set_val(M2M_PERIPH_GPIO5, 0);

	m2m_wifi_handle_events(NULL);

	while ((err = send(m_impl->_socket, (void *)buf, size, 0)) < 0) {
		// Exit on fatal error, retry if buffer not ready.
		if (err != SOCK_ERR_BUFFER_FULL) {
			setWriteError();
			m2m_periph_gpio_set_val(M2M_PERIPH_GPIO16, 1);
			m2m_periph_gpio_set_val(M2M_PERIPH_GPIO5, 1);
			return 0;
		}
		m2m_wifi_handle_events(NULL);
	}

	// Network led OFF (rev A then rev B).
	m2m_periph_gpio_set_val(M2M_PERIPH_GPIO16, 1);
	m2m_periph_gpio_set_val(M2M_PERIPH_GPIO5, 1);

	return size;
}

int WiFiClient::available()
{
    if(m_impl)
    {
        m2m_wifi_handle_events(NULL);
        return m_impl->_head - m_impl->_tail;
    }
	return 0;
}

int WiFiClient::read()
{
	uint8_t b;
	if(read(&b, sizeof(b)) == -1){ return -1; }
	return b;
}

int WiFiClient::read(uint8_t* buf, size_t size)
{
	// sizeof(size_t) is architecture dependent
	// but we need a 16 bit data type here
	uint16_t size_tmp = available();

	if(!m_impl || size_tmp == 0){ return -1; }
	if (size < size_tmp){ size_tmp = size; }

    uint8_t* src_buf = m_impl->_buffer;
    uint32_t& tail = m_impl->_tail;

	for (uint32_t i = 0; i < size_tmp; ++i) {
		buf[i] = src_buf[tail++];
	}

	if(m_impl->_tail == m_impl->_head) {
		m_impl->_tail = m_impl->_head = 0;
		m_impl->_flag &= ~SOCKET_BUFFER_FLAG_FULL;
		recv(m_impl->_socket, m_impl->_buffer, SOCKET_BUFFER_MTU, 0);
		m2m_wifi_handle_events(NULL);
	}
	return size_tmp;
}

int WiFiClient::peek()
{
	if(available()){ return m_impl->_buffer[m_impl->_tail]; }
    return -1;
}

void WiFiClient::flush()
{
	while(available()){ read(); }
}

void WiFiClient::stop()
{
	if(m_impl){ m_impl->stop(); }
}

uint8_t WiFiClient::connected()
{
	m2m_wifi_handle_events(NULL);
	return available() || IS_CONNECTED;
}

uint32_t WiFiClient::flag() const
{
    return m_impl ? m_impl->_flag : 0;
}

uint8_t WiFiClient::status()
{
	// Deprecated.
	return 0;
}

WiFiClient::operator bool()
{
	return m_impl;
}

WiFiClient& WiFiClient::operator=(const WiFiClient &other)
{
    m_impl = other.m_impl;
	return *this;
}

bool WiFiClient::operator==(const WiFiClient &other) const
{
    m_impl && m_impl == other.m_impl;
}

bool WiFiClient::operator!=(const WiFiClient &other) const
{
    return !(*this == other);
}
