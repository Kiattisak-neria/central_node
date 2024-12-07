#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <PubSubClient.h>
#include<NTPClient.h>
#include <Adafruit_SHT31.h>

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 7*3600;
const int   daylightOffset_sec = 0;
/*#define ssid "............"
#define password "kkkkoooo"*/

#define ssid "TP-Link_FA5C"
#define password "47845862"

const char* mqtt_server = "broker.emqx.io";
WiFiClient espClient;
PubSubClient client(espClient);

const int moistureThreshold = 30;

float Rtemp,Rhum,temperature,humidity;
String status_auto="อัตโนมัติ",status_man="แมนนวล";
Adafruit_SHT31 sht31 = Adafruit_SHT31();
String tt;
int ma[6],mi[6],setma[6],setmi[6],avgsoil[4],sumsoil[4],ccso,soilif[12],moistureValue,h,m,s,day,year,mon,y,mm,days,wday,dst;
int relay[4]={25,4,12,13};
int soilauto,soilon[4],StageRelay=0,StageAuto=0;
int oon1,oon2,nodeLED=0,strat_to_man=0;
String H,M,S,SW;
int soilH[4],soilM[4],soilS[4],Hi[4],Mi[4],HH[4];
unsigned long Time[4],Time2[4],intervel[4];
unsigned long Time3;
String topic_off = "soiloff";

void setup() {
  Serial.begin(115200);
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  sht31.begin(0x44);
  for(int ii=0;ii<=4;ii++){
    pinMode(relay[ii],OUTPUT);
  }
  connectWiFi();
  setupMQTT();

}

void processMessage(String message) {
  int delimiterIndex = message.indexOf(':');
  
  if (delimiterIndex != -1) {
    String nodeIdStr = message.substring(0, delimiterIndex);
    String moistureValueStr = message.substring(delimiterIndex + 1);
    
    int moistureValue = moistureValueStr.toInt();

    if (tt == "soilH") {
      for(int ii=1;ii<=4;ii++){
         H="H"+String(ii);
        if(nodeIdStr==H){
          soilH[ii]=moistureValue;
          HH[ii]=moistureValue;
      }}}
    if (tt == "soilM") {
      for(int ii=1;ii<=4;ii++){
        M="M"+String(ii);
        if(nodeIdStr==M){
          soilM[ii]=moistureValue;
      }}}
    if (tt == "soilS") {
      for(int ii=1;ii<=4;ii++){
        S="S"+String(ii);
        if(nodeIdStr==S){
          soilS[ii]=moistureValue;
      }}}
    if (tt == "soilon") {
      for(int ii=1;ii<=3;ii++){
        SW="SW"+String(ii);
        if(nodeIdStr==SW){
          Serial.println(SW);
          unsigned long currentMills = millis();
          Time[ii]=currentMills;
          soilon[ii]=moistureValue;
          if(ii==3){oon1=moistureValue;}
        }}
        }
    if (tt == "soilon2") {
      if(nodeIdStr=="SW4"){
        unsigned long currentMills = millis();
        Time[4]=currentMills;
        oon2=moistureValue;
      }
    }
    Serial.print("Node ID: ");
    Serial.print(nodeIdStr);
    Serial.print(", Moisture Value: ");
    Serial.println(moistureValue);}}

void timeNTP(){
  time_t now = time(nullptr);
  struct tm*p_tm=localtime(&now);

  if(!getLocalTime(p_tm)){
    Serial.println("Failed to obtain time");
    time_t now = time(nullptr);
    struct tm*p_tm=localtime(&now);
    return;
  }

  h=p_tm->tm_hour;
  m=p_tm->tm_min;
  s=p_tm->tm_sec;
  y=p_tm->tm_year;
  wday=p_tm->tm_wday;
  mm=p_tm->tm_mon;
  day=p_tm->tm_mday;
  year=y+1900+543;
  mon=mm+1;
  Serial.print("time ");
  Serial.print(h);
  Serial.print(":");
  Serial.print(m);
  Serial.print(":");
  Serial.println(s);
  Serial.print("deat ");
  Serial.print(day);
  Serial.print("/");
  Serial.print(mon);
  Serial.print("/");
  Serial.println(year);
  Serial.print("week day ");
  Serial.println(wday);
  Serial.println("");
}

void SHT_31(){
  unsigned long currentMills = millis();
  temperature = sht31.readTemperature();
  humidity = sht31.readHumidity();
  if(currentMills-Time3>=3000){
    Time3=currentMills;
  client.publish("temp",String(temperature).c_str());
  client.publish("hum",String(humidity).c_str());}
}

