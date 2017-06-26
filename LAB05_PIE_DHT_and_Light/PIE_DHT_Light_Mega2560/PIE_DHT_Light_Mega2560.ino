#include <Ethernet.h>
#include <MicroGear.h>
#include "DHT.h"              // library สำหรับอ่านค่า DHT Sensor

#define APPID   "YOUR_APPID"                              // ให้แทนที่ด้วย AppID
#define KEY     "YOUR_KEY"                                // ให้แทนที่ด้วย Key
#define SECRET  "YOUR_SECRET"                             // ให้แทนที่ด้วย Secret
#define ALIAS   "mega2560"                                // ชื่ออุปกรณ์

#define neighbor "nodewifi"                               // ชื่ออุปกรณ์ที่ต้องการส่งข้อความไปให้
#define topicPublish "/dht"                               // topic ที่ต้องการ publish ส่งข้อความ
#define topicRelayPublish "/relay"                        // topic ที่ต้องการ publish ส่งข้อความ

#define LEDPIN 13        // GPIO13 ขาที่ต่อเข้ากับขา S ของ LED
#define LDRPIN A3        // Analog3 ขาที่ต่อเข้ากับขา S ของ Photocell Sensor
#define DHTPIN 5         // GPIO5 ขาที่ต่อเข้ากับขา S ของ DHT Sensor
#define DHTTYPE DHT11     // e.g. DHT11, DHT21, DHT22
DHT dht(DHTPIN, DHTTYPE);

unsigned long lastTimeDHT = 0;      // เก็บเวลา timestamp ที่อ่าน DHT Sensor ล่าสุด
float humid = 0;                    // เก็บค่าความชื้นที่อ่านล่าสุด
float temp = 0;                     // เก็บค่าอุณหภูมิที่อ่านล่าสุด
int light = 0;                      // เก็บค่าแสงสว่าง
bool sendMsgLight = false;          // เก็บค่าสถานะการส่งข้อมูลเปิดปิด LED
bool sendMsgRelay = false;          // เก็บค่าสถานะการส่งข้อมูลเปิดปิด Relay

EthernetClient client;

int timer = 0;
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
MicroGear microgear(client);

/* If a new message arrives, do this */
void onMsghandler(char *topic, uint8_t* msg, unsigned int msglen) {
    Serial.print("Incoming message --> ");
    msg[msglen] = '\0';
    Serial.println((char *)msg);

    // สถานะ LED บน NodeMCU ที่แสดงผล จะติดก็ต่อเมื่อสั่ง LOW 
    // แต่ถ้าเป็น LED ที่ต่อแยก จะต้องสั่งเป็น HIGH 
    if(*(char *)msg == '1'){
        digitalWrite(LEDPIN, HIGH); // LED on
    }else{
        digitalWrite(LEDPIN, LOW); // LED off
    }
}

/* When a microgear is connected, do this */
void onConnected(char *attribute, uint8_t* msg, unsigned int msglen) {
    Serial.println("Connected to NETPIE...");
    /* Set the alias of this microgear ALIAS */
    microgear.setAlias(ALIAS);
}


void setup() {
    /* Add Event listeners */

    /* Call onMsghandler() when new message arraives */
    microgear.on(MESSAGE,onMsghandler);

    /* Call onConnected() when NETPIE connection is established */
    microgear.on(CONNECTED,onConnected);
    
    Serial.begin(9600);
    Serial.println("Starting...");
    
    dht.begin();                      // setup ตัวแปรสำหรับอ่านค่า DHT Sensor
    pinMode(LEDPIN, OUTPUT);          // LED pin mode กำหนดค่า
        
    if (Ethernet.begin(mac)) {
      Serial.println(Ethernet.localIP());
      microgear.init(KEY,SECRET,ALIAS);
      microgear.connect(APPID);
    }
}

void loop() {
    /* To check if the microgear is still connected */
    if (microgear.connected()) {

        /* Call this method regularly otherwise the connection may be lost */
        microgear.loop();
        timer = 0;
        
        light = analogRead(LDRPIN); 

        if(light<100 && !sendMsgLight){             // เมื่อแสงสว่างต่ำกว่า 100 ให้สั่งเปิดไฟที่ nodewifi
          Serial.println("Sending Light --> ON");
          microgear.chat(neighbor,"1");
          sendMsgLight = !sendMsgLight;
        }else if(light>200 && sendMsgLight){        // เมื่อแสงสว่างมากกว่า 200 ให้สั่งปิดไฟที่ nodewifi
          Serial.println("Sending Light --> OFF");
          microgear.chat(neighbor,"0");
          sendMsgLight = !sendMsgLight;
        }

        if(millis()-lastTimeDHT > 1000){
          lastTimeDHT = millis();
          
          float h = dht.readHumidity();     // อ่านค่าความชื้น
          float t = dht.readTemperature();  // อ่านค่าอุณหภูมิ
          
          Serial.print("Humidity: ");
          Serial.print(h);
          Serial.print(" %RH , ");
          Serial.print("Temperature: ");
          Serial.print(t);
          Serial.println(" *C ");

          // ตรวจสอบค่า humid และ temp เป็นตัวเลขหรือไม่
          if (isnan(h) || isnan(t)) {
            Serial.println("Failed to read from DHT sensor!");
          }else{
            humid = h;
            temp = t;

            if(temp>=27 && !sendMsgRelay){    // เมื่ออุณหภูมิมากกว่าหรือเท่ากับ 27 องศา สั่งเปิด relay ให้ทำงาน
               Serial.println("Sending Relay --> ON");
              sendMsgRelay = !sendMsgRelay;
              microgear.publish(topicRelayPublish,"1");
            }else if(temp<=25 && sendMsgRelay){    // เมื่ออุณหภูมิต่ำกว่าหรือเท่ากับ 25 องศา สั่งปิด relay ไม่ให้ทำงาน
               Serial.println("Sending Relay --> OFF");
              sendMsgRelay = !sendMsgRelay;
              microgear.publish(topicRelayPublish,"0");   // publish ข้อมูลส่งไปที่ topic /relay
            }
            
            String valuePublish = (String)humid+","+(String)temp+","+(String)light;
            Serial.print("Sending --> ");
            Serial.println(valuePublish);
            microgear.publish(topicPublish,valuePublish);   // publish ข้อมูลส่งไปที่ topic /dht
          }
        }
    }
    else {
        Serial.println("connection lost, reconnect...");
        if (timer >= 5000) {
            microgear.connect(APPID);
            timer = 0;
        }
        else timer += 100;
        delay(100);
    }
}
