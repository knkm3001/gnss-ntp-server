#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <WiFiUDP.h>

#include "wifiConfig.h"


static WiFiUDP wifiUdp;
static const char *remoteIpadr = "192.168.0.255";
static const int remoteUdpPort = 9000;
IPAddress ip(192, 168, 0, 128);
IPAddress subnet(255, 255, 255, 0);
IPAddress gateway(192,168, 0, 1);
IPAddress DNS(192, 168, 0, 1);



SoftwareSerial mySerial(5, 4); // RX, TX
 
void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(57600);
  while (!Serial) {
  ; // wait for serial port to connect. Needed for native USB port only
  }

  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.config(ip, gateway, subnet, DNS);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }

  static const int localPort = 7000;
  wifiUdp.begin(localPort);
  
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("");

 
 // set the data rate for the SoftwareSerial port
 mySerial.begin(9600);
 mySerial.println("Hello, world?");
}
 
void loop() { // run over and over

  if (wifiUdp.parsePacket() == 0) {
    delay(100);
    return; 
  }

  Serial.println("packet received!");
  Serial.println(wifiUdp.read());

  wifiUdp.beginPacket(remoteIpadr, remoteUdpPort);
  while(1){
    if (mySerial.available()) {
      String serialStr = mySerial.readStringUntil('\n');
      Serial.println(serialStr);
      wifiUdp.print(serialStr);
      if(serialStr.substring(3,6)=="ZDA"){
        wifiUdp.write("from ESP8266\r\n");
        Serial.println("--------");
        break;
      }
    }
  }
  wifiUdp.endPacket();
}