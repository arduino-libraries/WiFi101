// Port of CC3000 MDNS Responder to WINC1500.
// Author: Tony DiCola
//
// This MDNSResponder class implements just enough MDNS functionality to respond
// to name requests, for example 'foo.local'.  This does not implement any other
// MDNS or Bonjour functionality like services, etc.
//
// Copyright (c) 2016 Adafruit Industries.  All right reserved.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

#ifndef WIFIMDNS_H
#define WIFIMDNS_H

#include "WiFi101.h"
#include "WiFiUdp.h"

class MDNSResponder {
public:
  MDNSResponder();
  ~MDNSResponder();
  bool begin(const char* domain, uint32_t ttlSeconds = 3600);
  void update();

private:
  void _broadcastResponse();

  // Expected query values
  static uint8_t _queryHeader[];
  uint8_t* _expected;
  int _expectedLen;
  // Current parsing state
  int _index;
  // Response data
  uint8_t* _response;
  int _responseLen;
  // UDP socket for receiving/sending MDNS data.
  WiFiUDP _mdnsSocket;
};

#endif
