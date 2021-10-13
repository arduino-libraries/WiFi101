# WiFi101 Library

## WiFi Class

### `WiFi.begin()`

#### Description
Initializes the WiFi101 library's network settings and provides the current status.

#### Syntax
```
WiFi.begin();
WiFi.begin(ssid);
WiFi.begin(ssid, pass);
WiFi.begin(ssid, keyIndex, key);
``` 
#### Parameters
ssid: the SSID (Service Set Identifier) is the name of the WiFi network you want to connect to.

keyIndex: WEP encrypted networks can hold up to 4 different keys. This identifies which key you are going to use.

key: a hexadecimal string used as a security code for WEP encrypted networks.

pass: WPA encrypted networks use a password in the form of a string for security.

#### Returns 
WL_CONNECTED when connected to a network
WL_IDLE_STATUS when not connected to a network, but powered on
#### Example
```
#include <WiFi101.h>

//SSID of your network
char ssid[] = "yourNetwork";
//password of your WPA Network
char pass[] = "secretPassword";

void setup()
{
 WiFi.begin(ssid, pass);
}

void loop () {}
```

### `WiFi.beginProvision()`
#### Description
Initializes the WiFi101 library's in provision mode. This mode implements the behaviour needed to connect the board to known APs or to select new ones if nothing known can be reached.

When called, the function will try to connect to a previously associated access point.
If this fails, an access point named "wifi101-XXXX" will be created, where XXXX are the last 4 digits of the boards MAC address. You need to connect to the access point created, then visit http://wifi101/ and configure an SSID and password of an AP within reach. When the new association is done, the next connection will be to that AP.
#### Syntax
```
WiFi.beginProvision();
WiFi.beginProvision(channel);
```
#### Parameters
channel: optional parameter, to specify channel to use for provisioning mode, channel 1 will be used by default

#### Returns 
WL_CONNECTED when connected to a network
WL_PROVISIONING when connecting to previously associated access point fails
#### Example
```
/*
  WiFi Web Server

  A simple web server that shows the value of the analog input pins.
  using a WiFi 101 Shield.

  This example is written to configure the WiFi settings using provisioning mode.
  It also sets up an mDNS server so the IP address of the board doesn't have to
  be obtained via the serial monitor.

  Circuit:
   WiFi 101 Shield attached
   Analog inputs attached to pins A0 through A5 (optional)

  created 13 July 2010
  by dlf (Metodo2 srl)
  modified 31 May 2012
  by Tom Igoe

*/

#include <SPI.h>
#include <WiFi101.h>
#include <WiFiMDNSResponder.h>

const int ledPin = 6; // LED pin for connectivity status indicator

char mdnsName[] = "wifi101"; // the MDNS name that the board will respond to
                             // after WiFi settings have been provisioned
// Note that the actual MDNS name will have '.local' after
// the name above, so "wifi101" will be accessible on
// the MDNS name "wifi101.local".

WiFiServer server(80);

// Create a MDNS responder to listen and respond to MDNS name requests.
WiFiMDNSResponder mdnsResponder;

void setup() {
  //Initialize serial:
  Serial.begin(9600);

  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi 101 Shield not present");
    // don't continue:
    while (true);
  }

  // configure the LED pin for output mode
  pinMode(ledPin, OUTPUT);

  // Start in provisioning mode:
  //  1) This will try to connect to a previously associated access point.
  //  2) If this fails, an access point named "wifi101-XXXX" will be created, where XXXX
  //     is the last 4 digits of the boards MAC address. Once you are connected to the access point,
  //     you can configure an SSID and password by visiting http://wifi101/
  WiFi.beginProvision();

  while (WiFi.status() != WL_CONNECTED) {
    // wait while not connected

    // blink the led to show an unconnected status
    digitalWrite(ledPin, HIGH);
    delay(500);
    digitalWrite(ledPin, LOW);
    delay(500);
  }

  // connected, make the LED stay on
  digitalWrite(ledPin, HIGH);

  server.begin();

  // Setup the MDNS responder to listen to the configured name.
  // NOTE: You _must_ call this _after_ connecting to the WiFi network and
  // being assigned an IP address.
  if (!mdnsResponder.begin(mdnsName)) {
    Serial.println("Failed to start MDNS responder!");
    while(1);
  }

  Serial.print("Server listening at http://");
  Serial.print(mdnsName);
  Serial.println(".local/");

  // you're connected now, so print out the status:
  printWiFiStatus();
}


void loop() {
  // Call the update() function on the MDNS responder every loop iteration to
  // make sure it can detect and respond to name requests.
  mdnsResponder.poll();

  // listen for incoming clients
  WiFiClient client = server.available();
  if (client) {
    Serial.println("new client");
    // an HTTP request ends with a blank line
    bool currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the HTTP request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          // send a standard HTTP response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");  // the connection will be closed after completion of the response
          client.println("Refresh: 5");  // refresh the page automatically every 5 sec
          client.println();
          client.println("<!DOCTYPE HTML>");
          client.println("<html>");
          // output the value of each analog input pin
          for (int analogChannel = 0; analogChannel < 6; analogChannel++) {
            int sensorReading = analogRead(analogChannel);
            client.print("analog input ");
            client.print(analogChannel);
            client.print(" is ");
            client.print(sensorReading);
            client.println("<br />");
          }
          client.println("</html>");
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        }
        else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);

    // close the connection:
    client.stop();
    Serial.println("client disconnected");
  }
}


void printWiFiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi 101 Shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}
```

### `WiFi.end()`
#### Description
Turns off the WiFi module. If WiFi.begin() was used to connect to an access point, the connection will be disconnected.
If WiFi.beginAP() was used before to create an access point, the WiFi.end() will stop listening it too.

#### Syntax
```
WiFi.end();
```
#### Parameters
none

#### Returns 
nothing

### `WiFi.beginAP()`
#### Description
Initializes the WiFi101 library in Access Point (AP) mode. Other WiFi devices will be able to discover and connect to the created Access Point.

#### Syntax
```
WiFi.beginAP(ssid);
WiFi.beginAP(ssid, channel);
WiFi.begin(ssid, keyIndex, key);
WiFi.begin(ssid, keyIndex, key, channel);
```
#### Parameters
ssid: the SSID (Service Set Identifier) of the created Access Point.

keyIndex: optional, WEP encrypted networks can hold up to 4 different keys. This identifies which key you are going to use.

key: optional, a hexadecimal string used as the security code for WEP encrypted network.

channel: optional, channel of created Access Point (1 - 14). Defaults to channel 1;

