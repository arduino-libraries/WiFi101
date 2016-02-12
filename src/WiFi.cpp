/*
  WiFi.cpp - Library for Arduino Wifi shield.
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

#include "WiFi101.h"

extern "C" {
  #include "bsp/include/nm_bsp.h"
  #include "socket/include/socket_buffer.h"
  #include "driver/source/nmasic.h"
  #include "driver/include/m2m_periph.h"
}

static void wifi_cb(uint8_t u8MsgType, void *pvMsg)
{
	switch (u8MsgType) {
		case M2M_WIFI_RESP_CON_STATE_CHANGED:
		{
			tstrM2mWifiStateChanged *pstrWifiState = (tstrM2mWifiStateChanged *)pvMsg;
			if (pstrWifiState->u8CurrState == M2M_WIFI_CONNECTED) {
				//SERIAL_PORT_MONITOR.println("wifi_cb: M2M_WIFI_RESP_CON_STATE_CHANGED: CONNECTED");
				if (WiFi._mode == WL_STA_MODE && !WiFi._dhcp) {
					WiFi._status = WL_CONNECTED;

					// WiFi led ON.
					m2m_periph_gpio_set_val(M2M_PERIPH_GPIO15, 0);
				}
			} else if (pstrWifiState->u8CurrState == M2M_WIFI_DISCONNECTED) {
				//SERIAL_PORT_MONITOR.println("wifi_cb: M2M_WIFI_RESP_CON_STATE_CHANGED: DISCONNECTED");
				if (WiFi._mode == WL_STA_MODE) {
					WiFi._status = WL_DISCONNECTED;
					if (WiFi._dhcp) {
						WiFi._localip = 0;
						WiFi._submask = 0;
						WiFi._gateway = 0;
					}
				}
				// WiFi led OFF (rev A then rev B).
				m2m_periph_gpio_set_val(M2M_PERIPH_GPIO15, 1);
				m2m_periph_gpio_set_val(M2M_PERIPH_GPIO4, 1);
			}
		}
		break;

		case M2M_WIFI_REQ_DHCP_CONF:
		{
			if (WiFi._mode == WL_STA_MODE) {
				tstrM2MIPConfig *pstrIPCfg = (tstrM2MIPConfig *)pvMsg;
				WiFi._localip = pstrIPCfg->u32StaticIP;
				WiFi._submask = pstrIPCfg->u32SubnetMask;
				WiFi._gateway = pstrIPCfg->u32Gateway;
				
				WiFi._status = WL_CONNECTED;

				// WiFi led ON (rev A then rev B).
				m2m_periph_gpio_set_val(M2M_PERIPH_GPIO15, 0);
				m2m_periph_gpio_set_val(M2M_PERIPH_GPIO4, 0);
			}
			/*uint8_t *pu8IPAddress = (uint8_t *)pvMsg;
			SERIAL_PORT_MONITOR.print("wifi_cb: M2M_WIFI_REQ_DHCP_CONF: IP is ");
			SERIAL_PORT_MONITOR.print(pu8IPAddress[0], 10);
			SERIAL_PORT_MONITOR.print(".");
			SERIAL_PORT_MONITOR.print(pu8IPAddress[1], 10);
			SERIAL_PORT_MONITOR.print(".");
			SERIAL_PORT_MONITOR.print(pu8IPAddress[2], 10);
			SERIAL_PORT_MONITOR.print(".");
			SERIAL_PORT_MONITOR.print(pu8IPAddress[3], 10);
			SERIAL_PORT_MONITOR.println("");*/
		}
		break;

		case M2M_WIFI_RESP_CURRENT_RSSI:
		{
			WiFi._resolve = *((int8_t *)pvMsg);
		}
		break;

		case M2M_WIFI_RESP_PROVISION_INFO:
		{
			tstrM2MProvisionInfo *pstrProvInfo = (tstrM2MProvisionInfo *)pvMsg;
			//SERIAL_PORT_MONITOR.println("wifi_cb: M2M_WIFI_RESP_PROVISION_INFO");

			if (pstrProvInfo->u8Status == M2M_SUCCESS) {
				memset(WiFi._ssid, 0, M2M_MAX_SSID_LEN);
				memcpy(WiFi._ssid, (char *)pstrProvInfo->au8SSID, strlen((char *)pstrProvInfo->au8SSID));
				WiFi._mode = WL_STA_MODE;
				WiFi._localip = 0;
				WiFi._submask = 0;
				WiFi._gateway = 0;
				m2m_wifi_connect((char *)pstrProvInfo->au8SSID, strlen((char *)pstrProvInfo->au8SSID),
						pstrProvInfo->u8SecType, pstrProvInfo->au8Password, M2M_WIFI_CH_ALL);
			} else {
				WiFi._status = WL_CONNECT_FAILED;
				//SERIAL_PORT_MONITOR.println("wifi_cb: Provision failed.\r\n");
			}
		}
		break;

		case M2M_WIFI_RESP_SCAN_DONE:
		{
			tstrM2mScanDone *pstrInfo = (tstrM2mScanDone *)pvMsg;
			if (pstrInfo->u8NumofCh >= 1) {
				WiFi._status = WL_SCAN_COMPLETED;
			}
		}
		break;

		case M2M_WIFI_RESP_SCAN_RESULT:
		{
			tstrM2mWifiscanResult *pstrScanResult = (tstrM2mWifiscanResult *)pvMsg;
			uint16_t scan_ssid_len = strlen((const char *)pstrScanResult->au8SSID);
			memset(WiFi._scan_ssid, 0, M2M_MAX_SSID_LEN);
			if (scan_ssid_len) {
				memcpy(WiFi._scan_ssid, (const char *)pstrScanResult->au8SSID, scan_ssid_len);
			}
			if (WiFi._bssid) {
				memcpy(WiFi._bssid, (const char *)pstrScanResult->au8BSSID, 6);
			}
			WiFi._resolve = pstrScanResult->s8rssi;
			WiFi._scan_auth = pstrScanResult->u8AuthType;
			WiFi._status = WL_SCAN_COMPLETED;
		}
		break;

		default:
		break;
	}
}

