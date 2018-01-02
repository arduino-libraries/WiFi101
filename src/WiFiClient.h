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

#ifndef WIFICLIENT_H
#define WIFICLIENT_H

#include <Arduino.h>
#include <Client.h>
#include <IPAddress.h>

extern "C" {
	#include "socket/include/socket.h"
}

class WiFiClient : public Client {

public:
	WiFiClient();
	WiFiClient(uint8_t sock);

	uint8_t status();
	
	int connectSSL(IPAddress ip, uint16_t port);
	int connectSSL(const char* host, uint16_t port);
	virtual int connect(IPAddress ip, uint16_t port);
	virtual int connect(const char* host, uint16_t port);
	virtual size_t write(uint8_t);
	virtual size_t write(const uint8_t *buf, size_t size);
	virtual int available();
	virtual int read();
	virtual int read(uint8_t *buf, size_t size);
	virtual int peek();
	virtual void flush();
	virtual void stop();
	virtual uint8_t connected();
	virtual operator bool();
	bool operator==(const WiFiClient &other) const;
	bool operator!=(const WiFiClient &other) const;

	using Print::write;

	virtual IPAddress remoteIP();
	virtual uint16_t remotePort();

private:
	SOCKET _socket;

	int connect(const char* host, uint16_t port, uint8_t opt);
	int connect(IPAddress ip, uint16_t port, uint8_t opt, const uint8_t *hostname);
};

#endif /* WIFICLIENT_H */