#### Returns 
WL_AP_LISTENING when creating access point succeeds
WL_CONNECT_FAILED when creating access point fails
#### Example
```
/*
  WiFi Web Server LED Blink

  A simple web server that lets you blink an LED via the web.
  This sketch will create a new access point (with no password).
  It will then launch a new server and print out the IP address
  to the Serial monitor. From there, you can open that address in a web browser
  to turn on and off the LED on pin 13.

  If the IP address of your shield is yourAddress:
    http://yourAddress/H turns the LED on
    http://yourAddress/L turns it off

  created 25 Nov 2012
  by Tom Igoe
  adapted to WiFi AP by Adafruit
 */

#include <SPI.h>
#include <WiFi101.h>
#include "arduino_secrets.h"
///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;                // your network key Index number (needed only for WEP)

int led =  LED_BUILTIN;
int status = WL_IDLE_STATUS;
WiFiServer server(80);

void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  Serial.println("Access Point Web Server");

  pinMode(led, OUTPUT);      // set the LED pin mode

  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi 101 Shield not present");
    // don't continue
    while (true);
  }

  // by default the local IP address of will be 192.168.1.1
  // you can override it with the following:
  // WiFi.config(IPAddress(10, 0, 0, 1));

  // print the network name (SSID);
  Serial.print("Creating access point named: ");
  Serial.println(ssid);

  // Create open network. Change this line if you want to create a WEP network:
  status = WiFi.beginAP(ssid);
  if (status != WL_AP_LISTENING) {
    Serial.println("Creating access point failed");
    // don't continue
    while (true);
  }

  // wait 10 seconds for connection:
  delay(10000);

  // start the web server on port 80
  server.begin();

  // you're connected now, so print out the status
  printWiFiStatus();
}


void loop() {
  // compare the previous status to the current status
  if (status != WiFi.status()) {
    // it has changed, so update the variable
    status = WiFi.status();

    if (status == WL_AP_CONNECTED) {
      byte remoteMac[6];

      // a device has connected to the AP
      Serial.print("Device connected to AP, MAC address: ");
      WiFi.APClientMacAddress(remoteMac);
      printMacAddress(remoteMac);
    } else {
      // a device has disconnected from the AP, and we are back in listening mode
      Serial.println("Device disconnected from AP");
    }
  }
 
  WiFiClient client = server.available();   // listen for incoming clients

  if (client) {                             // if you get a client,
    Serial.println("new client");           // print a message out the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there are bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        if (c == '\n') {                    // if the byte is a newline character

          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();

            // the content of the HTTP response follows the header:
            client.print("Click <a href=\"/H\">here</a> turn the LED on<br>");
            client.print("Click <a href=\"/L\">here</a> turn the LED off<br>");

            // The HTTP response ends with another blank line:
            client.println();
            // break out of the while loop:
            break;
          }
          else {      // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        }
        else if (c != '\r') {    // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }

        // Check to see if the client request was "GET /H" or "GET /L":
        if (currentLine.endsWith("GET /H")) {
          digitalWrite(led, HIGH);               // GET /H turns the LED on
        }
        if (currentLine.endsWith("GET /L")) {
          digitalWrite(led, LOW);                // GET /L turns the LED off
        }
      }
    }
    // close the connection:
    client.stop();
    Serial.println("client disconnected");
  }
}

void printWiFiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi 101 Shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
  // print where to go in a browser:
  Serial.print("To see this page in action, open a browser to http://");
  Serial.println(ip);

}

void printMacAddress(byte mac[]) {
  for (int i = 5; i >= 0; i--) {
    if (mac[i] < 16) {
      Serial.print("0");
    }
    Serial.print(mac[i], HEX);
    if (i > 0) {
      Serial.print(":");
    }
  }
  Serial.println();
}
```

### `WiFi.disconnect()`
#### Description
Disconnects the Arduino WiFi Shield 101 from the current network.

#### Syntax
```
WiFi.disconnect();
```
#### Parameters
none

#### Returns 
nothing

### `WiFi.config()`
#### Description
WiFi.config() allows you to configure a static IP address as well as change the DNS, gateway, and subnet addresses on the WiFi shield.

Unlike WiFi.begin() which automatically configures the WiFi shield to use DHCP, WiFi.config() allows you to manually set the network address of the shield.

Calling WiFi.config() before WiFi.begin() forces begin() to configure the WiFi shield with the network addresses specified in config().

You can call WiFi.config() after WiFi.begin(), but the shield will initialize with begin() in the default DHCP mode. Once the config() method is called, it will change the network address as requested.

#### Syntax
```
WiFi.config(ip);
WiFi.config(ip, dns);
WiFi.config(ip, dns, gateway);
WiFi.config(ip, dns, gateway, subnet);
```
#### Parameters
ip: the IP address of the device (array of 4 bytes)

dns: the address for a DNS server.

gateway: the IP address of the network gateway (array of 4 bytes). optional: defaults to the device IP address with the last octet set to 1

subnet: the subnet mask of the network (array of 4 bytes). optional: defaults to 255.255.255.0

#### Returns 
Nothing

#### Example
```
This example shows how to set the static IP address, 192.168.0.177, of the LAN network to the Arduino WiFi Shield 101:

#include <SPI.h>
#include <WiFi101.h>

// the IP address for the shield:
IPAddress ip(192, 168, 0, 177);    

char ssid[] = "yourNetwork";    // your network SSID (name)
char pass[] = "secretPassword"; // your network password (use for WPA, or use as key for WEP)

int status = WL_IDLE_STATUS;

void setup()
{  
  // Initialize serial and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }

  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    while(true);  // don't continue
  }

  WiFi.config(ip);

  // attempt to connect to Wifi network:
  while ( status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:    
    status = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    delay(10000);
  }

  // print your WiFi shield's IP address:
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void loop () {}
```

### `WiFi.setDNS()`
#### Description
WiFi.setDNS() allows you to configure the DNS (Domain Name System) server.

#### Syntax
```
WiFi.setDNS(dns_server1)
WiFi.setDNS(dns_server1, dns_server2)
```
#### Parameters
dns_server1: the IP address of the primary DNS server

dns_server2: the IP address of the secondary DNS server

#### Returns 
Nothing

#### Example
```
This example shows how to set the Google DNS (8.8.8.8). You can set it as an object IPAddress.

#include <SPI.h>
#include <WiFi101.h>

// the IP address for the shield:
IPAddress dns(8, 8, 8, 8);  //Google dns  

char ssid[] = "yourNetwork";    // your network SSID (name)
char pass[] = "secretPassword"; // your network password (use for WPA, or use as key for WEP)

int status = WL_IDLE_STATUS;

void setup()
{  
  // Initialize serial and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }

  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    while(true);  // don't continue
  }

  // attempt to connect to Wifi network:
  while ( status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:    
    status = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    delay(10000);
  }

  // print your WiFi shield's IP address:
  WiFi.setDNS(dns);
  Serial.print("Dns configured.");
}

void loop () {
}
 
```

### `WiFi.SSID()`
#### Description
Gets the SSID of the current network

#### Syntax
```
WiFi.SSID();
WiFi.SSID(wifiAccessPoint)
```
#### Parameters
wifiAccessPoint: specifies from which network to get the information