static void resolve_cb(uint8_t * /* hostName */, uint32_t hostIp)
{
	WiFi._resolve = hostIp;
}

WiFiClass::WiFiClass()
{
	_mode = WL_RESET_MODE;
	_status = WL_NO_SHIELD;
	_init = 0;
}

int WiFiClass::init()
{
	tstrWifiInitParam param;
	int8_t ret;

	// Initialize the WiFi BSP:
	nm_bsp_init();

	// Initialize WiFi module and register status callback:
	param.pfAppWifiCb = wifi_cb;
	ret = m2m_wifi_init(&param);
	if (M2M_SUCCESS != ret) {
		// Error led ON (rev A then rev B).
		m2m_periph_gpio_set_val(M2M_PERIPH_GPIO18, 0);
		m2m_periph_gpio_set_dir(M2M_PERIPH_GPIO6, 1);
		return ret;
	}

	// Initialize socket API and register socket callback:
	socketInit();
	socketBufferInit();
	registerSocketCallback(socketBufferCb, resolve_cb);
	_init = 1;
	_status = WL_IDLE_STATUS;
	_localip = 0;
	_submask = 0;
	_gateway = 0;
	_dhcp = 1;
	memset(_client, 0, sizeof(WiFiClient *) * TCP_SOCK_MAX);

	// Initialize IO expander LED control (rev A then rev B)..
	m2m_periph_gpio_set_val(M2M_PERIPH_GPIO15, 1);
	m2m_periph_gpio_set_val(M2M_PERIPH_GPIO16, 1);
	m2m_periph_gpio_set_val(M2M_PERIPH_GPIO18, 1);
	m2m_periph_gpio_set_dir(M2M_PERIPH_GPIO15, 1);
	m2m_periph_gpio_set_dir(M2M_PERIPH_GPIO16, 1);
	m2m_periph_gpio_set_dir(M2M_PERIPH_GPIO18, 1);
	m2m_periph_gpio_set_val(M2M_PERIPH_GPIO4, 1);
	m2m_periph_gpio_set_val(M2M_PERIPH_GPIO5, 1);
	m2m_periph_gpio_set_val(M2M_PERIPH_GPIO6, 1);
	m2m_periph_gpio_set_dir(M2M_PERIPH_GPIO4, 1);
	m2m_periph_gpio_set_dir(M2M_PERIPH_GPIO5, 1);
	m2m_periph_gpio_set_dir(M2M_PERIPH_GPIO6, 1);

	return ret;
}

