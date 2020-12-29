#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <WiFiUDP.h>
#include <time.h>
#include <Arduino.h>
#include <string>
#include "wifiConfig.h"

#define JST 3600*9

static WiFiUDP wifiUdp;
IPAddress ip(192, 168, 0, 128);
IPAddress subnet(255, 255, 255, 0);
IPAddress gateway(192,168, 0, 1);
IPAddress DNS(192, 168, 0, 1);

SoftwareSerial mySerial(5, 4); // RX, TX

static const int NtpPort = 123;
static const double NtpUnixTimeDelta = 2208988800.0; // NTP Time と Unix Time の差(70年)

time_t referenceTimestamp;
char reqPacketBuffer[48];
int8_t resPacketBuffer[48];

double calcUnixTime();
void createResPacket(double);
 
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


  configTzTime("JST-9", "133.243.238.244", "210.173.160.27");
  // DNSが機能しない？
  // 133.243.238.244: ntp.nict.jp
  // 210.173.160.27 : ntp1.jst.mfeed.ad.jp
  // 216.239.35.0   : time1.google.com.ntp
  
  Serial.print("NTP sync ");
  for(int i = 0; i < 10; i++) {
      delay(100);
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

  wifiUdp.begin(NtpPort);

  }



void loop() {
  int reqPacketSize = wifiUdp.parsePacket();
  if (reqPacketSize==0) {
    delay(10);
    return; 
  }

  // リクエストデータの表示
  Serial.println("reqPacket received!");
  double receivedTimeStamp = time(NULL);
  Serial.print("receivedTimestamp: ");
  Serial.println(receivedTimeStamp);

  IPAddress remoteUdpIp = wifiUdp.remoteIP();
  Serial.print("ip from: ");
  Serial.println(remoteUdpIp);
  Serial.print("port from: ");
  Serial.println(wifiUdp.remotePort());
  Serial.print("reqPacketSize[byte]: ");
  Serial.println(reqPacketSize);
  memset(reqPacketBuffer, 0, sizeof(reqPacketBuffer));
  int bufferSize = wifiUdp.read(reqPacketBuffer, 255);
  Serial.print("bufferSize: ");
  Serial.println(bufferSize);

  // パケットのUDPデータ部分を表示
  Serial.print("reqPacket resPacket: ");
  for(int i=0;i<bufferSize;i++){
    uint8_t data = reqPacketBuffer[i];
    Serial.print(data, HEX);
    Serial.print(" ");
  }
  Serial.println("");

  createResPacket(receivedTimeStamp);

  // 送信データの処理
  wifiUdp.beginPacket(remoteUdpIp, wifiUdp.remotePort());

  Serial.print("resPacket: ");
  int resPacketSize = sizeof(resPacketBuffer);
  for(int i=0;i<resPacketSize;i++){
    wifiUdp.write(resPacketBuffer[i]);
    Serial.print(resPacketBuffer[i], HEX);
    Serial.print(" ");
  }
  Serial.println("");

  //wifiUdp.write("from ESP8266\r\n");
  wifiUdp.endPacket();

  Serial.println("finished!");
  Serial.println("");

}