#### Returns 
A string containing the SSID the WiFi shield is currently connected to.

string containing name of network requested.

#### Example
```
#include <SPI.h>
#include <WiFi101.h>

//SSID of your network
char ssid[] = "yourNetwork";
int status = WL_IDLE_STATUS;     // the Wifi radio's status

void setup()
{
  // initialize serial:
  Serial.begin(9600);

  // scan for existing networks:
  Serial.println("Scanning available networks...");
  scanNetworks();

  // attempt to connect using WEP encryption:
  Serial.println("Attempting to connect to open network...");
  status = WiFi.begin(ssid);

  Serial.print("SSID: ");
  Serial.println(ssid);

}

void loop () {}

void scanNetworks() {
  // scan for nearby networks:
  Serial.println("** Scan Networks **");
  byte numSsid = WiFi.scanNetworks();

  // print the list of networks seen:
  Serial.print("SSID List:");
  Serial.println(numSsid);
  // print the network number and name for each network found:
  for (int thisNet = 0; thisNet<numSsid; thisNet++) {
    Serial.print(thisNet);
    Serial.print(") Network: ");
    Serial.println(WiFi.SSID(thisNet));
  }
}
 
```

### `WiFi.BSSID()`
#### Description
Gets the MAC address of the routher you are connected to

#### Syntax
```
WiFi.BSSID(bssid);
```
#### Parameters
bssid: 6 byte array

#### Returns 
A byte array containing the MAC address of the router the WiFi shield is currently connected to.

#### Example
```
#include <WiFi101.h>

//SSID of your network
char ssid[] = "yourNetwork";
//password of your WPA Network
char pass[] = "secretPassword";

void setup()
{
 WiFi.begin(ssid, pass);

  if ( status != WL_CONNECTED) {
    Serial.println("Couldn't get a wifi connection");
    while(true);
  }
  // if you are connected, print out info about the connection:
  else {
  // print the MAC address of the router you're attached to:
  byte bssid[6];
  WiFi.BSSID(bssid);    
  Serial.print("BSSID: ");
  Serial.print(bssid[5],HEX);
  Serial.print(":");
  Serial.print(bssid[4],HEX);
  Serial.print(":");
  Serial.print(bssid[3],HEX);
  Serial.print(":");
  Serial.print(bssid[2],HEX);
  Serial.print(":");
  Serial.print(bssid[1],HEX);
  Serial.print(":");
  Serial.println(bssid[0],HEX);
  }
}

void loop () {}
```

### `WiFi.RSSI()`
#### Description
Gets the signal strength of the connection to the router

#### Syntax
```
WiFi.RSSI();
WiFi.RSSI(wifiAccessPoint);
```
#### Parameters
wifiAccessPoint: specifies from which network to get the information

#### Returns 
long : the current RSSI /Received Signal Strength in dBm

#### Example
```
#include <SPI.h>
#include <WiFi101.h>

//SSID of your network
char ssid[] = "yourNetwork";
//password of your WPA Network
char pass[] = "secretPassword";

void setup()
{
 WiFi.begin(ssid, pass);

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Couldn't get a wifi connection");
    while(true);
  }
  // if you are connected, print out info about the connection:
  else {
   // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("RSSI:");
  Serial.println(rssi);
  }
}

void loop () {}
```

### `WiFi.encryptionType()`
#### Description
Gets the encryption type of the current network

#### Syntax
```
WiFi.encryptionType();
WiFi.encryptionType(wifiAccessPoint);
```
#### Parameters
wifiAccessPoint: specifies which network to get information from
#### Returns 
byte : value represents the type of encryption

TKIP (WPA) = 2
WEP = 5
CCMP (WPA) = 4
NONE = 7
AUTO = 8
#### Example
```
#include <SPI.h>
#include <WiFi101.h>

//SSID of your network
char ssid[] = "yourNetwork";
//password of your WPA Network
char pass[] = "secretPassword";

void setup()
{
 WiFi.begin(ssid, pass);

  if ( status != WL_CONNECTED) {
    Serial.println("Couldn't get a wifi connection");
    while(true);
  }
  // if you are connected, print out info about the connection:
  else {
   // print the encryption type:
  byte encryption = WiFi.encryptionType();
  Serial.print("Encryption Type:");
  Serial.println(encryption,HEX);
  }
}

void loop () {}
```

### `WiFi.scanNetworks()`
#### Description
Scans for available WiFi networks and returns the discovered number

#### Syntax
```
WiFi.scanNetworks();
```
#### Parameters
none

#### Returns 
byte : number of discovered networks

#### Example
```
#include <SPI.h>
#include <WiFi101.h>

char ssid[] = "yourNetwork";     // the name of your network
int status = WL_IDLE_STATUS;     // the Wifi radio's status

byte mac[6];                     // the MAC address of your Wifi shield


void setup()
{
 Serial.begin(9600);

 status = WiFi.begin(ssid);

 if ( status != WL_CONNECTED) {
    Serial.println("Couldn't get a wifi connection");
    while(true);
  }
  // if you are connected, print your MAC address:
  else {

  Serial.println("** Scan Networks **");
  byte numSsid = WiFi.scanNetworks();

  Serial.print("SSID List:");
  Serial.println(numSsid);
  }
}

void loop () {}
```

### `WiFi.ping()`
#### Description
Ping a remote device on the network.

#### Syntax
```
WiFi.ping(ip);
WiFi.begin(ip, ttl);
WiFi.begin(host);
WiFi.begin(host, ttl);
```
#### Parameters
ip: the IP address to ping (array of 4 bytes)

host: the host to ping (string)

ttl: Time of live (optional, defaults to 128). Maximum number of routers the request can be forwarded to.

