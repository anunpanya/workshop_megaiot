#include <ESP8266WiFi.h>
#include <MicroGear.h>

const char* ssid     = "YOUR_WIFI_SSID";                  // ชื่อ ssid
const char* password = "YOUR_WIFI_PASSWORD";              // รหัสผ่าน wifi

#define APPID   "YOUR_APPID"                              // ให้แทนที่ด้วย AppID
#define KEY     "YOUR_KEY"                                // ให้แทนที่ด้วย Key
#define SECRET  "YOUR_SECRET"                             // ให้แทนที่ด้วย Secret
#define ALIAS   "nodewifi"                                // ชื่ออุปกรณ์

#define neighbor "mega2560"                               // ชื่ออุปกรณ์ที่ต้องการส่งข้อความไปให้

#define SWITCHPIN 0      // Switch pin
#define RELAYPIN 4       // 5V Relay pin
#define LEDPIN 5         // LED pin

int stateLED = 0;      // สถานะ LED
int stateRelay = 0;   // สถานะ Relay
bool stateChange = false; // สถานะการกดปุ่ม

WiFiClient client;

int timer = 0;
MicroGear microgear(client);

void onMsghandler(char *topic, uint8_t* msg, unsigned int msglen) {
    Serial.println("Incoming message :");
    msg[msglen] = '\0';
    
    if(String(topic) == ("/" APPID "/gearname/nodewifi")){
      Serial.print("  Topic --> ");
      Serial.print(topic);
      Serial.print(" : ");
      Serial.println((char *)msg);
    
      // สถานะ LED บน NodeMCU ที่แสดงผล จะติดก็ต่อเมื่อสั่ง LOW 
      // แต่ถ้าเป็น LED ที่ต่อแยก จะต้องสั่งเป็น HIGH 
      if(*(char *)msg == '1'){
          stateLED = 1;
          digitalWrite(LEDPIN, HIGH); // LED on
          microgear.chat(neighbor,stateLED);    // ตอบกลับไปที่ neighbor ชื่อ mega2560
      }else{
          stateLED = 0;
          digitalWrite(LEDPIN, LOW); // LED off
          microgear.chat(neighbor,stateLED);    // ตอบกลับไปที่ neighbor ชื่อ mega2560
      }
    }else if(String(topic) == ("/" APPID "/relay")){
      Serial.print("  Topic --> ");
      Serial.print(topic);
      Serial.print(" : ");
      Serial.println((char *)msg);
    
      if(*(char *)msg == '1'){
          stateRelay = 1;
          digitalWrite(RELAYPIN, HIGH); // LED on
          microgear.publish("/relay/state",stateRelay);   // publish ไปที่ topic /relay/state
      }else{
          stateRelay = 0;
          digitalWrite(RELAYPIN, LOW); // LED off
          microgear.publish("/relay/state",stateRelay);   // publish ไปที่ topic /relay/state
      }
    }
}

void onConnected(char *attribute, uint8_t* msg, unsigned int msglen) {
    Serial.println("Connected to NETPIE...");
    microgear.setAlias(ALIAS);
    microgear.subscribe("/relay");
}


void setup() {
    // กำหนดฟังก์ชั้นสำหรับรับ event callback 
    microgear.on(MESSAGE,onMsghandler);
    microgear.on(CONNECTED,onConnected);

    // กำหนด baud rate สำหรับการสื่อสาร
    Serial.begin(115200);
    Serial.println("Starting...");

    // กำหนดชนิดของ PIN (ขาI/O) เช่น INPUT, OUTPUT เป็นต้น
    pinMode(LEDPIN, OUTPUT);          // LED pin mode กำหนดค่า
    stateLED = digitalRead(LEDPIN);
    
    pinMode(RELAYPIN, OUTPUT);        // Relay pin mode กำหนดค่า
    digitalWrite(RELAYPIN, LOW);
    
    pinMode(SWITCHPIN, INPUT);        // Switch pin mode รับค่า

    // เชื่อมต่อ wifi
    if (WiFi.begin(ssid, password)) {
        while (WiFi.status() != WL_CONNECTED) {
            delay(1000);
            Serial.print(".");
        }
    }

    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    microgear.init(KEY,SECRET,ALIAS);   // กำหนดค่าตันแปรเริ่มต้นให้กับ microgear
    microgear.connect(APPID);           // ฟังก์ชั่นสำหรับเชื่อมต่อ NETPIE
}

void loop() {
    if (microgear.connected()) { // microgear.connected() เป็นฟังก์ชั่นสำหรับตรวจสอบสถานะการเชื่อมต่อ
        microgear.loop();     // เป็นฟังก์ชั่นสำหรับทวนสถานะการเชื่อมต่อกับ NETPIE (จำเป็นต้องมีใช้ loop)
        timer = 0;
        int state = digitalRead(SWITCHPIN);
        if(state==LOW && !stateChange){
          stateChange = true;
        }

        if(state==HIGH && stateChange) {
          stateChange = false;
          stateLED = !stateLED;
          microgear.chat(ALIAS,stateLED);
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
