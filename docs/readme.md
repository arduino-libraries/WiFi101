# WiFi101 library
This library allows you to use the Arduino WiFi Shield 101 and the MKR1000 board. These are powerful IoT solutions with crypto-authentication, developed with ATMEL, that connects your Arduino or Genuino to the internet wirelessly. Connecting the board or the shield to a WiFi network is simple, no further configuration in addition to the SSID and the password are required. It can serve as either a server accepting incoming connections or a client making outgoing ones. The library supports WEP and WPA2 Personal encryption. Compared to the retired WiFi Shield and the related library, this product and library support all the same methods plus the connectSSL().

The board connected to the shield communicates with the WiFi shield 101 using the SPI bus. This is on digital pins 11, 12, and 13 on the Uno and pins 50, 51, and 52 on the Mega. On both boards, pin 10 is used as SS. On the Mega, the hardware SS pin, 53, is not used but it must be kept as an output or the SPI interface won't work. Digital pin 7 is used as a handshake pin between the WiFi shield 101 and the underlying board, and should not be used. The WiFi101 library is very similar to the [Ethernet](https://www.arduino.cc/en/Reference/Ethernet) and the library [WiFi](https://www.arduino.cc/en/Reference/WiFi), and many of the function calls are the same.

For additional information on the Arduino WiFi Shield 101, see the [Getting Started](https://www.arduino.cc/en/Guide/ArduinoWiFiShield101) page and the [Arduino WiFi Shield 101 hardware page](https://www.arduino.cc/en/Main/ArduinoWiFiShield101[). For more information on MKR1000 see the [product page](https://www.arduino.cc/en/Main/ArduinoMKR1000).

To use this library
```
#include <SPI.h>
#include <WiFi101.h>
```
## Note
This library requires that your board or shield has a matching firmware installed. When the library is updated, also the firmware might be updated, but it is not mandatory. To avoid any issue and ensure that you have the most up to date setup, we suggest that you check your WiFi101 library with the Arduino Software (IDE) Library Manager. There is an option in the Preferences that enables the check for updates of any of the installed libraries at startup. If you haven't installed the WiFi101 library yet, you won't get notified about its updates. Anyway, you get the library status just writing its name in the search field on top of the Library Manager.


When the library version installed on your computer is the latest available, you may check the firmware version of the board or the shield. We have prepared a utility sketch to check the firmware version and its matching with the library. If the firmware needs an update, another utility sketch enables the process. Below the link to the relevant tutorials.

## Utilities
- [CheckWiFi101FirmwareVersion](https://www.arduino.cc/en/Tutorial/CheckWiFi101FirmwareVersion) : Reads the required firmware number required from library and matches with the one installed on the board or the shield.
- [FirmwareUpdater](https://www.arduino.cc/en/Tutorial/FirmwareUpdater) : The sketch that must be loaded to allow the firmware and certificates update process through the integrated plugin of Arduino Software (IDE) rel. 1.6.10 or later.
## Examples
- [ConnectNoEncryption](https://www.arduino.cc/en/Tutorial/Wifi101ConnectNoEncryption) : Demonstrates how to connect to an open network
- [ConnectWithWEP](https://www.arduino.cc/en/Tutorial/Wifi101ConnectWithWEP) : Demonstrates how to connect to a network that is encrypted with WEP
- [ConnectWithWPA](https://www.arduino.cc/en/Tutorial/Wifi101ConnectWithWPA) : Demonstrates how to connect to a network that is encrypted with WPA2 Personal
- [ScanNetworks](https://www.arduino.cc/en/Tutorial/Wifi101ScanNetworks) : Displays all WiFi networks in range
- [WiFiChatServer](https://www.arduino.cc/en/Tutorial/Wifi101WiFiChatServer) : Set up a simple chat server
- [WiFiWebClient](https://www.arduino.cc/en/Tutorial/Wifi101WiFiWebClient) : Connect to a remote webserver
- [WiFiWebClientRepeating](https://www.arduino.cc/en/Tutorial/Wifi101WiFiWebClientRepeating) : Make repeated HTTP calls to a webserver
- [WiFiWebServer](https://www.arduino.cc/en/Tutorial/Wifi101WiFiWebServer) : Serve a webpage from the WiFi shield
- [WiFiUdpSendReceiveString](https://www.arduino.cc/en/Tutorial/Wifi101WiFiUdpSendReceiveString) : Send and receive a UDP string
- [UdpNTPClient](https://www.arduino.cc/en/Tutorial/Wifi101UdpNTPClient) : Query a Network Time Protocol (NTP) server using UDP