void loop() {
  unsigned long currentMills = millis();
  //timeNTP();
  SHT_31();
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();
  if(strat_to_man==0){client.publish("status123",status_man.c_str());
  strat_to_man=1;}
  if(soilauto==1){onRelay();}
  else{man();}
  LLED();
  }

void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  String string;
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] = ");
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  tt=topic;
  moistureValue = message.toInt();
  Serial.println(moistureValue);
  processMessage(message);
  for(int ii=1;ii<=6;ii++){
    String mma="setmax"+String(ii);
  if (tt == mma){
    ma[ii]=moistureValue;
    Serial.print(mma);
    Serial.print(" = ");
    Serial.println(ma[ii]);
  }}

  for(int ii=1;ii<=6;ii++){
    String mma="setlow"+String(ii);
  if (tt == mma){
    mi[ii]=moistureValue;
    Serial.print(mma);
    Serial.print(" = ");
    Serial.println(mi[ii]);
  }}
  
  if(tt == "set"){
        setma[1]=ma[1];
        setmi[1]=mi[1];
    }
  if(tt == "set2"){
        setma[2]=ma[2];
        setmi[2]=mi[2];
    }
  for(int ii=1;ii<=12;ii++){
    String ddd = "data"+String(ii);
    if (tt == ddd) {soilif[ii]=moistureValue;
  }
  if (tt == "soilauto") {soilauto=moistureValue;
  if(soilauto==1){
    client.publish("status123",status_auto.c_str());}
  else{client.publish("status123",status_man.c_str());
  }} 
}}


void connectWiFi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
}

void setupMQTT() {
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("connect mqtt....");
    Serial.println(".");
    if (client.connect("nnnppprruuu")) {
      Serial.println("connect");
      client.subscribe("data1");
      client.subscribe("data2");
      client.subscribe("data3");
      client.subscribe("data4");
      client.subscribe("setmax1");
      client.subscribe("setlow1");
      client.subscribe("setmax2");
      client.subscribe("setlow2");
      client.subscribe("setmax3");
      client.subscribe("setlow3");
      client.subscribe("setmax4");
      client.subscribe("setlow4");
      client.subscribe("set");
      client.subscribe("set2");
      client.subscribe("soilauto");
      client.subscribe("soilon");
      client.subscribe("soilon2");
      client.subscribe("soilH");
      client.subscribe("soilM");
      client.subscribe("soilS");
    } else {
      Serial.print("MQTT connection failed, rc=");
      Serial.println(client.state());
      Serial.println("Trying again in 5 seconds...");
      delay(5000);
    }
  }
}

void onRelay(){
  ccso=0;
  StageRelay=0;
  if(StageAuto==0){
    digitalWrite(relay[0], LOW);
    digitalWrite(relay[1], LOW);
    digitalWrite(relay[2], LOW);
    digitalWrite(relay[3], LOW);
  }
    unsigned long currentMills = millis();
        sumsoil[1]=soilif[1]+soilif[2];
        avgsoil[1]=sumsoil[1]/2;
        sumsoil[2]=soilif[3]+soilif[4];
        avgsoil[2]=sumsoil[2]/2;
  for(int ii=1;ii<=2;ii++){
  if(setma[ii]!=0 && setmi[ii]!=0){
    if(StageAuto==0){
      if(avgsoil[ii] <=setmi[ii]){StageAuto=ii;
          Time2[ii]=currentMills;}}
    if(StageAuto==ii){
        if(currentMills-Time2[ii]>=3000){digitalWrite(relay[ii-1], HIGH);
          }
        if(avgsoil[ii] >= setma[ii]){digitalWrite(relay[ii-1], LOW);
          StageAuto=0;}
      }}}
      }