void createResPacket(double receivedTimeStamp){
  memset(resPacketBuffer, 0, sizeof(resPacketBuffer));

  // 0th byte (LI, VN, Mode)
  uint8_t reqPacketFirstByte = reqPacketBuffer[0];
  int vn = (reqPacketFirstByte >> 3) & B0000111; // Version Number の取得(3~4bit)
  int li = 0;   // 0で固定 (0~1bit)
  int mode = 4; // 4で固定 (5~7bit)
  resPacketBuffer[0] = uint8_t(li+(vn<<3)+mode);

  // 1st byte (Stratum)
  resPacketBuffer[1] = uint8_t(1);

  // 2nd byte (Polling)
  resPacketBuffer[2] = uint8_t(6);

  // 3rd byte (Precision)
  resPacketBuffer[3] = int8_t(-12);

  // 4th~7th byte (Root Delay)
  for (int i = 0; i < 4; i++){
    resPacketBuffer[4+i] = uint8_t(0);
  }

  // 8th~11th byte (Root Dispersion)
  for (int i = 0; i < 4; i++){
    resPacketBuffer[8+i] = uint8_t(0);
  }
  
  // 12th~15th byte (Reference ID)
  char ReferenceID[] = "GNSS";
  memcpy(resPacketBuffer+12,ReferenceID,4);

  // 16th~23th byte (Reference Timestamp)
  Serial.print("referenceTimestamp: ");
  Serial.println(referenceTimestamp);

  double referenceTimestampNTP = referenceTimestamp + NtpUnixTimeDelta;
  Serial.print("referenceTimestamp(NTP time): ");
  Serial.println(referenceTimestampNTP); // unix time to ntp time
  uint32_t referenceTimestampIntPart = round(referenceTimestampNTP);
  referenceTimestampIntPart = htonl(referenceTimestampIntPart); // リトルエンディアンだからビット順反転
  // ここ便宜的(小数部はデタラメ)
  String referenceTimestampFlacPartString = String(referenceTimestampNTP - (int)referenceTimestampNTP);
  uint32_t referenceTimestampFlacPart = uint32_t(referenceTimestampFlacPartString.substring(2).toInt());
  referenceTimestampFlacPart = htonl(referenceTimestampFlacPart); // リトルエンディアンだからビット順反転

  memcpy(resPacketBuffer+16, &referenceTimestampIntPart, sizeof(referenceTimestampIntPart));
  memcpy(resPacketBuffer+20, &referenceTimestampFlacPart, sizeof(referenceTimestampFlacPart));

  // 24th~31th byte (Origin Timestamp) NTP リクエストのTransmit Timestamp
  // リクエストパケットの40~47バイトをresPacketの24~31バイトにコピー
  memcpy(resPacketBuffer+24,reqPacketBuffer+40,8);

  // 32th~39th byte (Receive Timestamp)
  double receivedTimeStampNTP = receivedTimeStamp + NtpUnixTimeDelta;
  uint32_t receivedTimeStampNTPIntPart = round(receivedTimeStampNTP);
  receivedTimeStampNTPIntPart = htonl(receivedTimeStampNTPIntPart); // リトルエンディアンだからビット順反転
  // ここ便宜的(小数部はデタラメ)
  String receivedTimeStampNTPFlacPartString = String(receivedTimeStampNTP - (int)receivedTimeStampNTP);
  uint32_t receivedTimeStampNTPFlacPart = uint32_t(receivedTimeStampNTPFlacPartString.substring(2).toInt());
  receivedTimeStampNTPFlacPart = htonl(receivedTimeStampNTPFlacPart); // リトルエンディアンだからビット順反転

  memcpy(resPacketBuffer+32, &receivedTimeStampNTPIntPart, sizeof(receivedTimeStampNTPIntPart));
  memcpy(resPacketBuffer+38, &receivedTimeStampNTPFlacPart, sizeof(receivedTimeStampNTPFlacPart));

  // 40th~48th byte (Transmit Timestamp)
  double unixTime = calcUnixTime();
  Serial.print("Unixtime(UTC): ");
  Serial.println(unixTime);
  double unixTimeNTP = unixTime + NtpUnixTimeDelta;
  uint32_t unixTimeNTPIntPart = round(unixTimeNTP);
  unixTimeNTPIntPart = htonl(unixTimeNTPIntPart); // リトルエンディアンだからビット順反転
  // ここ便宜的(小数部はデタラメ)
  String unixTimeNTPFlacPartString = String(unixTimeNTP - (int)unixTimeNTP);
  uint32_t unixTimeNTPFlacPart = uint32_t(unixTimeNTPFlacPartString.substring(2).toInt());
  unixTimeNTPFlacPart = htonl(unixTimeNTPFlacPart); // リトルエンディアンだからビット順反転

  memcpy(resPacketBuffer+40, &unixTimeNTPIntPart, sizeof(unixTimeNTPIntPart));
  memcpy(resPacketBuffer+44, &unixTimeNTPFlacPart, sizeof(unixTimeNTPFlacPart));
}

double calcUnixTime(){
  // TODO PPSでUnixTIMEを取得できるまでの便宜的措置
  double Unixtime;
  while(1){
    if (mySerial.available()) {
      String serialStr = mySerial.readStringUntil('\n');
      if(serialStr.substring(0, 6) == "$GPRMC"){
        Serial.print("NMEA Code: ");
        Serial.println(serialStr);
        int timePosStart = serialStr.indexOf(",")+1;
        int timePosEnd = serialStr.indexOf(",",timePosStart);
        String UTCTime = serialStr.substring(timePosStart, timePosEnd+1);
        Serial.print("UTCTime: ");
        Serial.println(UTCTime);
        double hours = UTCTime.substring(0, 2).toFloat();
        double minutes = UTCTime.substring(2, 4).toFloat();
        double seconds = UTCTime.substring(4).toFloat();
        double TodaysUTCTimeSec = hours*3600+minutes*60+seconds;
        
        int dateStartPos = 0;
        for(int i=0;i<9;i++){
          dateStartPos = serialStr.indexOf(",",dateStartPos)+1;
        }
        int dateEndPos = serialStr.indexOf(",",dateStartPos);
        String UTCDate = serialStr.substring(dateStartPos, dateEndPos);
        Serial.print("UTCDate: ");
        Serial.println(UTCDate);
        int day = UTCDate.substring(0, 2).toInt();
        int month = UTCDate.substring(2, 4).toInt();
        int year = UTCDate.substring(4).toInt()+2000;
        Serial.print("day: ");
        Serial.print(day);
        Serial.print(" month: ");
        Serial.print(month);
        Serial.print(" year: ");
        Serial.print(year);

        Serial.print(" h: ");
        Serial.print(hours);
        Serial.print(" m: ");
        Serial.print(minutes);
        Serial.print(" s: ");
        Serial.println(seconds);
        Serial.print("TodaysUTCTime(sec): ");
        Serial.println(TodaysUTCTimeSec);

        struct tm tm = {
          0, 0, 0, // sec,min,hour
          day,
          month-1,
          year-1900,
        };
        double unixDate = mktime(&tm);

        Serial.print("Unix time untile today midnight: ");
        Serial.println(unixDate);

        Unixtime = unixDate + TodaysUTCTimeSec + 3600*9;

        break;
      }
    }
  }
  return Unixtime;
}
