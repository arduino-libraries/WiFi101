/*
  Pager Server

  The example is a simple server that echoes any incoming
  messages to all connected clients. Connect two or more
  telnet sessions to see how server.available() and
  server.print() work.

 Circuit:
 * WiFi 101 Shield attached

*/

#include <WiFi101.h>

#include "arduino_secrets.h"
///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)

int status = WL_IDLE_STATUS;

WiFiServer server(23);

void setup() {

  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  // check for the WiFi module:
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi 101 Shield not present");
    // don't continue
    while (true);
  }

  // attempt to connect to WiFi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    delay(10000);
  }

  server.begin();

  IPAddress ip = WiFi.localIP();
  Serial.println();
  Serial.println("Connected to WiFi network.");
  Serial.print("To access the server, connect with Telnet client to ");
  Serial.print(ip);
  Serial.println(" 23");
}

void loop() {

  WiFiClient client = server.available(); // returns first client which has data to read or a 'false' client
  if (client) { // client is true only if it is connected and has data to read
    String s = client.readStringUntil('\n'); // read the message incoming from one of the clients
    s.trim(); // trim eventual \r
    Serial.println(s); // print the message to Serial Monitor
    client.print("echo: "); // this is only for the sending client
    server.println(s); // send the message to all connected clients
    server.flush(); // flush the buffers
  }
}
