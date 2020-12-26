#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <WiFiUDP.h>

#include "wifiConfig.h"


static WiFiUDP wifiUdp;
IPAddress ip(192, 168, 0, 128);
IPAddress subnet(255, 255, 255, 0);
IPAddress gateway(192,168, 0, 1);
IPAddress DNS(192, 168, 0, 1);

char packetBuffer[255];
static const int remoteUdpPort = 9000;

SoftwareSerial mySerial(5, 4); // RX, TX
 
void setup() {
  Serial.begin(57600);
  while (!Serial) {
  ;
  }

  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.config(ip, gateway, subnet, DNS);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }

  static const int localPort = 123; //ntp port no.
  wifiUdp.begin(localPort);
  
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("");

 // set the data rate for the SoftwareSerial port
 mySerial.begin(9600);

}
 
void loop() {
  int packetSize = wifiUdp.parsePacket();
  if (packetSize==0) {
    delay(100);
    return; 
  }

  // リクエストデータの処理
  Serial.println("packet received!");
  IPAddress remoteUdpIp = wifiUdp.remoteIP();
  Serial.println(remoteUdpIp);
  Serial.println(wifiUdp.remotePort());
  int len = wifiUdp.read(packetBuffer, packetSize);
  if (len > 0) packetBuffer[len] = '\0';
  Serial.println(packetBuffer);

  // 送信データの処理
  wifiUdp.beginPacket(remoteUdpIp, remoteUdpPort);
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