#### Returns 
WL_PING_SUCCESS when the ping was successful
WL_PING_DEST_UNREACHABLE when the destination (IP or host is unreachable)
WL_PING_TIMEOUT when the ping times out
WL_PING_UNKNOWN_HOST when the host cannot be resolved via DNS
WL_PING_ERROR when an error occurs
#### Example
```
/*

  This example connects to a encrypted WiFi network (WPA/WPA2).
  Then it prints the MAC address of the WiFi 101 Shield,
  the IP address obtained, and other network details.
  Then it continuously pings given host specified by IP Address or name.

  Circuit:
   WiFi 101 Shield attached / MKR1000

  created 13 July 2010
  by dlf (Metodo2 srl)
  modified 09 June 2016
  by Petar Georgiev
*/
#include <SPI.h>
#include <WiFi101.h>

#include "arduino_secrets.h"
///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)
int status = WL_IDLE_STATUS;     // the WiFi radio's status

// Specify IP address or hostname
String hostName = "www.google.com";
int pingResult;

void setup() {
  // Initialize serial and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi 101 Shield not present");
    // don't continue:
    while (true);
  }

  // attempt to connect to WiFi network:
  while ( status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network:
    status = WiFi.begin(ssid, pass);

    // wait 5 seconds for connection:
    delay(5000);
  }

  // you're connected now, so print out the data:
  Serial.println("You're connected to the network");
  printCurrentNet();
  printWiFiData();
}

void loop() {
  Serial.print("Pinging ");
  Serial.print(hostName);
  Serial.print(": ");

  pingResult = WiFi.ping(hostName);

  if (pingResult >= 0) {
    Serial.print("SUCCESS! RTT = ");
    Serial.print(pingResult);
    Serial.println(" ms");
  } else {
    Serial.print("FAILED! Error code: ");
    Serial.println(pingResult);
  }

  delay(5000);
}

void printWiFiData() {
  // print your WiFi 101 Shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP address : ");
  Serial.println(ip);

  Serial.print("Subnet mask: ");
  Serial.println((IPAddress)WiFi.subnetMask());

  Serial.print("Gateway IP : ");
  Serial.println((IPAddress)WiFi.gatewayIP());

  // print your MAC address:
  byte mac[6];
  WiFi.macAddress(mac);
  Serial.print("MAC address: ");
  printMacAddress(mac);
}

void printCurrentNet() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print the MAC address of the router you're attached to:
  byte bssid[6];
  WiFi.BSSID(bssid);
  Serial.print("BSSID: ");
  printMacAddress(bssid);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI): ");
  Serial.println(rssi);

  // print the encryption type:
  byte encryption = WiFi.encryptionType();
  Serial.print("Encryption Type: ");
  Serial.println(encryption, HEX);
  Serial.println();
}

void printMacAddress(byte mac[]) {
  for (int i = 5; i >= 0; i--) {
    if (mac[i] < 16) {
      Serial.print("0");
    }
    Serial.print(mac[i], HEX);
    if (i > 0) {
      Serial.print(":");
    }
  }
  Serial.println();
}
```

### `WiFi.status()`
#### Description
Return the connection status.

#### Syntax
```
WiFi.status();
```
#### Parameters
none

#### Returns 
WL_CONNECTED: assigned when connected to a WiFi network;
WL_AP_CONNECTED : assigned when a device is connected in Access Point mode;
 WL_AP_LISTENING : assigned when the listening for connections in Access Point mode;
WL_NO_SHIELD: assigned when no WiFi shield is present;
WL_IDLE_STATUS: it is a temporary status assigned when WiFi.begin() is called and remains active until the number of attempts expires (resulting in WL_CONNECT_FAILED) or a connection is established (resulting in WL_CONNECTED);
WL_NO_SSID_AVAIL: assigned when no SSID are available;
WL_SCAN_COMPLETED: assigned when the scan networks is completed;
WL_CONNECT_FAILED: assigned when the connection fails for all the attempts;
WL_CONNECTION_LOST: assigned when the connection is lost;
WL_DISCONNECTED: assigned when disconnected from a network;
#### Example
```

#include <SPI.h>
#include <WiFi101.h>

char ssid[] = "yourNetwork";                     // your network SSID (name)
char key[] = "D0D0DEADF00DABBADEAFBEADED";       // your network key
int keyIndex = 0;                                // your network key Index number
int status = WL_IDLE_STATUS;                     // the Wifi radio's status

void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }

  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    // don't continue:
    while (true);
  }

  // attempt to connect to Wifi network:
  while ( status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WEP network, SSID: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, keyIndex, key);

    // wait 10 seconds for connection:
    delay(10000);
  }

  // once you are connected :
  Serial.print("You're connected to the network");
}

void loop() {
  // check the network status connection once every 10 seconds:
  delay(10000);
 Serial.println(WiFi.status());
}

 
```

### `WiFi.macAddress()`
#### Description
Gets the MAC Address of your WiFi shield

#### Syntax
```
WiFi.macAddress(mac);
```
#### Parameters
mac: a 6 byte array to hold the MAC address

#### Returns 
byte array : 6 bytes representing the MAC address of your shield

#### Example
```
#include <SPI.h>
#include <WiFi101.h>

char ssid[] = "yourNetwork";     // the name of your network
int status = WL_IDLE_STATUS;     // the Wifi radio's status

byte mac[6];                     // the MAC address of your Wifi shield


void setup()
{
 Serial.begin(9600);

 status = WiFi.begin(ssid);

 if ( status != WL_CONNECTED) {
    Serial.println("Couldn't get a wifi connection");
    while(true);
  }
  // if you are connected, print your MAC address:
  else {
  WiFi.macAddress(mac);
  Serial.print("MAC: ");
  Serial.print(mac[5],HEX);
  Serial.print(":");
  Serial.print(mac[4],HEX);
  Serial.print(":");
  Serial.print(mac[3],HEX);
  Serial.print(":");
  Serial.print(mac[2],HEX);
  Serial.print(":");
  Serial.print(mac[1],HEX);
  Serial.print(":");
  Serial.println(mac[0],HEX);
  }
}

void loop () {}
```

### `WiFi.lowPowerMode()`
#### Description
Enable low power mode. This is an automatically managed mode where the WINC1500 reduces its power drain to some 30/40 mA. Any incoming data is received and the device sends out regularly the beacon signal each 100ms to keep the AP connection alive. This mode is useful when you expect to send and receive data frequently.

#### Syntax
```
WiFi.lowPowerMode();
```
#### Returns 
none

### `WiFi.maxLowPowerMode()`
#### Description
Maximize power saving. Anytime you transmit or during a beacon interval (100ms) the chip will wake up. During the beacon interval, the AP will also notify the WiFi chip if there is any incoming data. So in max power saving, the TX side will be delayed because there is penalty to wake up the chip if it’s sleeping. This mode is useful when you expect to receive data frequently, but you don't send data often.

#### Syntax
```
WiFi.maxLowPowerMode();
```
#### Returns 
none

### `WiFi.noLowPowerMode()`
#### Description
Disable any kind of power saving mode. This is the default option.

#### Syntax
```
WiFi.noLowPowerMode();
```
#### Returns 
none

### `WiFi.APClientMacAddress()`
#### Description
Gets the MAC Address of the client connected in AP (access point) mode.

#### Syntax
```
WiFi.APClientMacAddress(mac);
```
#### Parameters
mac: a 6 byte array to hold the MAC address

#### Returns 
byte array : 6 bytes representing the MAC address of the client connected to the AP. Will be 00:00:00:00:00:00 if there is no client connected.

### `WiFi.getTime()`
#### Description
Get the time in seconds since January 1st, 1970. The time is retrieved from the WiFi module which periodically fetches the NTP time from an NTP server.

#### Syntax
```
WiFi.getTime();
```
#### Parameters
none

#### Returns 
Returns the time in seconds since January 1st, 1970 on success. 0 on failure.