extern "C" {
	sint8 nm_get_firmware_info(tstrM2mRev* M2mRev);
}

char* WiFiClass::firmwareVersion()
{
	tstrM2mRev rev;
	
	if (!_init) {
		init();
	}
	nm_get_firmware_info(&rev);
	memset(_version, 0, 9);
	if (rev.u8FirmwareMajor != rev.u8DriverMajor && rev.u8FirmwareMinor != rev.u8DriverMinor) {
		sprintf(_version, "-Err-");
	}
	else {
		sprintf(_version, "%d.%d.%d", rev.u8FirmwareMajor, rev.u8FirmwareMinor, rev.u8FirmwarePatch);
	}
	return _version;
}

uint8_t WiFiClass::begin()
{
	if (!_init) {
		init();
	}
	
	// Connect to router:
	if (_dhcp) {
		_localip = 0;
		_submask = 0;
		_gateway = 0;
	}
	if (m2m_wifi_default_connect() < 0) {
		_status = WL_CONNECT_FAILED;
		return _status;
	}
	_status = WL_IDLE_STATUS;
	_mode = WL_STA_MODE;

	// Wait for connection or timeout:
	unsigned long start = millis();
	while (!(_status & WL_CONNECTED) &&
			!(_status & WL_DISCONNECTED) &&
			millis() - start < 60000) {
		m2m_wifi_handle_events(NULL);
	}
	if (!(_status & WL_CONNECTED)) {
		_mode = WL_RESET_MODE;
	}

	memset(_ssid, 0, M2M_MAX_SSID_LEN);
	return _status;
}

uint8_t WiFiClass::begin(const char *ssid)
{
	return startConnect(ssid, M2M_WIFI_SEC_OPEN, (void *)0);
}

uint8_t WiFiClass::begin(const char *ssid, uint8_t key_idx, const char* key)
{
	tstrM2mWifiWepParams wep_params;

	memset(&wep_params, 0, sizeof(tstrM2mWifiWepParams));
	wep_params.u8KeyIndx = key_idx;
	wep_params.u8KeySz = strlen(key);
	strcpy((char *)&wep_params.au8WepKey[0], key);
	return startConnect(ssid, M2M_WIFI_SEC_WEP, &wep_params);
}

uint8_t WiFiClass::begin(const char *ssid, const char *key)
{
	return startConnect(ssid, M2M_WIFI_SEC_WPA_PSK, key);
}

uint8_t WiFiClass::startConnect(const char *ssid, uint8_t u8SecType, const void *pvAuthInfo)
{
	if (!_init) {
		init();
	}
	
	// Connect to router:
	if (_dhcp) {
		_localip = 0;
		_submask = 0;
		_gateway = 0;
	}
	if (m2m_wifi_connect(ssid, strlen(ssid), u8SecType, pvAuthInfo, M2M_WIFI_CH_ALL) < 0) {
		_status = WL_CONNECT_FAILED;
		return _status;
	}
	_status = WL_IDLE_STATUS;
	_mode = WL_STA_MODE;

	// Wait for connection or timeout:
	unsigned long start = millis();
	while (!(_status & WL_CONNECTED) &&
			!(_status & WL_DISCONNECTED) &&
			millis() - start < 60000) {
		m2m_wifi_handle_events(NULL);
	}
	if (!(_status & WL_CONNECTED)) {
		_mode = WL_RESET_MODE;
	}

	memset(_ssid, 0, M2M_MAX_SSID_LEN);
	memcpy(_ssid, ssid, strlen(ssid));
	return _status;
}

