/*
  WiFi.h - Library for Arduino Wifi shield.
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

/* Added DNS member definition for callback: M2M_WIFI_REQ_DHCP_CONF
*  Added DNS member function for dnsIP()
*  RH 11-AUG-2016
*/

#ifndef WIFI_H
#define WIFI_H

#define WIFI_FIRMWARE_REQUIRED "19.4.4"

#include <Arduino.h>

extern "C" {
	#include "driver/include/m2m_wifi.h"
	#include "socket/include/socket.h"
}


#include "Adafruit_WINC1500Client.h"
#include "Adafruit_WINC1500SSLClient.h"
#include "Adafruit_WINC1500Server.h"

typedef enum {
	WL_NO_SHIELD = 255,
	WL_IDLE_STATUS = 0,
	WL_NO_SSID_AVAIL,
	WL_SCAN_COMPLETED,
	WL_CONNECTED,
	WL_CONNECT_FAILED,
	WL_CONNECTION_LOST,
	WL_DISCONNECTED
} wl_status_t;

/* Encryption modes */
enum wl_enc_type {  /* Values map to 802.11 encryption suites... */
	ENC_TYPE_WEP  = M2M_WIFI_SEC_WEP,
	ENC_TYPE_TKIP = M2M_WIFI_SEC_WPA_PSK,
	ENC_TYPE_CCMP = M2M_WIFI_SEC_802_1X,
	/* ... except these two, 7 and 8 are reserved in 802.11-2007 */
	ENC_TYPE_NONE = M2M_WIFI_SEC_OPEN,
	ENC_TYPE_AUTO = M2M_WIFI_SEC_INVALID
};

typedef enum {
	WL_RESET_MODE = 0,
	WL_STA_MODE,
	WL_PROV_MODE,
	WL_AP_MODE
} wl_mode_t;

class Adafruit_WINC1500
{
public:
	uint32_t _localip;
	uint32_t _submask;
	uint32_t _gateway;
	uint32_t _dns;
	int _dhcp;
	uint32_t _resolve;
	byte *_bssid;
	wl_mode_t _mode;
	wl_status_t _status;
	char _scan_ssid[M2M_MAX_SSID_LEN];
	uint8_t _scan_auth;
	char _ssid[M2M_MAX_SSID_LEN];
	Adafruit_WINC1500Client *_client[TCP_SOCK_MAX];

	Adafruit_WINC1500();
	Adafruit_WINC1500(uint8_t winc_cs, uint8_t winc_irq, uint8_t winc_rst);

	int init();

	char* firmwareVersion();

	/* Start Wifi connection with WPA/WPA2 encryption.
	 *
	 * param ssid: Pointer to the SSID string.
	 * param key: Key input buffer.
	 */
	uint8_t begin();
	uint8_t begin(const char *ssid);
	uint8_t begin(const char *ssid, uint8_t key_idx, const char* key);
	uint8_t begin(const char *ssid, const char *key);
	uint8_t begin(const String &ssid) { return begin(ssid.c_str()); }
	uint8_t begin(const String &ssid, uint8_t key_idx, const String &key) { return begin(ssid.c_str(), key_idx, key.c_str()); }
	uint8_t begin(const String &ssid, const String &key) { return begin(ssid.c_str(), key.c_str()); }

	/* Start Wifi in Access Point, with open security.
	 * Only one client can connect to the AP at a time.
	 *
	 * param ssid: Pointer to the SSID string.
	 * param channel: Wifi channel to use. Valid values are 1-12.
	 */
	uint8_t beginAP(char *ssid);
	uint8_t beginAP(char *ssid, uint8_t channel);
	uint8_t beginAP(const char *ssid, uint8_t key_idx, const char* key);
	uint8_t beginAP(const char *ssid, uint8_t key_idx, const char* key, uint8_t channel);

	uint8_t beginProvision(char *ssid, char *url);
	uint8_t beginProvision(char *ssid, char *url, uint8_t channel);

	uint32_t provisioned();

	void config(IPAddress local_ip);
	void config(IPAddress local_ip, IPAddress dns_server);
	void config(IPAddress local_ip, IPAddress dns_server, IPAddress gateway);
	void config(IPAddress local_ip, IPAddress dns_server, IPAddress gateway, IPAddress subnet);

	void disconnect();

	uint8_t *macAddress(uint8_t *mac);

	uint32_t localIP();
	uint32_t subnetMask();
	uint32_t gatewayIP();
	uint32_t dnsIP();

	char* SSID();
	int32_t RSSI();
	uint8_t encryptionType();
	uint8_t* BSSID(uint8_t* bssid);
	int8_t scanNetworks();
	char* SSID(uint8_t pos);
	int32_t RSSI(uint8_t pos);
	uint8_t encryptionType(uint8_t pos);

	uint8_t status();
	void setGPIO(uint8_t g, boolean val);

	int hostByName(const char* hostname, IPAddress& result);
	int hostByName(const String &hostname, IPAddress& result) { return hostByName(hostname.c_str(), result); }

	void refresh(void);

	int8_t setSleepMode(uint8_t pstype, uint8_t bcasten);
	int8_t requestSleep(uint32_t ms);
private:
	int _init;
	char _version[9];
	uint8_t startConnect(const char *ssid, uint8_t u8SecType, const void *pvAuthInfo);
	uint8_t startAP(const char *ssid, uint8_t u8SecType, const void *pvAuthInfo, uint8_t channel);
};

extern Adafruit_WINC1500 WiFi;

#endif /* WIFI_H */