### `WiFi.setPins()`
#### Description
WiFi.setPins() allows you to override the default pins used to interface with the WINC1500 WiFi chip. When using the WiFi101 library with the WiFi101 shield or MKR1000 there is no need call WiFi.setPins() as the correct pins are configured by the library.

It MUST be called before using any other WiFi101 operations such as WiFi.status() or WiFi.begin() to take effect.

#### Syntax
```
WiFi.setPins(chipSelect, irq, reset, enable)
```
#### Parameters
chipSelect: new chip select pin to use, must be connected to SPI SS pin of the WINC1500

irq: new interrupt pin to use, must be connected to the IRQ pin of the WINC1500

reset: new reset pin to use, must be connected to the RESET pin of the WINC1600

enable: new enable pin to use, connected to the EN of the WINC1500, use -1 if not connected

#### Returns 
none

## IPAddress class

### `WiFi.localIP()`
#### Description
Gets the WiFi shield's IP address

#### Syntax
```
WiFi.localIP();
```
#### Parameters
none

#### Returns 
the IP address of the shield

#### Example
```
#include <WiFi101.h>

char ssid[] = "yourNetwork";      //SSID of your network

int status = WL_IDLE_STATUS;     // the Wifi radio's status

IPAddress ip;                    // the IP address of your shield

void setup()
{
 // initialize serial:
 Serial.begin(9600);

 WiFi.begin(ssid);

  if ( status != WL_CONNECTED) {
    Serial.println("Couldn't get a wifi connection");
    while(true);
  }
  // if you are connected, print out info about the connection:
  else {
 //print the local IP address
  ip = WiFi.localIP();
  Serial.println(ip);

  }
}

void loop () {}
```

### `WiFi.subnetMask()`
#### Description
Gets the WiFi shield's subnet mask

#### Syntax
```
WiFi.subnet();
```
#### Parameters
none

#### Returns 
the subnet mask of the shield

#### Example
```
#include <WiFi101.h>
int status = WL_IDLE_STATUS;     // the Wifi radio's status

//SSID of your network
char ssid[] = "yourNetwork";
//password of your WPA Network
char pass[] = "secretPassword";

IPAddress ip;
IPAddress subnet;
IPAddress gateway;

void setup()
{
  WiFi.begin(ssid, pass);

  if ( status != WL_CONNECTED) {
    Serial.println("Couldn't get a wifi connection");
    while(true);
  }
  // if you are connected, print out info about the connection:
  else {

    // print your subnet mask:
    subnet = WiFi.subnetMask();
    Serial.print("NETMASK: ");
    Serial.println();

  }
}

void loop () {
}
 
```

### `WiFi.gatewayIP()`
#### Description
Gets the WiFi shield's gateway IP address.

#### Syntax
```
WiFi.gatewayIP();
```
#### Parameters
none

#### Returns 
An array containing the shield's gateway IP address

#### Example
```
#include <SPI.h>
#include <WiFi101.h>

int status = WL_IDLE_STATUS;     // the Wifi radio's status

//SSID of your network
char ssid[] = "yourNetwork";
//password of your WPA Network
char pass[] = "secretPassword";

IPAddress gateway;

void setup()
{
  Serial.begin(9600);

 WiFi.begin(ssid, pass);

  if ( status != WL_CONNECTED) {
    Serial.println("Couldn't get a wifi connection");
    while(true);
  }
  // if you are connected, print out info about the connection:
  else {

  // print your gateway address:
  gateway = WiFi.gatewayIP();
  Serial.print("GATEWAY: ");
  Serial.println(gateway);

  }
}

void loop () {}
```

## Server class

### `Server`
#### Description
Server is the base class for all WiFi server based calls. It is not called directly, but invoked whenever you use a function that relies on it.

### `WiFiServer()`
#### Description
Creates a server that listens for incoming connections on the specified port.

#### Syntax
```
Server(port);
```
#### Parameters
port: the port to listen on (int)

#### Returns 
None

#### Example
```
#include <SPI.h>
#include <WiFi101.h>

char ssid[] = "myNetwork";          //  your network SSID (name)
char pass[] = "myPassword";   // your network password
int status = WL_IDLE_STATUS;

WiFiServer server(80);

void setup() {
  // initialize serial:
  Serial.begin(9600);
  Serial.println("Attempting to connect to WPA network...");
  Serial.print("SSID: ");
  Serial.println(ssid);

  status = WiFi.begin(ssid, pass);
  if ( status != WL_CONNECTED) {
    Serial.println("Couldn't get a wifi connection");
    while(true);
  }
  else {
    server.begin();
    Serial.print("Connected to wifi. My address:");
    IPAddress myAddress = WiFi.localIP();
    Serial.println(myAddress);

  }
}


void loop() {

}
```

### `begin()`
#### Description
Tells the server to begin listening for incoming connections.

#### Syntax
```
server.begin()
```
#### Parameters
None

#### Returns 
None

#### Example
```
#include <SPI.h>
#include <WiFi101.h>

char ssid[] = "lamaison";          //  your network SSID (name)
char pass[] = "tenantaccess247";   // your network password
int status = WL_IDLE_STATUS;

WiFiServer server(80);

void setup() {
  // initialize serial:
  Serial.begin(9600);
  Serial.println("Attempting to connect to WPA network...");
  Serial.print("SSID: ");
  Serial.println(ssid);

  status = WiFi.begin(ssid, pass);
  if ( status != WL_CONNECTED) {
    Serial.println("Couldn't get a wifi connection");
    while(true);
  }
  else {
    server.begin();
    Serial.print("Connected to wifi. My address:");
    IPAddress myAddress = WiFi.localIP();
    Serial.println(myAddress);

  }
}


void loop() {

}
 
```

### `available()`
#### Description
Gets a client that is connected to the server and has data available for reading. The connection persists when the returned client object goes out of scope; you can close it by calling client.stop().

available() inherits from the Stream utility class.

#### Syntax
```
server.available()
```
#### Parameters
None

#### Returns 
a Client object; if no Client has data available for reading, this object will evaluate to false in an if-statement

#### Example
```
#include <SPI.h>
#include <WiFi101.h>

char ssid[] = "Network";          //  your network SSID (name)
char pass[] = "myPassword";   // your network password
int status = WL_IDLE_STATUS;

WiFiServer server(80);

void setup() {
  // initialize serial:
  Serial.begin(9600);
  Serial.println("Attempting to connect to WPA network...");
  Serial.print("SSID: ");
  Serial.println(ssid);

  status = WiFi.begin(ssid, pass);
  if ( status != WL_CONNECTED) {
    Serial.println("Couldn't get a wifi connection");
    while(true);
  }
  else {
    server.begin();
    Serial.print("Connected to wifi. My address:");
    IPAddress myAddress = WiFi.localIP();
    Serial.println(myAddress);

  }
}

void loop() {
  // listen for incoming clients
  WiFiClient client = server.available();
  if (client) {

    if (client.connected()) {
      Serial.println("Connected to client");
    }

    // close the connection:
    client.stop();
  }
}
 
```

