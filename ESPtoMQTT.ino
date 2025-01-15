#include<WiFi.h> //ใช้ในการเชื่อมต่อ WiFi
#include<PubSubClient.h> //ใช้ในการสื่อสารกับโปโตคอล mqtt

#define WiFi_SSID "Your WiFi ssid" //ssid หรือชื่อของ WiFi ที่ต้องการเชื่อมต่อ
#define WiFi_PASSWORD "Your password" //password หรือรหัสของ WiFi ที่ต้องการเชื่อมต่อ

const char *mqtt_broker = "broker.emqx.io"; //ชื่อ Broker ที่ต้องการเชื่อมต่อ
const int mqtt_Port = 1883; //port ของโปโตคอล mqtt

const char *topic1 = "Tdata1"; //ชื่อ topic ของข้อมูลที่ต้องการส่งที่1
const char *topic2 = "Tdata2"; //ชื่อ topic ของข้อมูลที่ต้องการส่งที่2

WiFiClient espClient; //ทำการสร้าง Client ของ WiFi
PubSubClient client(espClient); //ทำการสร้าง Client ของ mqtt 

void setup() {
Serial.begin(115200);
client.setServer(mqtt_broker, mqtt_Port); //ทำการเชื่อต่อกับ broker
WiFi.begin(WiFi_SSID,WiFi_PASSWORD);} //ทำการเชื่อมต่อกับ WiFi

//สร้างฟังก์ชันในการตรวจเช็คการเชื่อมต่อ WiFi
void checkWiFi(){
while(WiFi.status() != WL_CONNECTED)//ทำการ วนลูปเมื่อเชื่อมต่อ WiFi ไม่ได้
  {
delay(1000);
Serial.print(".");}}

//สร้างฟังก์ชันในการตรวจเช็คการเชื่อมต่อ mqtt
void check_mqtt() {
  //สร้างเงื่อนไขเมื่อเชื่อมต่อ mqtt ไม่ได้
  if (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("npru_test_mqtt1")) //ทำการเชื่อมต่อ mqtt โดยที่ในวงเล็บคือชื่อที่สามารถตั้งได้ตามต้องการโดยที่ชื่อที่ตั้งไม่ควรซ้ำกันในจำนวนมาก
    {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state()); //เมื่อไม่สามารถเชื่อมต่อ mqtt ได้จะทำการแสดง state เพื่อบงบอกสาเหตุที่ไม่สามารถเชื่อมต่อ mqtt ได้
      delay(5000);
   }}}

void loop() {
  for(int i=0;i<=10;i++) //ตัวอย่างการสร้างข้อมูลเพื่อทำการส่งไปยังดัชบอร์ด
  {
    checkWiFi();
    check_mqtt();
    client.publish(topic1,String(i).c_str()); //ทำการส่งข้อมูลที่1 ไปยัง broker 
    client.publish(topic2,String(10-i).c_str()); //ทำการส่งข้อมูลที่2  ไปยัง broker 
    delay(2000);
  }}
