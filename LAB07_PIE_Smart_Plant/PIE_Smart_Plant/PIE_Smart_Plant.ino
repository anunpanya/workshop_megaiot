#include <Ethernet.h>
#include <MicroGear.h>
#include "Arduino.h"
#include "Wire.h"
#include "uRTCLib.h"

#define APPID   "YOUR_APPID"                              // ให้แทนที่ด้วย AppID
#define KEY     "YOUR_KEY"                                // ให้แทนที่ด้วย Key
#define SECRET  "YOUR_SECRET"                             // ให้แทนที่ด้วย Secret
#define ALIAS   "mega2560"                                // ชื่ออุปกรณ์

#define topicPublish "/moist"                        // topic ที่ต้องการ publish ส่งข้อความ
#define topicRelayPublish "/relay"                        // topic ที่ต้องการ publish ส่งข้อความ

#define SWITCHPIN 3      // Switch pin
#define LEDPIN 13        // GPIO13 ขาที่ต่อเข้ากับขา S ของ LED
#define MOISTPIN A2      // Analog2 ขาที่ต่อเข้ากับขา S ของ Photocell Sensor
bool sendMsgRelay = false;            // สถานะส่งข้อความ
unsigned long lastTimePublish = 0;    // เก็บเวลาล่าสุดที่ส่งข้อความค่าความชื้นในดิน
int stateLED = 0;                     // สถานะ LED
bool stateChange = false;             // สถานะการกดปุ่ม

String dayOfWeek[] = {"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"};

//message string time
String m;                 // ตัวแปรเก็บข้อความชนิด string
int datetime[7];          // array สำหรับเก็บค่าเวลาการตั้งเวลาปัจจุบันของ RTC

//alarm
int datetimealarm[7];     // array สำหรับเก็บค่าเวลาการตั้งเวลาให้ทำงานตามเงื่อนไขที่กำหนด

//Placeholders within input substring, which will jump from delimiter to delimiter
int boundLow;             // ตำแหน่งต่ำสุดของการ substring
int boundHigh;            // ตำแหน่งสูงสุดของการ substring

//Delimiter character
const char delimiter = ',';   // ใช้สำหรับเช็ครูปแบบข้อความที่คั่นด้วยเครื่องหมาย comma

uRTCLib rtc;                  // ประกาศ object ของ uRTCLib ที่จะใช้งาน 
unsigned int pos;             // ตำแหน่ง index eeprom
int updateTime;               // เก็บค่าเวลา 1 วินาทีที่มีการเปลี่ยนแปลงเวลา

EthernetClient client;

int timer = 0;
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
MicroGear microgear(client);