### `write()`
#### Description
Write data to all the clients connected to a server.

#### Syntax
```
server.write(data)
```
#### Parameters
data: the value to write (byte or char)

#### Returns 
byte : the number of bytes written. It is not necessary to read this.

#### Example
```
#include <SPI.h>
#include <WiFi101.h>

char ssid[] = "yourNetwork";
char pass[] = "yourPassword";
int status = WL_IDLE_STATUS;

WiFiServer server(80);

void setup() {
  // initialize serial:
  Serial.begin(9600);
  Serial.println("Attempting to connect to WPA network...");
  Serial.print("SSID: ");
  Serial.println(ssid);

  status = WiFi.begin(ssid, pass);
  if ( status != WL_CONNECTED) {
    Serial.println("Couldn't get a wifi connection");
    while(true);
  }
  else {
    server.begin();
  }
}

void loop() {
  // listen for incoming clients
  WiFiClient client = server.available();
  if (client == true) {
       // read bytes from the incoming client and write them back
    // to any clients connected to the server:
    server.write(client.read());
  }
}
```

### `print()`
#### Description
Print data to all the clients connected to a server. Prints numbers as a sequence of digits, each an ASCII character (e.g. the number 123 is sent as the three characters '1', '2', '3').

#### Syntax
```
server.print(data)
server.print(data, BASE)
```
#### Parameters
data: the data to print (char, byte, int, long, or string)

BASE (optional): the base in which to print numbers: BIN for binary (base 2), DEC for decimal (base 10), OCT for octal (base 8), HEX for hexadecimal (base 16).

#### Returns 
byte
print() will return the number of bytes written, though reading that number is optional

### `println()`
#### Description
Prints data, followed by a newline, to all the clients connected to a server. Prints numbers as a sequence of digits, each an ASCII character (e.g. the number 123 is sent as the three characters '1', '2', '3').

#### Syntax
```
server.println()
server.println(data)
server.println(data, BASE)
```
#### Parameters
data (optional): the data to print (char, byte, int, long, or string)

BASE (optional): the base in which to print numbers: DEC for decimal (base 10), OCT for octal (base 8), HEX for hexadecimal (base 16).

#### Returns 
byte
println() will return the number of bytes written, though reading that number is optional

##Client class

### `Client()`
#### Description
Client is the base class for all WiFi client based calls. It is not called directly, but invoked whenever you use a function that relies on it.

Functions

### `WiFiClient`
#### Description
Creates a client that can connect to to a specified internet IP address and port as defined in client.connect().

#### Syntax
```
WiFiClient
```
#### Parameters
client : the named client to refer to

#### Returns 
none

#### Example
```
#include <SPI.h>
#include <WiFi101.h>

char ssid[] = "myNetwork";          //  your network SSID (name)
char pass[] = "myPassword";   // your network password

int status = WL_IDLE_STATUS;
IPAddress server(74,125,115,105);  // Google

// Initialize the client library
WiFiClient client;

void setup() {
  Serial.begin(9600);
  Serial.println("Attempting to connect to WPA network...");
  Serial.print("SSID: ");
  Serial.println(ssid);

  status = WiFi.begin(ssid, pass);
  if ( status != WL_CONNECTED) {
    Serial.println("Couldn't get a wifi connection");
    // don't do anything else:
    while(true);
  }
  else {
    Serial.println("Connected to wifi");
    Serial.println("\nStarting connection...");
    // if you get a connection, report back via serial:
    if (client.connect(server, 80)) {
      Serial.println("connected");
      // Make a HTTP request:
      client.println("GET /search?q=arduino HTTP/1.0");
      client.println();
    }
  }
}

void loop() {

}
```

### `WiFiClient`
#### Description
Creates a client that can connect to to a specified internet IP address and port as defined in client.connect().

#### Syntax
```
WiFiClient
```
#### Parameters
client : the named client to refer to

#### Returns 
none

#### Example
```
#include <SPI.h>
#include <WiFi101.h>

char ssid[] = "myNetwork";          //  your network SSID (name)
char pass[] = "myPassword";   // your network password

int status = WL_IDLE_STATUS;
IPAddress server(74,125,115,105);  // Google

// Initialize the client library
WiFiClient client;

void setup() {
  Serial.begin(9600);
  Serial.println("Attempting to connect to WPA network...");
  Serial.print("SSID: ");
  Serial.println(ssid);

  status = WiFi.begin(ssid, pass);
  if ( status != WL_CONNECTED) {
    Serial.println("Couldn't get a wifi connection");
    // don't do anything else:
    while(true);
  }
  else {
    Serial.println("Connected to wifi");
    Serial.println("\nStarting connection...");
    // if you get a connection, report back via serial:
    if (client.connect(server, 80)) {
      Serial.println("connected");
      // Make a HTTP request:
      client.println("GET /search?q=arduino HTTP/1.0");
      client.println();
    }
  }
}

void loop() {

}
```

### `WiFiSSLClient()`
#### Description
This class allows to create a client that always connects in SSL to the specified IP address and port, even if client.connect() is used instead of client.connectSSL(). This is useful If you have a library that accepts only plain Client, but you want to force it to use SSL, keeping the same method names of the non SSL client.

#### Syntax
```
WiFiSSLClient()
```
#### Parameters
none

#### Example
```
/*
This example creates a client object that connects and transfers
data using always SSL.

It is compatible with the methods normally related to plain
connections, like client.connect(host, port).

Written by Arturo Guadalupi
last revision November 2015

*/

#include <SPI.h>
#include <WiFi101.h>

char ssid[] = "yourNetwork"; //  your network SSID (name)
char pass[] = "secretPassword";    // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;            // your network key Index number (needed only for WEP)

int status = WL_IDLE_STATUS;
// if you don't want to use DNS (and reduce your sketch size)
// use the numeric IP instead of the name for the server:
//IPAddress server(74,125,232,128);  // numeric IP for Google (no DNS)
char server[] = "www.google.com";    // name address for Google (using DNS)

// Initialize the Ethernet client library
// with the IP address and port of the server
// that you want to connect to (port 80 is default for HTTP):
WiFiSSLClient client;

void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    // don't continue:
    while (true);
  }

  // attempt to connect to Wifi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    delay(10000);
  }
  Serial.println("Connected to wifi");
  printWifiStatus();

  Serial.println("\nStarting connection to server...");
  // if you get a connection, report back via serial:
  if (client.connect(server, 443)) {
    Serial.println("connected to server");
    // Make a HTTP request:
    client.println("GET /search?q=arduino HTTP/1.1");
    client.println("Host: www.google.com");
    client.println("Connection: close");
    client.println();
  }
}

void loop() {
  // if there are incoming bytes available
  // from the server, read them and print them:
  while (client.available()) {
    char c = client.read();
    Serial.write(c);
  }

  // if the server's disconnected, stop the client:
  if (!client.connected()) {
    Serial.println();
    Serial.println("disconnecting from server.");
    client.stop();

    // do nothing forevermore:
    while (true);
  }
}


void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}
```

