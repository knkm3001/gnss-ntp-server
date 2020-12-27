#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <WiFiUDP.h>
#include <time.h>
#include <Arduino.h>
#include "wifiConfig.h"

#define JST 3600*9

static WiFiUDP wifiUdp;
IPAddress ip(192, 168, 0, 128);
IPAddress subnet(255, 255, 255, 0);
IPAddress gateway(192,168, 0, 1);
IPAddress DNS(192, 168, 0, 1);

char packetBuffer[127];
int8_t payloadBuffer[127];

static const int remoteUdpPort = 9000;

static const long NtpUnixTimeDelta = 2208988800;
time_t referenceTimestamp;

SoftwareSerial mySerial(5, 4); // RX, TX
 
void setup() {
  Serial.begin(57600);
  while (!Serial) {
  ;
  }

  Serial.println("");
  Serial.print("Connecting to ");
  Serial.print(ssid);
  WiFi.config(ip, gateway, subnet, DNS);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }
  Serial.println("");
  
  Serial.println("WiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("");

  // set the data rate for the SoftwareSerial port
  mySerial.begin(9600);


  configTzTime("JST-9", "133.243.238.244","ntp.nict.jp", "ntp.jst.mfeed.ad.jp");
  
  Serial.print("NTP sync ");
  for(int i = 0; i < 10; i++) {
      delay(500);
      Serial.print(".");
  }
  Serial.println("");

  referenceTimestamp = time(NULL);
  Serial.print("referenceTimestamp: ");
  Serial.println((long)referenceTimestamp);
  

  struct tm *tm = localtime(&referenceTimestamp);
  String jstDateTime = ""; 
  jstDateTime += String(tm->tm_year+1900)+"/";
  jstDateTime += (String(tm->tm_mon+1).length() ==1 ? "0"+String(tm->tm_mon+1) : String(tm->tm_mon+1))+"/";
  jstDateTime += (String(tm->tm_mday).length()  ==1 ? "0"+String(tm->tm_mday)  : String(tm->tm_mday)) +"T";
  jstDateTime += (String(tm->tm_hour).length()  ==1 ? "0"+String(tm->tm_hour)  : String(tm->tm_hour)) +":";
  jstDateTime += (String(tm->tm_min).length()   ==1 ? "0"+String(tm->tm_min)   : String(tm->tm_min))  +":";
  jstDateTime += (String(tm->tm_sec).length()   ==1 ? "0"+String(tm->tm_sec)   : String(tm->tm_sec));
  Serial.print(jstDateTime);
  Serial.println(" JST");
  
  Serial.println("");

  static const int localPort = 123; //ntp port no.
  wifiUdp.begin(localPort);

  }

void loop() {
  int packetSize = wifiUdp.parsePacket();
  if (packetSize==0) {
    delay(100);
    return; 
  }

  // リクエストデータの表示
  Serial.println("packet received!");
  time_t receivedTimestamp = time(NULL);
  Serial.print("receivedTimestamp: ");
  Serial.println((long)receivedTimestamp);

  IPAddress remoteUdpIp = wifiUdp.remoteIP();
  Serial.print("ip from: ");
  Serial.println(remoteUdpIp);
  Serial.print("port from: ");
  Serial.println(wifiUdp.remotePort());
  Serial.print("packetSize[byte]: ");
  Serial.println(packetSize);
  memset(packetBuffer, 0, sizeof(packetBuffer));
  int bufferSize = wifiUdp.read(packetBuffer, 255);
  Serial.print("bufferSize: ");
  Serial.println(bufferSize);

  // パケットのUDPデータ部分を表示
  Serial.print("packet payload: ");
  for(int i=0;i<bufferSize;i++){
    uint8_t data = packetBuffer[i];
    Serial.print(data, HEX);
    Serial.print(" ");
  }
  Serial.println("");

  memset(payloadBuffer, 0, sizeof(payloadBuffer));


  // 0th byte (LI, VN, Mode)
  uint8_t packetFirstByte = packetBuffer[0];
  int vn = (packetFirstByte >> 3) & 7; // Version Number の取得(3~4bit)
  int li = 0;   // 0で固定 (0~1bit)
  int mode = 4; // 4で固定 (5~7bit)
  payloadBuffer[0] = uint8_t(li+(vn<<3)+mode);

  // 1st byte (Stratum)
  payloadBuffer[1] = uint8_t(1);

  // 2nd byte (Polling)
  payloadBuffer[2] = uint8_t(6);

  // 3rd byte (Precision)
  payloadBuffer[2] = int8_t(-12);

  // 4th~7th byte (Root Delay)
  for (int i = 0; i < 4; i++){
    payloadBuffer[4+i] = uint8_t(0);
  }

  // 8th~11th byte (Root Dispersion)
  for (int i = 0; i < 4; i++){
    payloadBuffer[8+i] = uint8_t(0);
  }
  
  // 12th~15th byte (Reference ID)
  char ReferenceID[] = "GNSS";
  Serial.print("ReferenceID: ");
  for(int i = 0; i < 4; i++){
    Serial.print(uint8_t(ReferenceID[i]));
    payloadBuffer[12+i] = uint8_t(ReferenceID[i]);
    Serial.print(" ");
  }
  Serial.println("");

  // 16th~23th byte (Reference Timestamp)
  Serial.print("referenceTimestamp: ");
  Serial.println((long)referenceTimestamp);
  Serial.print("referenceTimestamp(NTP time): ");
  Serial.println((long)referenceTimestamp- NtpUnixTimeDelta);
  payloadBuffer[16] = uint8_t((long)referenceTimestamp - NtpUnixTimeDelta);
  
  // 24th~31th byte (Origin Timestamp) NTP リクエストのTransmit Timestamp
  for (int i = 0; i < 7; i++){
    payloadBuffer[24+i] = packetBuffer[40+i];
  }

  // 32th~39th byte (Receive Timestamp)
  Serial.print("receivedTimestamp: ");
  Serial.println(uint8_t((long)receivedTimestamp - NtpUnixTimeDelta));
  payloadBuffer[32] = uint8_t((long)receivedTimestamp - NtpUnixTimeDelta);

  // 40th~48th byte (Transmit Timestamp)
  for (int i = 0; i < 4; i++){
    payloadBuffer[8+i] = uint8_t(0);
  }
  
  Serial.println("");

  // 送信データの処理
  wifiUdp.beginPacket(remoteUdpIp, remoteUdpPort);
  while(1){
    if (mySerial.available()) {
      String serialStr = mySerial.readStringUntil('\n');
      Serial.println(serialStr);
      //wifiUdp.print(serialStr);
      if(serialStr.substring(3,6)=="ZDA"){
        break;
      }
    }
  }

  int payloadSize = sizeof(payloadBuffer);
  for(int i=0;i<payloadSize;i++){
    wifiUdp.write(payloadBuffer[i]);
    Serial.print(payloadBuffer[i], HEX);
  }
  wifiUdp.write("from ESP8266\r\n");
  wifiUdp.endPacket();
}