/* If a new message arrives, do this */
void onMsghandler(char *topic, uint8_t* msg, unsigned int msglen) {
    Serial.print("Incoming message --> ");
    msg[msglen] = '\0';
    Serial.println((char *)msg);

    m = (char*)msg;

    // ตั้งเวลาปัจจุบันของ RTC
    if(String(topic) == ("/" APPID "/rtc/settime")){
      Serial.println(topic);
      if (m.length() > 0)
      {
        //second
        boundLow = m.indexOf(delimiter);
        if(boundLow!=-1) datetime[0] = m.substring(0, boundLow).toInt();
        
        //minute
        boundHigh = m.indexOf(delimiter, boundLow+1);
        if(boundLow!=-1 && boundHigh!=-1) datetime[1] = m.substring(boundLow+1, boundHigh).toInt();
        
        //hour
        boundLow = m.indexOf(delimiter, boundHigh+1);
        if(boundLow!=-1 && boundHigh!=-1) datetime[2] = m.substring(boundHigh+1, boundLow).toInt();
        
        //dayOfWeek
        boundHigh = m.indexOf(delimiter, boundLow+1);
        if(boundLow!=-1 && boundHigh!=-1) datetime[3] = m.substring(boundLow+1, boundHigh).toInt();
        
        //dayOfMonth
        boundLow = m.indexOf(delimiter, boundHigh+1);
        if(boundLow!=-1 && boundHigh!=-1) datetime[4] = m.substring(boundHigh+1, boundLow).toInt();
        
        //month
        boundHigh = m.indexOf(delimiter, boundLow+1);
        if(boundLow!=-1 && boundHigh!=-1) datetime[5] = m.substring(boundLow+1, boundHigh).toInt();
        
        //year
        if(boundLow!=-1 && boundHigh!=-1) datetime[6] = m.substring(boundHigh+1).toInt();

        if(datetime[0]!=-1 && datetime[1]!=-1 && datetime[2]!=-1 && datetime[3]!=-1 && datetime[4]!=-1 && datetime[5]!=-1 && datetime[6]!=-1){
          Serial.print("   Date: ");
          Serial.print(datetime[4]);
          Serial.print("/");
          Serial.print(datetime[5]);
          Serial.print("/");
          Serial.print(datetime[6]);
          Serial.print("  dayOfWeek: ");
          Serial.print(dayOfWeek[datetime[3]+1]);
          Serial.print("  Time: ");
          Serial.print(datetime[2]);
          Serial.print(":");
          Serial.print(datetime[1]);
          Serial.print(":");
          Serial.println(datetime[0]);
          rtc.set(datetime[0], datetime[1], datetime[2], datetime[3], datetime[4], datetime[5], datetime[6]); // กำหนดเวลา RTC
        }
        for(int i=0; i<sizeof(datetime);i++){  // reset ค่า
          datetime[i]=-1;
        }
        Serial.println("RESET ---------------------------------- ");
        Serial.print("   Date: ");
        Serial.print(datetime[4]);
        Serial.print("/");
        Serial.print(datetime[5]);
        Serial.print("/");
        Serial.print(datetime[6]);
        Serial.print("  dayOfWeek: ");
        Serial.print(datetime[3]+1);
        Serial.print("  Time: ");
        Serial.print(datetime[2]);
        Serial.print(":");
        Serial.print(datetime[1]);
        Serial.print(":");
        Serial.println(datetime[0]);
        Serial.println("---------------------------------------- ");
      }
    }

    // ตั้งเวลาทำงานอัตโนมัติ
    if(String(topic) == ("/" APPID "/rtc/setalarmon")){
      Serial.println(topic);
      if (m.length() > 0)
      {
        //second
        boundLow = m.indexOf(delimiter);
        if(boundLow!=-1) datetimealarm[0] = m.substring(0, boundLow).toInt();
        
        //minute
        boundHigh = m.indexOf(delimiter, boundLow+1);
        if(boundLow!=-1 && boundHigh!=-1) datetimealarm[1] = m.substring(boundLow+1, boundHigh).toInt();
        
        //hour
        boundLow = m.indexOf(delimiter, boundHigh+1);
        if(boundLow!=-1 && boundHigh!=-1) datetimealarm[2] = m.substring(boundHigh+1, boundLow).toInt();
        
        //dayOfWeek
        boundHigh = m.indexOf(delimiter, boundLow+1);
        if(boundLow!=-1 && boundHigh!=-1) datetimealarm[3] = m.substring(boundLow+1, boundHigh).toInt();
        
        //dayOfMonth
        boundLow = m.indexOf(delimiter, boundHigh+1);
        if(boundLow!=-1 && boundHigh!=-1) datetimealarm[4] = m.substring(boundHigh+1, boundLow).toInt();
        
        //month
        boundHigh = m.indexOf(delimiter, boundLow+1);
        if(boundLow!=-1 && boundHigh!=-1) datetimealarm[5] = m.substring(boundLow+1, boundHigh).toInt();
        
        //year
        if(boundLow!=-1 && boundHigh!=-1) datetimealarm[6] = m.substring(boundHigh+1).toInt();

        if(datetimealarm[0]!=-1 && datetimealarm[1]!=-1 && datetimealarm[2]!=-1 && datetimealarm[3]!=-1 && datetimealarm[4]!=-1 && datetimealarm[5]!=-1 && datetimealarm[6]!=-1){
          Serial.print("----> Alarm   Date: ");
          Serial.print(datetimealarm[4]);
          Serial.print("/");
          Serial.print(datetimealarm[5]);
          Serial.print("/");
          Serial.print(datetimealarm[6]);
          Serial.print("  dayOfWeek: ");
          Serial.print(dayOfWeek[datetimealarm[3]+1]);
          Serial.print("  Time: ");
          Serial.print(datetimealarm[2]);
          Serial.print(":");
          Serial.print(datetimealarm[1]);
          Serial.print(":");
          Serial.println(datetimealarm[0]);
        }
      }
    }
}

/* When a microgear is connected, do this */
void onConnected(char *attribute, uint8_t* msg, unsigned int msglen) {
    Serial.println("Connected to NETPIE...");
    /* Set the alias of this microgear ALIAS */
    microgear.setAlias(ALIAS);
    microgear.subscribe("/rtc/settime");      // subscribe topic /rtc/settime
    microgear.subscribe("/rtc/setalarmon");   // subscribe topic /rtc/setalarmon
}


