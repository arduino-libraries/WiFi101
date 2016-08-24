/*

  This example makes an http request every 10s
  and puts in sleep mode WiFi in the meanwhile

  Circuit:
   WiFi shield 101 attached to an Arduino/Genuino board
   or Arduino/Genuino MKR1000 board

  https://www.arduino.cc/en/Tutorial/Wifi101WiFiSleepingWebClient

  created 18 Mar 2016
  by Arturo Guadalupi
*/

#include <SPI.h>
#include <WiFi101.h>

char ssid[] = "yourNetwork";     //  your network SSID (name)
char pass[] = "yourPassword";    // your network password
int status = WL_IDLE_STATUS;     // the Wifi radio's status

WiFiSSLClient client;

// server address:
char server[] = "www.arduino.cc";

void setup() {
  //Initialize Serial and wait for port to open:
  Serial.begin(9600);

  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    // don't continue:
    while (true);
  }
  // attempt to connect to WiFi network:

  while ( status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network:
    status = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    delay(10000);
  }

  // you're connected now, so print out the data:
  Serial.print("You're connected to the network");
  printCurrentNet();
  printWifiData();

  Serial.println("Sleeping...");
  WiFi.sleepFor(10000); //Sleep for 10s
}

void loop() {
  if (WiFi.isAwake()) {
    Serial.println("Awake!");
    httpRequest();
    listenToClient();
    WiFi.sleepFor(10000); //Sleep for 10s
    Serial.println("Sleeping");
  }
}

void printWifiData() {
  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print your MAC address:
  byte mac[6];
  WiFi.macAddress(mac);
  Serial.print("MAC address: ");
  Serial.print(mac[5], HEX);
  Serial.print(": ");
  Serial.print(mac[4], HEX);
  Serial.print(": ");
  Serial.print(mac[3], HEX);
  Serial.print(": ");
  Serial.print(mac[2], HEX);
  Serial.print(": ");
  Serial.print(mac[1], HEX);
  Serial.print(": ");
  Serial.println(mac[0], HEX);

}

void printCurrentNet() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print the MAC address of the router you're attached to:
  byte bssid[6];
  WiFi.BSSID(bssid);
  Serial.print("BSSID: ");
  Serial.print(bssid[5], HEX);
  Serial.print(": ");
  Serial.print(bssid[4], HEX);
  Serial.print(": ");
  Serial.print(bssid[3], HEX);
  Serial.print(": ");
  Serial.print(bssid[2], HEX);
  Serial.print(": ");
  Serial.print(bssid[1], HEX);
  Serial.print(": ");
  Serial.println(bssid[0], HEX);

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

void httpRequest() {

  if (client.connect(server, 443)) {
    // Make a HTTP request:
    client.println("GET /asciilogo.txt HTTP/1.1");
    client.println("Host: arduino.cc");
    client.println("Connection: close");
    client.println();
  }
  else {
    Serial.println("connection failed");
  }
}

void listenToClient()
{
  unsigned long startTime = millis();
  bool received = false;

  while ((millis() - startTime < 10000) && !received) { //try to listen for 10 seconds
    while (client.available()) {
      received = true;
      char c = client.read();
      Serial.write(c);
    }
  }
  client.stop();
  Serial.println();
}

