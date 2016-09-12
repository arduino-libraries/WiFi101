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
  #include "bsp/include/nm_bsp_arduino.h"
  #include "socket/include/socket_buffer.h"
  #include "socket/include/m2m_socket_host_if.h"
  #include "driver/source/nmasic.h"
  #include "driver/include/m2m_periph.h"
}

void WiFiClass::wifi_cb(uint8_t u8MsgType, void *pvMsg)
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
				} else if (WiFi._mode == WL_AP_MODE) {
					WiFi._status = WL_AP_CONNECTED;
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
					// Close sockets to clean state
					// Clients will need to reconnect once the physical link will be re-established
					for (int i=0; i < TCP_SOCK_MAX; i++) {
						if (WiFi._client[i])
							WiFi._client[i]->stop();
					}
				} else if (WiFi._mode == WL_AP_MODE) {
					WiFi._status = WL_AP_LISTENING;
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

void WiFiClass::resolve_cb(uint8_t * /* hostName */, uint32_t hostIp)
{
	WiFi._resolve = hostIp;
}

void WiFiClass::ping_cb(uint32 u32IPAddr, uint32 /*u32RTT*/, uint8 u8ErrorCode)
{
	if (PING_ERR_SUCCESS == u8ErrorCode) {
		// Ensure this ICMP reply comes from requested IP address
		if (WiFi._resolve == u32IPAddr) {
			WiFi._resolve = WL_PING_SUCCESS;
		} else {
			// Another network device replied to the our ICMP request
			WiFi._resolve = WL_PING_DEST_UNREACHABLE;
		}
	} else if (PING_ERR_DEST_UNREACH == u8ErrorCode) {
		WiFi._resolve = WL_PING_DEST_UNREACHABLE;
	} else if (PING_ERR_TIMEOUT == u8ErrorCode) {
		WiFi._resolve = WL_PING_TIMEOUT;
	} else {
		WiFi._resolve = WL_PING_ERROR;
	}
}

WiFiClass::WiFiClass()
{
	_mode = WL_RESET_MODE;
	_status = WL_NO_SHIELD;
	_init = 0;
}

void WiFiClass::setPins(int8_t cs, int8_t irq, int8_t rst, int8_t en)
{
	gi8Winc1501CsPin = cs;
	gi8Winc1501IntnPin = irq;
	gi8Winc1501ResetPin = rst;
	gi8Winc1501ChipEnPin = en;
}

int WiFiClass::init()
{
	tstrWifiInitParam param;
	int8_t ret;

	// Initialize the WiFi BSP:
	nm_bsp_init();

	// Initialize WiFi module and register status callback:
	param.pfAppWifiCb = WiFiClass::wifi_cb;
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
	registerSocketCallback(socketBufferCb, WiFiClass::resolve_cb);
	_init = 1;
	_status = WL_IDLE_STATUS;
	_localip = 0;
	_submask = 0;
	_gateway = 0;
	_dhcp = 1;

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

uint8_t WiFiClass::beginAP(const char *ssid)
{
	return beginAP(ssid, 1);
}

uint8_t WiFiClass::beginAP(const char *ssid, uint8_t channel)
{
	return startAP(ssid, M2M_WIFI_SEC_OPEN, NULL, channel);
}

uint8_t WiFiClass::beginAP(const char *ssid, uint8_t key_idx, const char* key)
{
	return beginAP(ssid, key_idx, key, 1);
}

uint8_t WiFiClass::beginAP(const char *ssid, uint8_t key_idx, const char* key, uint8_t channel)
{
	tstrM2mWifiWepParams wep_params;

	if (key_idx == 0) {
		key_idx = 1; // 1 is the minimum key index
	}

	memset(&wep_params, 0, sizeof(tstrM2mWifiWepParams));
	wep_params.u8KeyIndx = key_idx;
	wep_params.u8KeySz = strlen(key);
	strcpy((char *)&wep_params.au8WepKey[0], key);

	return startAP(ssid, M2M_WIFI_SEC_WEP, &wep_params, channel);
}

uint8_t WiFiClass::startAP(const char *ssid, uint8_t u8SecType, const void *pvAuthInfo, uint8_t channel)
{
	tstrM2MAPConfig strM2MAPConfig;

	if (!_init) {
		init();
	}

	if (channel == 0) {
		channel = 1; // channel 1 is the minium channel
	}

	// Enter Access Point mode:
	memset(&strM2MAPConfig, 0x00, sizeof(tstrM2MAPConfig));
	strcpy((char *)&strM2MAPConfig.au8SSID, ssid);
	strM2MAPConfig.u8ListenChannel = channel - 1;
	strM2MAPConfig.u8SecType = u8SecType;
	strM2MAPConfig.au8DHCPServerIP[0] = 0xC0; /* 192 */
	strM2MAPConfig.au8DHCPServerIP[1] = 0xA8; /* 168 */
	strM2MAPConfig.au8DHCPServerIP[2] = 0x01; /* 1 */
	strM2MAPConfig.au8DHCPServerIP[3] = 0x01; /* 1 */

	if (u8SecType == M2M_WIFI_SEC_WEP) {
		tstrM2mWifiWepParams* wep_params = (tstrM2mWifiWepParams*)pvAuthInfo;

		strM2MAPConfig.u8KeyIndx = wep_params->u8KeyIndx;
		strM2MAPConfig.u8KeySz = wep_params->u8KeySz;
		strcpy((char*)strM2MAPConfig.au8WepKey, (char *)wep_params->au8WepKey);
	}

	if (m2m_wifi_enable_ap(&strM2MAPConfig) < 0) {
		_status = WL_AP_FAILED;
		return _status;
	}
	_status = WL_AP_LISTENING;
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

void WiFiClass::end()
{
	if (_mode == WL_AP_MODE) {
		m2m_wifi_disable_ap();

		_status = WL_IDLE_STATUS;
		_mode = WL_RESET_MODE;
	} else {
		if (_mode == WL_PROV_MODE) {
			m2m_wifi_stop_provision_mode();
		}

		m2m_wifi_disconnect();
	}

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
	if (_status == WL_CONNECTED || _status == WL_AP_LISTENING || _status == WL_AP_CONNECTED) {
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

	m2m_wifi_handle_events(NULL);

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

void WiFiClass::lowPowerMode(void)
{
	m2m_wifi_set_sleep_mode(M2M_PS_H_AUTOMATIC, true);
}

void WiFiClass::maxLowPowerMode(void)
{
	m2m_wifi_set_sleep_mode(M2M_PS_DEEP_AUTOMATIC, true);
}

void WiFiClass::noLowPowerMode(void)
{
	m2m_wifi_set_sleep_mode(M2M_NO_PS, false);
}

uint8_t WiFiClass::ping(const char* hostname, uint8_t ttl)
{
	IPAddress ip;

	if (hostByName(hostname, ip) > 0) {
		return ping(ip, ttl);
	} else {
		return WL_PING_UNKNOWN_HOST;
	}
}

uint8_t WiFiClass::ping(const String &hostname, uint8_t ttl)
{
	return ping(hostname.c_str(), ttl);
}

uint8_t WiFiClass::ping(IPAddress host, uint8_t ttl)
{
	// Network led ON (rev A then rev B).
	m2m_periph_gpio_set_val(M2M_PERIPH_GPIO16, 0);
	m2m_periph_gpio_set_val(M2M_PERIPH_GPIO5, 0);

	uint32_t dstHost = (uint32_t)host;
	_resolve = dstHost;

	if (m2m_ping_req((uint32_t)host, ttl, &WiFiClass::ping_cb) < 0) {
		// Network led OFF (rev A then rev B).
		m2m_periph_gpio_set_val(M2M_PERIPH_GPIO16, 1);
		m2m_periph_gpio_set_val(M2M_PERIPH_GPIO5, 1);
		//  Error sending ping request
		return WL_PING_ERROR;
	}

	// Wait for success or timeout:
	unsigned long start = millis();
	while (_resolve == dstHost && millis() - start < 5000) {
		m2m_wifi_handle_events(NULL);
	}

	// Network led OFF (rev A then rev B).
	m2m_periph_gpio_set_val(M2M_PERIPH_GPIO16, 1);
	m2m_periph_gpio_set_val(M2M_PERIPH_GPIO5, 1);

	if (_resolve == dstHost) {
		return WL_PING_TIMEOUT;
	} else {
		return _resolve;
	}
}

WiFiClass WiFi;