uint8_t WiFiClass::beginAP(char *ssid)
{
	return beginAP(ssid, 1);
}

uint8_t WiFiClass::beginAP(char *ssid, uint8_t channel)
{
	tstrM2MAPConfig strM2MAPConfig;

	if (!_init) {
		init();
	}

	// Enter Access Point mode:
	memset(&strM2MAPConfig, 0x00, sizeof(tstrM2MAPConfig));
	strcpy((char *)&strM2MAPConfig.au8SSID, ssid);
	strM2MAPConfig.u8ListenChannel = channel;
	strM2MAPConfig.u8SecType = M2M_WIFI_SEC_OPEN;
	strM2MAPConfig.au8DHCPServerIP[0] = 0xC0; /* 192 */
	strM2MAPConfig.au8DHCPServerIP[1] = 0xA8; /* 168 */
	strM2MAPConfig.au8DHCPServerIP[2] = 0x01; /* 1 */
	strM2MAPConfig.au8DHCPServerIP[3] = 0x01; /* 1 */
	if (m2m_wifi_enable_ap(&strM2MAPConfig) < 0) {
		_status = WL_CONNECT_FAILED;
		return _status;
	}
	_status = WL_CONNECTED;
	_mode = WL_AP_MODE;

	memset(_ssid, 0, M2M_MAX_SSID_LEN);
	memcpy(_ssid, ssid, strlen(ssid));
	m2m_memcpy((uint8 *)&_localip, (uint8 *)&strM2MAPConfig.au8DHCPServerIP[0], 4);
	_submask = 0x00FFFFFF;
	_gateway = _localip;

	// WiFi led ON (rev A then rev B).
	m2m_periph_gpio_set_val(M2M_PERIPH_GPIO15, 0);
	m2m_periph_gpio_set_val(M2M_PERIPH_GPIO4, 0);

	return _status;
}

uint8_t WiFiClass::beginProvision(char *ssid, char *url)
{
	return beginProvision(ssid, url, 1);
}

uint8_t WiFiClass::beginProvision(char *ssid, char *url, uint8_t channel)
{
	tstrM2MAPConfig strM2MAPConfig;

	if (!_init) {
		init();
	}

	// Enter Provision mode:
	memset(&strM2MAPConfig, 0x00, sizeof(tstrM2MAPConfig));
	strcpy((char *)&strM2MAPConfig.au8SSID, ssid);
	strM2MAPConfig.u8ListenChannel = channel;
	strM2MAPConfig.u8SecType = M2M_WIFI_SEC_OPEN;
	strM2MAPConfig.u8SsidHide = SSID_MODE_VISIBLE;
	strM2MAPConfig.au8DHCPServerIP[0] = 0xC0; /* 192 */
	strM2MAPConfig.au8DHCPServerIP[1] = 0xA8; /* 168 */
	strM2MAPConfig.au8DHCPServerIP[2] = 0x01; /* 1 */
	strM2MAPConfig.au8DHCPServerIP[3] = 0x01; /* 1 */

	if (m2m_wifi_start_provision_mode((tstrM2MAPConfig *)&strM2MAPConfig, url, 1) < 0) {
		_status = WL_CONNECT_FAILED;
		return _status;
	}
	_status = WL_CONNECTED;
	_mode = WL_PROV_MODE;

	memset(_ssid, 0, M2M_MAX_SSID_LEN);
	memcpy(_ssid, ssid, strlen(ssid));
	m2m_memcpy((uint8 *)&_localip, (uint8 *)&strM2MAPConfig.au8DHCPServerIP[0], 4);
	_submask = 0x00FFFFFF;
	_gateway = _localip;

	// WiFi led ON (rev A then rev B).
	m2m_periph_gpio_set_val(M2M_PERIPH_GPIO15, 0);
	m2m_periph_gpio_set_val(M2M_PERIPH_GPIO4, 0);

	return _status;
}