void  man(){
    unsigned long currentMills = millis();
    Time2[1]=currentMills;
    Time2[2]=currentMills;
    if(StageRelay==0){
      digitalWrite(relay[0], LOW);
      digitalWrite(relay[1], LOW);
      digitalWrite(relay[2], LOW);
      digitalWrite(relay[3], LOW);
    }
    for(int ii=1;ii<=4;ii++){
    Hi[ii]=soilH[ii]*3600;
    Mi[ii]=soilM[ii]*60;
    intervel[ii] = (Hi[ii]+Mi[ii]+soilS[ii])*1000;}

    if (soilon[1]==1){
      String top=topic_off+String(1);
      const char *tpic=top.c_str();
      if(currentMills-Time[1]>=intervel[1]+3000){
        soilon[1]=0;
        client.publish(tpic,String(0).c_str());
        Time[1]=currentMills;
        digitalWrite(relay[0], LOW);
        
        if(soilon[2]==1){StageRelay=2;
           Time[2]=currentMills;}
        else{StageRelay=0;}
      } 
      else{
        if(StageRelay==0){
        StageRelay=1;
        Time[1]=currentMills;}
        if(StageRelay==1){
          if(currentMills-Time[1]>=3000){digitalWrite(relay[0], HIGH);
          }}
        if(StageRelay!=0 && StageRelay!=1){Time[1]=currentMills;}
    }}
    if (soilon[1]==0){digitalWrite(relay[0], LOW);
        if(StageRelay==1){StageRelay=0;}
        else{StageRelay=StageRelay;}
        }
    
    if (soilon[2]==1){
      String top=topic_off+String(2);
      const char *tpic=top.c_str();
      if(currentMills-Time[2]>=intervel[2]+3000){
        soilon[2]=0;
        client.publish(tpic,String(0).c_str());
        Time[2]=currentMills;
        digitalWrite(relay[1], LOW);
        if(oon1==1){StageRelay=3;
           Time[3]=currentMills;}
        else{StageRelay=0;}
      } 
      else{
        if(StageRelay==0){
        StageRelay=2;
        Time[2]=currentMills;}
        if(StageRelay==2){
          if(currentMills-Time[2]>=3000){digitalWrite(relay[1], HIGH);
          }}
        if(StageRelay!=0 && StageRelay!=2){Time[2]=currentMills;}
    }}
    if (soilon[2]==0){digitalWrite(relay[1], LOW);
        if(StageRelay==2){StageRelay=0;}
        else{StageRelay=StageRelay;}
        }

    if(oon1==1){
      String top=topic_off+String(3);
      const char *tpic=top.c_str();
      if(currentMills-Time[3]>=intervel[3]+3000){
        oon1=0;
        client.publish(tpic,String(0).c_str());
        Time[3]=currentMills;
        digitalWrite(relay[2], LOW);
        if(oon2==1){StageRelay=4;
           Time[4]=currentMills;}
        else{StageRelay=0;}
      } 
      else{
        if(StageRelay==0){
        StageRelay=3;
        Time[3]=currentMills;}
        if(StageRelay==3){
          if(currentMills-Time[3]>=3000){digitalWrite(relay[2], HIGH);
          }}
        if(StageRelay!=0 && StageRelay!=3){Time[3]=currentMills;}
    }}
    if (oon1==0){digitalWrite(relay[2], LOW);
        if(StageRelay==3){StageRelay=0;}
        else{StageRelay=StageRelay;}
        }

    if(oon2==1){
      String top=topic_off+String(4);
      const char *tpic=top.c_str();
      if(currentMills-Time[4]>=intervel[4]+3000){
        oon2=0;
        client.publish(tpic,String(0).c_str());
        Time[4]=currentMills;
        digitalWrite(relay[3], LOW);
        if(soilon[1]==1){StageRelay=1;
           Time[1]=currentMills;}
        else{StageRelay=0;}
      } 
      else{
        if(StageRelay==0){
        StageRelay=4;
        Time[4]=currentMills;}
        if(StageRelay==4){
          if(currentMills-Time[4]>=3000){digitalWrite(relay[3], HIGH);
          }}
        if(StageRelay!=0 && StageRelay!=4){Time[4]=currentMills;}
    }}
    if (oon2==0){digitalWrite(relay[3], LOW);
        if(StageRelay==4){StageRelay=0;}
        else{StageRelay=StageRelay;}
        }}
    
    void LLED(){
      String LED_N0 = "LED01";
      String LED_N1 = "LED02";
      
      int rree1 = digitalRead(relay[0]);
      if(rree1==1 && nodeLED==0){nodeLED=1;
      client.publish(LED_N0.c_str(),String(1).c_str());
      }
      if(rree1==0 && nodeLED==1){nodeLED=0;
      client.publish(LED_N0.c_str(),String(0).c_str());
      }

      int rree2 = digitalRead(relay[1]);
      if(rree2==1 && nodeLED==0){nodeLED=2;
      client.publish(LED_N1.c_str(),String(1).c_str());
      }
      if(rree2==0 && nodeLED==2){nodeLED=0;
      client.publish(LED_N1.c_str(),String(0).c_str());
      }
      //Serial.println("["+String(rree1)+","+String(nodeLED)+"]["+String(rree2)+","+String(nodeLED)+"]");
    }



