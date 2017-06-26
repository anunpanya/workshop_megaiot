#include <Ethernet.h>
#include <MicroGear.h>

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
        stateLED = 1;
        digitalWrite(LEDPIN, HIGH); // LED on
    }else{
        stateLED = 0;
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

    // กำหนดชนิดของ PIN (ขาI/O) เช่น INPUT, OUTPUT เป็นต้น
    pinMode(LEDPIN, OUTPUT);          // LED pin mode กำหนดค่า
    stateLED = digitalRead(LEDPIN);
    pinMode(SWITCHPIN, INPUT);        // Switch pin mode รับค่า
        
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

        int state = digitalRead(SWITCHPIN);
        if(state==HIGH && !stateChange){
          stateChange = true;
        }
        if(state==LOW && stateChange) {
          stateChange = false;
          stateLED = !stateLED;
          microgear.publish(topicRelayPublish,stateLED);    // pubslih ค่าไปที่ topic /relay
        }
        
        int moist = analogRead(MOISTPIN);   // อ่านค่าความชื้นในดิน

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
          microgear.publish(topicPublish,moist);      // pubslih ค่าไปที่ topic /moist
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