uint32_t WiFiClass::provisioned()
{
	if (_mode == WL_STA_MODE) {
		return 1;
	}
	else {
		return 0;
	}
}

void WiFiClass::config(IPAddress local_ip)
{
	config(local_ip, (uint32_t)0);
}

void WiFiClass::config(IPAddress local_ip, IPAddress dns_server)
{
	config(local_ip, dns_server, (uint32_t)0);
}

void WiFiClass::config(IPAddress local_ip, IPAddress dns_server, IPAddress gateway)
{
	config(local_ip, dns_server, gateway, (uint32_t)0);
}

void WiFiClass::config(IPAddress local_ip, IPAddress dns_server, IPAddress gateway, IPAddress subnet)
{
	tstrM2MIPConfig conf;

	if (!_init) {
		init();
	}

	conf.u32DNS = (uint32_t)dns_server;
	conf.u32Gateway = (uint32_t)gateway;
	conf.u32StaticIP = (uint32_t)local_ip;
	conf.u32SubnetMask = (uint32_t)subnet;
	_dhcp = 0;
	m2m_wifi_enable_dhcp(0); // disable DHCP
	m2m_wifi_set_static_ip(&conf);
	_localip = conf.u32StaticIP;
	_submask = conf.u32SubnetMask;
	_gateway = conf.u32Gateway;
}

void WiFiClass::disconnect()
{
	m2m_wifi_disconnect();

	// WiFi led OFF (rev A then rev B).
	m2m_periph_gpio_set_val(M2M_PERIPH_GPIO15, 1);
	m2m_periph_gpio_set_val(M2M_PERIPH_GPIO4, 1);
}

uint8_t *WiFiClass::macAddress(uint8_t *mac)
{
	m2m_wifi_get_mac_address(mac);
	byte tmpMac[6], i;
	
	m2m_wifi_get_mac_address(tmpMac);
	
	for(i = 0; i < 6; i++)
		mac[i] = tmpMac[5-i];
		
	return mac;
}

uint32_t WiFiClass::localIP()
{
	return _localip;
}

uint32_t WiFiClass::subnetMask()
{
	return _submask;
}

uint32_t WiFiClass::gatewayIP()
{
	return _gateway;
}

char* WiFiClass::SSID()
{
	if (_status == WL_CONNECTED) {
		return _ssid;
	}
	else {
		return 0;
	}
}

uint8_t* WiFiClass::BSSID(uint8_t* bssid)
{
	int8_t net = scanNetworks();
	
	_bssid = bssid;
	memset(bssid, 0, 6);
	for (uint8_t i = 0; i < net; ++i) {
		SSID(i);
		if (strcmp(_scan_ssid, _ssid) == 0) {
			break;
		}
	}
	
	_bssid = 0;
	return bssid;
}

int32_t WiFiClass::RSSI()
{
	// Clear pending events:
	m2m_wifi_handle_events(NULL);

	// Send RSSI request:
	_resolve = 0;
	if (m2m_wifi_req_curr_rssi() < 0) {
		return 0;
	}

	// Wait for connection or timeout:
	unsigned long start = millis();
	while (_resolve == 0 && millis() - start < 1000) {
		m2m_wifi_handle_events(NULL);
	}

	return _resolve;
}