### `connected()`
#### Description
Whether or not the client is connected. Note that a client is considered connected if the connection has been closed but there is still unread data.

#### Syntax
```
client.connected()
```
#### Parameters
none

#### Returns 
Returns true if the client is connected, false if not.

#### Example
```
#include <SPI.h>
#include <WiFi101.h>

char ssid[] = "myNetwork";          //  your network SSID (name)
char pass[] = "myPassword";   // your network password

int status = WL_IDLE_STATUS;
IPAddress server(74,125,115,105);  // Google

// Initialize the client library
WiFiClient client;

void setup() {
  Serial.begin(9600);
  Serial.println("Attempting to connect to WPA network...");
  Serial.print("SSID: ");
  Serial.println(ssid);

  status = WiFi.begin(ssid, pass);
  if ( status != WL_CONNECTED) {
    Serial.println("Couldn't get a wifi connection");
    // don't do anything else:
    while(true);
  }
  else {
    Serial.println("Connected to wifi");
    Serial.println("\nStarting connection...");
    // if you get a connection, report back via serial:
    if (client.connect(server, 80)) {
      Serial.println("connected");
      // Make a HTTP request:
      client.println("GET /search?q=arduino HTTP/1.0");
      client.println();
    }
  }
}

void loop() {
   if (client.available()) {
    char c = client.read();
    Serial.print(c);
  }

  if (!client.connected()) {
    Serial.println();
    Serial.println("disconnecting.");
    client.stop();
    for(;;)
      ;
  }
}
```

### `connect()`
#### Description
Connect to the IP address and port specified in the constructor. The return value indicates success or failure. connect() also supports DNS lookups when using a domain name (ex:google.com).

#### Syntax
```
client.connect(ip, port)
client.connect(URL, port)
```
#### Parameters
ip: the IP address that the client will connect to (array of 4 bytes)

URL: the domain name the client will connect to (string, ex.:"arduino.cc")

port: the port that the client will connect to (int)

#### Returns 
Returns true if the connection succeeds, false if not.

#### Example
```
#include <SPI.h>
#include <WiFi101.h>

char ssid[] = "myNetwork";          //  your network SSID (name)
char pass[] = "myPassword";   // your network password

int status = WL_IDLE_STATUS;
char servername[]="google.com";  // remote server we will connect to

WiFiClient client;

void setup() {
  Serial.begin(9600);
  Serial.println("Attempting to connect to WPA network...");
  Serial.print("SSID: ");
  Serial.println(ssid);

  status = WiFi.begin(ssid, pass);
  if ( status != WL_CONNECTED) {
    Serial.println("Couldn't get a wifi connection");
    // don't do anything else:
    while(true);
  }
  else {
    Serial.println("Connected to wifi");
    Serial.println("\nStarting connection...");
    // if you get a connection, report back via serial:
    if (client.connect(servername, 80)) {
      Serial.println("connected");
      // Make a HTTP request:
      client.println("GET /search?q=arduino HTTP/1.0");
      client.println();
    }
  }
}

void loop() {

}
```

### `ConnectSSL()`
#### Description
Connect to the IP address and port specified in the constructor using the SSL protocol. The method connectSSL is required when the server provides only HTTPS connections. Before using this method, it is required to load the SSL certificate used by the server into the Arduino WiFi Shield 101. The shield comes already loaded with certificates and it should be ready to use. To change or upload new SSL certificates you should follow the procedures that will be made available. connectSSL() also supports DNS lookups when using a domain name (ex:google.com).

#### Syntax
```
client.connectSSL(ip, port) client.connectSSL(URL, port)
```
#### Parameters
ip: the IP address that the client will connect to (array of 4 bytes)

URL: the domain name the client will connect to (string, ex.:"arduino.cc")

port: the port that the client will connect to (int)

#### Returns 
Returns true if the connection succeeds, false if not.

#### Example
```
/*
  Web client

 This sketch connects to a website through a SSL connection
 using a WiFi shield.

 This example is written for a network using WPA encryption. For
 WEP or WPA, change the Wifi.begin() call accordingly.

 Circuit:
 * WiFi101 shield attached

 created 13 July 2010
 by dlf (Metodo2 srl)
 modified 31 May 2012
 by Tom Igoe
 */


#include <SPI.h>
#include <WiFi101.h>

char ssid[] = "yourNetwork"; //  your network SSID (name)
char pass[] = "secretPassword";    // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;            // your network key Index number (needed only for WEP)

int status = WL_IDLE_STATUS;
char server[] = "www.arduino.cc";    // name address for Arduino (using DNS)

// Initialize the Wifi client library
// with the IP address and port of the server
// that you want to connect to (port 80 is default for HTTP):
WiFiClient client;

void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }

  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    // don't continue:
    while (true);
  }

  // attempt to connect to Wifi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    delay(10000);
  }
  Serial.println("Connected to wifi");

  Serial.println("\nStarting connection to server...");
  // if you get a connection, report back via serial:
  if (client.connectSSL(server, 443)) {
    Serial.println("Connected to server");
    // Make a HTTP request:
    client.println("GET /asciilogo.txt HTTP/1.1");
    client.println("Host: www.arduino.cc");
    client.println("Connection: close");
    client.println();
    Serial.println("Request sent");
  }
}

void loop() {
  // if there are incoming bytes available
  // from the server, read them and print them:
  while (client.available()) {
    char c = client.read();
    Serial.write(c);
  }

  // if the server's disconnected, stop the client:
  if (!client.connected()) {
    Serial.println();
    Serial.println("disconnecting from server.");
    client.stop();

    // do nothing forevermore:
    while (true);
  }
}
```

### `write()`
#### Description
Write data to the server the client is connected to.

#### Syntax
```
client.write(data)
```
#### Parameters
data: the byte or char to write

#### Returns 
byte: the number of characters written. it is not necessary to read this value.

### `print()`
#### Description
Print data to the server that a client is connected to. Prints numbers as a sequence of digits, each an ASCII character (e.g. the number 123 is sent as the three characters '1', '2', '3').

#### Syntax
```
client.print(data)
client.print(data, BASE)
```
#### Parameters
data: the data to print (char, byte, int, long, or string)

BASE (optional): the base in which to print numbers:, DEC for decimal (base 10), OCT for octal (base 8), HEX for hexadecimal (base 16).

#### Returns 
byte : returns the number of bytes written, though reading that number is optional

### `println()`
#### Description
Print data, followed by a carriage return and newline, to the server a client is connected to. Prints numbers as a sequence of digits, each an ASCII character (e.g. the number 123 is sent as the three characters '1', '2', '3').