void setup() {
    /* Add Event listeners */

    /* Call onMsghandler() when new message arraives */
    microgear.on(MESSAGE,onMsghandler);

    /* Call onConnected() when NETPIE connection is established */
    microgear.on(CONNECTED,onConnected);
    
    delay (2000);
    Serial.begin(9600);
    Serial.println("Starting...");

    // กำหนดชนิดของ PIN (ขาI/O) เช่น INPUT, OUTPUT เป็นต้น
    pinMode(LEDPIN, OUTPUT);          // LED pin mode กำหนดค่า
    stateLED = digitalRead(LEDPIN);
    pinMode(SWITCHPIN, INPUT);        // Switch pin mode รับค่า
    
    if (Ethernet.begin(mac)) {
      Serial.println(Ethernet.localIP());
      microgear.init(KEY,SECRET,ALIAS);
      microgear.connect(APPID);
    }
    
    for(pos = 0; pos < 1000; pos++) {
      rtc.eeprom_write(pos, (unsigned char) pos % 256);
    }
    
    //rtc.set(0, 42, 16, 6, 2, 5, 15);
    pos = 0;

    #ifdef _VARIANT_ARDUINO_STM32_
    Serial.println("Board: STM32");
    #else
    Serial.println("Board: Other");
    #endif

    // กำหนดค่าเริ่มต้นของ array datetime และ datetimealarm เป็น -1
    for(int i=0; i<sizeof(datetime);i++){
      datetime[i]=-1;
    }
    for(int i=0; i<sizeof(datetimealarm);i++){
      datetimealarm[i]=-1;
    }
}

void loop() {
    rtc.refresh();    // update ค่าเวลาปัจจุบัน
    /* To check if the microgear is still connected */
    if (microgear.connected()) {

        /* Call this method regularly otherwise the connection may be lost */
        microgear.loop();
        timer = 0;

        if(updateTime!=rtc.second()){
          updateTime = rtc.second();
          Serial.print("RTC DateTime: ");
          Serial.print(rtc.day());
          Serial.print('/');
          Serial.print(rtc.month());
          Serial.print('/');
          Serial.print(rtc.year());
          Serial.print(' ');
          Serial.print(dayOfWeek[rtc.dayOfWeek()]);
          Serial.print(' ');
          Serial.print(rtc.hour());
          Serial.print(':');
          Serial.print(rtc.minute());
          Serial.print(':');
          Serial.println(rtc.second());

          // ตรวจสอบเวลา ถ้าตรงกับเวลาที่ตั้งไว้ จะสั่งให้ Relay เปิด
          if(datetimealarm[0]!=-1 && datetimealarm[1]!=-1 && datetimealarm[2]!=-1 && datetimealarm[3]!=-1 && datetimealarm[4]!=-1 && datetimealarm[5]!=-1 && datetimealarm[6]!=-1){
            if(datetimealarm[0]==rtc.second() && datetimealarm[1]==rtc.minute() && datetimealarm[2]==rtc.hour() && datetimealarm[3]==rtc.dayOfWeek() && datetimealarm[4]==rtc.day() && datetimealarm[5]==rtc.month() && datetimealarm[6]==rtc.year()){
              microgear.publish(topicRelayPublish,"1");   // สั่งเปิด Relay
              for(int i=0; i<sizeof(datetimealarm);i++){  // reset ค่า
                datetimealarm[i]=-1;
              }
            }
          }
        }

        int state = digitalRead(SWITCHPIN);
        if(state==HIGH && !stateChange){
          stateChange = true;
        }

        if(state==LOW && stateChange) {
          stateChange = false;
          stateLED = !stateLED;
          microgear.publish(topicRelayPublish,stateLED);   // สั่งเปิดปิด Relay
        }
        
        int moist = analogRead(MOISTPIN);    // อ่านค่าความชื้นในดิน

        if(moist<300 && !sendMsgRelay){
          Serial.println("Sending Relay --> ON");
          microgear.publish(topicRelayPublish,"1");         // สั่งเปิด relay
          sendMsgRelay = !sendMsgRelay;
        }else if(moist>600 && sendMsgRelay){
          Serial.println("Sending Relay --> OFF");
          microgear.publish(topicRelayPublish,"0");         // สั่งปิด relay
          sendMsgRelay = !sendMsgRelay;
        }

        if(millis()-lastTimePublish > 1000){  // เมื่อครบ 1 วินาที จะเข้าทำงานเงื่อนไขด้านล่าง
          lastTimePublish = millis();
          Serial.print("Sending --> ");
          Serial.println(moist);
          microgear.publish(topicPublish,moist);     // pubslih ค่าไปที่ topic /moist
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