int8_t WiFiClass::scanNetworks()
{
	wl_status_t tmp = _status;

	if (!_init) {
		init();
	}

	// Start scan:
	if (m2m_wifi_request_scan(M2M_WIFI_CH_ALL) < 0) {
		return 0;
	}

	// Wait for scan result or timeout:
	_status = WL_IDLE_STATUS;
	unsigned long start = millis();
	while (!(_status & WL_SCAN_COMPLETED) && millis() - start < 5000) {
		m2m_wifi_handle_events(NULL);
	}
	_status = tmp;
	return m2m_wifi_get_num_ap_found();
}

char* WiFiClass::SSID(uint8_t pos)
{
	wl_status_t tmp = _status;

	// Get scan SSID result:
	memset(_scan_ssid, 0, M2M_MAX_SSID_LEN);
	if (m2m_wifi_req_scan_result(pos) < 0) {
		return 0;
	}

	// Wait for connection or timeout:
	_status = WL_IDLE_STATUS;
	unsigned long start = millis();
	while (!(_status & WL_SCAN_COMPLETED) && millis() - start < 2000) {
		m2m_wifi_handle_events(NULL);
	}

	_status = tmp;
	return _scan_ssid;
}

int32_t WiFiClass::RSSI(uint8_t pos)
{
	wl_status_t tmp = _status;

	// Get scan RSSI result:
	if (m2m_wifi_req_scan_result(pos) < 0) {
		return 0;
	}

	// Wait for connection or timeout:
	_status = WL_IDLE_STATUS;
	unsigned long start = millis();
	while (!(_status & WL_SCAN_COMPLETED) && millis() - start < 2000) {
		m2m_wifi_handle_events(NULL);
	}

	_status = tmp;
	return _resolve;
}

uint8_t WiFiClass::encryptionType()
{ 
	int8_t net = scanNetworks();

	for (uint8_t i = 0; i < net; ++i) {
		SSID(i);
		if (strcmp(_scan_ssid, _ssid) == 0) {
			break;
		}
	}

	return _scan_auth;
}

uint8_t WiFiClass::encryptionType(uint8_t pos)
{
	wl_status_t tmp = _status;

	// Get scan auth result:
	if (m2m_wifi_req_scan_result(pos) < 0) {
		return 0;
	}

	// Wait for connection or timeout:
	_status = WL_IDLE_STATUS;
	unsigned long start = millis();
	while (!(_status & WL_SCAN_COMPLETED) && millis() - start < 2000) {
		m2m_wifi_handle_events(NULL);
	}

	_status = tmp;
	return _scan_auth;
}

uint8_t WiFiClass::status()
{
	if (!_init) {
		init();
	}
	return _status;
}

int WiFiClass::hostByName(const char* aHostname, IPAddress& aResult)
{
	
	// check if aHostname is already an ipaddress
	if (aResult.fromString(aHostname)) {
		// if fromString returns true we have an IP address ready 
		return 1;

	} else {
		// Network led ON (rev A then rev B).
		m2m_periph_gpio_set_val(M2M_PERIPH_GPIO16, 0);
		m2m_periph_gpio_set_val(M2M_PERIPH_GPIO5, 0);
	
		// Send DNS request:
		_resolve = 0;
		if (gethostbyname((uint8 *)aHostname) < 0) {
			// Network led OFF (rev A then rev B).
			m2m_periph_gpio_set_val(M2M_PERIPH_GPIO16, 1);
			m2m_periph_gpio_set_val(M2M_PERIPH_GPIO5, 1);
			return 0;
		}

		// Wait for connection or timeout:
		unsigned long start = millis();
		while (_resolve == 0 && millis() - start < 20000) {
			m2m_wifi_handle_events(NULL);
		}

		// Network led OFF (rev A then rev B).
		m2m_periph_gpio_set_val(M2M_PERIPH_GPIO16, 1);
		m2m_periph_gpio_set_val(M2M_PERIPH_GPIO5, 1);

		if (_resolve == 0) {
			return 0;
		}

		aResult = _resolve;
		return 1;
	}
}

void WiFiClass::refresh(void)
{
	// Update state machine:
	m2m_wifi_handle_events(NULL);
}

WiFiClass WiFi;