#### Syntax
```
client.println()
client.println(data)
client.print(data, BASE)
```
#### Parameters
data (optional): the data to print (char, byte, int, long, or string)

BASE (optional): the base in which to print numbers: DEC for decimal (base 10), OCT for octal (base 8), HEX for hexadecimal (base 16).

#### Returns 
byte: return the number of bytes written, though reading that number is optional

### `available()`
#### Description
Returns the number of bytes available for reading (that is, the amount of data that has been written to the client by the server it is connected to).

available() inherits from the Stream utility class.

#### Syntax
```
client.available()
```
#### Parameters
none

#### Returns 
The number of bytes available.

#### Example
```
#include <SPI.h>
#include <WiFi101.h>

char ssid[] = "myNetwork";          //  your network SSID (name)
char pass[] = "myPassword";   // your network password

int status = WL_IDLE_STATUS;
char servername[]="google.com";  // Google

WiFiClient client;

void setup() {
  Serial.begin(9600);
  Serial.println("Attempting to connect to WPA network...");
  Serial.print("SSID: ");
  Serial.println(ssid);

  status = WiFi.begin(ssid, pass);
  if ( status != WL_CONNECTED) {
    Serial.println("Couldn't get a wifi connection");
    // don't do anything else:
    while(true);
  }
  else {
    Serial.println("Connected to wifi");
    Serial.println("\nStarting connection...");
    // if you get a connection, report back via serial:
    if (client.connect(servername, 80)) {
      Serial.println("connected");
      // Make a HTTP request:
      client.println("GET /search?q=arduino HTTP/1.0");
      client.println();
    }
  }
}

void loop() {
  // if there are incoming bytes available
  // from the server, read them and print them:
  if (client.available()) {
    char c = client.read();
    Serial.print(c);
  }

  // if the server's disconnected, stop the client:
  if (!client.connected()) {
    Serial.println();
    Serial.println("disconnecting.");
    client.stop();

    // do nothing forevermore:
    for(;;)
      ;
  }
}
```

### `read()`
Read the next byte received from the server the client is connected to (after the last call to read()).

read() inherits from the Stream utility class.

#### Syntax
```
client.read()
```
#### Parameters
none

#### Returns 
The next byte (or character), or -1 if none is available.

### `flush()`
Discard any bytes that have been written to the client but not yet read.

flush() inherits from the Stream utility class.

#### Syntax
```
client.flush()

```
#### Parameters
none

#### Returns 
none

### `stop()`
Disconnect from the server

#### Syntax
```
client.stop()
```
#### Parameters
none

#### Returns 
none

## UDP class

### `WiFiUDP`
#### Description
Creates a named instance of the WiFi UDP class that can send and receive UDP messages.

#### Syntax
```
WiFiUDP
```
#### Parameters
none

### `begin()` 

#### Description
Initializes the WiFi UDP library and network settings. Starts WiFiUDP socket, listening at local port PORT .

#### Syntax
```
WiFiUDP.begin(port);
```
#### Parameters
port: the local port to listen on (int)

#### Returns 
1: if successful
0: if there are no sockets available to use

### `available()`
#### Description
Get the number of bytes (characters) available for reading from the buffer. This is data that's already arrived.

This function can only be successfully called after WiFiUDP.parsePacket().

available() inherits from the Stream utility class.

#### Syntax
```
WiFiUDP.available()
```
#### Parameters
None

#### Returns 
the number of bytes available in the current packet
0: if WiFiUDP.parsePacket() hasn't been called yet

### `beginPacket()`

#### Description
Starts a connection to write UDP data to the remote connection

#### Syntax
```
WiFiUDP.beginPacket(hostName, port);
WiFiUDP.beginPacket(hostIp, port);
```
#### Parameters
hostName: the address of the remote host. It accepts a character string or an IPAddress

hostIp: the IP address of the remote connection (4 bytes)
port: the port of the remote connection (int)
#### Returns 
1: if successful
0: if there was a problem with the supplied IP address or port

### `endPacket()`

#### Description
Called after writing UDP data to the remote connection. It finishes off the packet and send it.

#### Syntax
```
WiFiUDP.endPacket();
```
#### Parameters
None

#### Returns 
1: if the packet was sent successfully
0: if there was an error

### `write()`

#### Description
Writes UDP data to the remote connection. Must be wrapped between beginPacket() and endPacket(). beginPacket() initializes the packet of data, it is not sent until endPacket() is called.

#### Syntax
```
WiFiUDP.write(byte);
WiFiUDP.write(buffer, size);
```
#### Parameters
byte: the outgoing byte

buffer: the outgoing message

size: the size of the buffer
#### Returns 
single byte into the packet
bytes size from buffer into the packet

### `parsePacket()`

#### Description
It starts processing the next available incoming packet, checks for the presence of a UDP packet, and reports the size. parsePacket() must be called before reading the buffer with UDP.read().

#### Syntax
```
UDP.parsePacket();
```
#### Parameters
None

#### Returns 
the size of the packet in bytes
0: if no packets are available

### `peek()`
Read a byte from the file without advancing to the next one. That is, successive calls to peek() will return the same value, as will the next call to read().

This function inherited from the Stream class. See the Stream class main page for more information.

#### Syntax
```
WiFiUDP.peek()
```
#### Parameters
none

#### Returns 
b: the next byte or character
-1: if none is available

### `read()`

#### Description
Reads UDP data from the specified buffer. If no arguments are given, it will return the next character in the buffer.

This function can only be successfully called after WiFiUDP.parsePacket().

#### Syntax
```
WiFiUDP.read();
WiFiUDP.read(buffer, len);
```
#### Parameters
buffer: buffer to hold incoming packets (char*)

len: maximum size of the buffer (int)

#### Returns 
b: the characters in the buffer (char)
size: the size of the buffer
-1: if no buffer is available

### `flush()`
Discard any bytes that have been written to the client but not yet read.

flush() inherits from the Stream utility class.

#### Syntax
```
WiFiUDP.flush()
```
#### Parameters
none

#### Returns 
none

### `stop()`
#### Description
Disconnect from the server. Release any resource being used during the UDP session.

#### Syntax
```
WiFiUDP.stop()
```
#### Parameters
none

#### Returns 
none

### `remoteIP()`

#### Description
Gets the IP address of the remote connection.

This function must be called after WiFiUDP.parsePacket().

#### Syntax
```
WiFiUDP.remoteIP();
```
#### Parameters
None

#### Returns 
4 bytes : the IP address of the host who sent the current incoming packet

### `remotePort()`

#### Description
Gets the port of the remote UDP connection.

This function must be called after UDP.parsePacket().

#### Syntax
```
UDP.remotePort();
```
#### Parameters
None

#### Returns 
The port of the host who sent the current incoming packet