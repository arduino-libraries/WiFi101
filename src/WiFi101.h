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

#ifndef WIFI_H
#define WIFI_H

extern "C" {
	#include "driver/include/m2m_wifi.h"
	#include "socket/include/socket.h"
}

#include "WiFiClient.h"
#include "WiFiServer.h"

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

class WiFiClass
{
private:
	int _init;
	char _version[9];
	uint8_t startConnect(char *ssid, uint8_t u8SecType, void *pvAuthInfo);

public:
	uint32_t _localip;
	uint32_t _resolve;
	uint32_t _req2;
	wl_mode_t _mode;
	wl_status_t _status;
	char _scan_ssid[M2M_MAX_SSID_LEN];
	char _ssid[M2M_MAX_SSID_LEN];

    WiFiClass();

    int init();
	
	char* firmwareVersion();

    /* Start Wifi connection with WPA/WPA2 encryption.
     *
     * param ssid: Pointer to the SSID string.
     * param key: Key input buffer.
     */
	uint8_t begin();
	uint8_t begin(char *ssid);
	uint8_t begin(char *ssid, uint8_t key_idx, const char* key);
	uint8_t begin(char *ssid, char *key);
	
    /* Start Wifi in Access Point, with open security.
     * Only one client can connect to the AP at a time.
     *
     * param ssid: Pointer to the SSID string.
     * param channel: Wifi channel to use. Valid values are 1-12.
     */
	uint8_t beginAP(char *ssid);
	uint8_t beginAP(char *ssid, uint8_t channel);
	
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
	char* SSID();
	int32_t RSSI();

	/*
	 * Return the Encryption Type associated with the network
	 *
	 * return: one value of wl_enc_type enum
	 */
	uint8_t encryptionType() { return 0; } // TODO

	/*
	 * Return the current BSSID associated with the network.
	 * It is the MAC address of the Access Point
	 *
	 * return: pointer to uint8_t array with length WL_MAC_ADDR_LENGTH
	 */
	uint8_t* BSSID(uint8_t* bssid) { return NULL; } // TODO

	int8_t scanNetworks();
	char* SSID(uint8_t pos);
	int32_t RSSI(uint8_t pos);
	uint8_t encryptionType(uint8_t pos);

	uint8_t status();
	
	int hostByName(const char* aHostname, IPAddress& aResult);

	void refresh(void);

};

extern WiFiClass WiFi;

#endif /* WIFI_H */
