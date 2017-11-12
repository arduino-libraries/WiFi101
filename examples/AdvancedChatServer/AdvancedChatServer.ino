/*
 Advanced Chat Server

 A more advanced server that distributes any incoming messages
 to all connected clients but the client the message comes from.
 To use telnet to  your device's IP address and type.
 You can see the client's input in the serial monitor as well.
 Using an Arduino Wiznet Ethernet shield.

 Circuit:
 * Ethernet shield attached to pins 10, 11, 12, 13
 * Analog inputs attached to pins A0 through A5 (optional)

 created 18 Dec 2009
 by David A. Mellis
 modified 9 Apr 2012
 by Tom Igoe
 redesigned to make use of operator== 25 Nov 2013
 by Norbert Truchsess

 */

#include <SPI.h>
#include <WiFi101.h>
#include "arduino_secrets.h"

///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)

int keyIndex = 0;            // your network key Index number (needed only for WEP)

int status = WL_IDLE_STATUS;

WiFiServer server(23);

WiFiClient clients[4];

void setup(){

    //Configure pins for Adafruit ATWINC1500 Feather
    WiFi.setPins(8, 7, 4, 2);

    //Initialize serial and wait for port to open:
    Serial.begin(9600);

    // wait for serial port to connect. Needed for native USB port only
    while(!Serial){}

    // check for the presence of the shield:
    if(WiFi.status() == WL_NO_SHIELD){
      Serial.println("WiFi shield not present");

      // don't continue:
      while(true);
    }

    // attempt to connect to WiFi network:
    while(status != WL_CONNECTED){
      Serial.print("Attempting to connect to SSID: ");
      Serial.println(ssid);

      // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
      status = WiFi.begin(ssid, pass);

      // wait 10 seconds for connection:
      delay(10000);
    }

    // start the server:
    server.begin();

    Serial.print("Chat server address: ");
    auto ip = WiFi.localIP();
    char *ip_bytes = reinterpret_cast<char*>(&ip), buf[64];
    sprintf(buf, "%d.%d.%d.%d", ip_bytes[0], ip_bytes[1], ip_bytes[2], ip_bytes[3]);
    Serial.println(buf);
}

void loop(){
    // wait for a new client:
    WiFiClient client = server.available();

    // when the client sends the first byte, say hello:
    if(client){

        boolean newClient = true;
        for(byte i = 0; i < 4; i++){

            //check whether this client refers to the same socket as one of the existing instances:
            if(clients[i] == client){
                newClient = false;
                break;
            }
        }

        if(newClient){

            //check which of the existing clients can be overridden:
            for(byte i = 0; i < 4; i++){
                if (!clients[i] && clients[i]!=client){
                  clients[i] = client;
                  // clead out the input buffer:
                  client.flush();
                  Serial.println("We have a new client");
                  client.print("Hello, client number: ");
                  client.print(i);
                  client.println();
                  break;
                }
            }
        }

        if(client.available() > 0){

            // read the bytes incoming from the client:
            char thisChar = client.read();

            // echo the bytes back to all other connected clients:
            for(byte i = 0; i < 4; i++){

                if(clients[i] && (clients[i] != client)){
                    clients[i].write(thisChar);
                }
            }
            // echo the bytes to the server as well:
            Serial.write(thisChar);
        }
    }

    for(byte i = 0; i < 4; i++){

        if(clients[i] && !clients[i].connected()){

            if(clients[i] && !(clients[i].connected())){
                // client.stop() invalidates the internal socket-descriptor, so next use of == will allways return false;
                Serial.print("\nclient number: ");
                Serial.print(i);
                Serial.println(" disconnected.");
                clients[i].stop();
            }
            // client.stop() invalidates the internal socket-descriptor, so next use of == will allways return false;
            clients[i].stop();
        }
    }
}
