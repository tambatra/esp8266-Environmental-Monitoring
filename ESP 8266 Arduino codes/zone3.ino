#include <ESP8266WiFi.h>
#include <MQTT.h>
#include <Wire.h>
#include <Ticker.h>
#include "MQ135.h"

#define ANALOGPIN A0
#define LED_WIFI D6
#define LED_SERVER D8
#define LED_GREEN D3
#define LED_YELLOW D2
#define LED_RED D5
#define INTERRUPT_PIN D1

//const char ssid[] = "BM652w-nxyKe4";
//const char pass[] = "N6ryT7nUq9";

const char ssid[] = "IotWAP";
const char pass[] = "tamb@871128";

WiFiClient net;
MQTTClient client;
Ticker tickerBlink;
MQ135 gasSensor = MQ135(ANALOGPIN);

String ipAddress;
String payload;
volatile int airLevel = 0;
volatile float airLevelFloat = 0.0;
volatile float dose = 0.0;
volatile unsigned long lastMillis = 0;
volatile unsigned long count = 0; //value from 0 to 4294967295
volatile float RLOAD = 18.0;
volatile float RZERO = 73.0;
volatile int airLevel1 = 600;
volatile int airLevel2 = 1200;
volatile int samplingTime = 15;
volatile int countAir = 0;

void turOffLEDs(){
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_YELLOW, LOW);
  digitalWrite(LED_RED, LOW);
}

void ICACHE_RAM_ATTR increment(){
  count++;
}

void blink(){
  int LED_YELLOW_STATE = digitalRead(LED_YELLOW);
  int LED_RED_STATE = digitalRead(LED_RED);
  if(airLevel<airLevel1){
    // blink only GREEN
    int LED_GREEN_STATE = digitalRead(LED_GREEN);
    digitalWrite(LED_GREEN, !LED_GREEN_STATE);
    digitalWrite(LED_YELLOW, LOW);
    digitalWrite(LED_RED, LOW);
  }
  else if(airLevel<airLevel2){
    // blink GREEN and YELLOW
    int LED_YELLOW_STATE = digitalRead(LED_YELLOW);
    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(LED_YELLOW, !LED_YELLOW_STATE);
    digitalWrite(LED_RED, LOW);
  }
  else{
    // blink GREEN, YELLOW and RED
    int LED_RED_STATE = digitalRead(LED_RED);
    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(LED_YELLOW, HIGH);
    digitalWrite(LED_RED, !LED_RED_STATE);
  }

}

void publish(){
  //String radiation = String(count);
  String radiation = String(dose);
  String airQ = String(airLevel);
  String payload = "{\"radiation\":" +radiation+ ",\"airQuality\":"+airQ+"}";
  client.publish("iot/zone3", payload);
  //Serial.println(payload);
}

void connect() {
  //Serial.print("Checking wifi...");
  tickerBlink.detach();
  detachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN)); 
  turOffLEDs();
  while (WiFi.status() != WL_CONNECTED) {
    //Serial.print(".");
    digitalWrite(LED_WIFI, HIGH);
    digitalWrite(LED_SERVER, HIGH);
    delay(250);
    digitalWrite(LED_SERVER, LOW);
    digitalWrite(LED_WIFI, LOW);
    delay(250);
  }
  //Serial.print("\nConnecting...");
  while (!client.connect("zone 3", "try", "try")) {
    //Serial.print(".");
    digitalWrite(LED_SERVER, HIGH);
    delay(250);
    digitalWrite(LED_SERVER, LOW);
    delay(250);
  }
  client.subscribe("broker/connection3");
  client.subscribe("broker/calibration3");
  attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), increment, RISING); 
  tickerBlink.attach_ms(500, blink);
  client.publish("iot/connection/zone3", "connected");
  //Serial.println("\nconnected!");
}


void messageReceived(String &topic, String &m_payload) {
  //Serial.println("incoming: " + topic + " - " + payload);
  String message = m_payload;
  if(topic == "broker/connection3"){
    int commaPosition = message.indexOf(',');
    String l1 = message.substring(0,commaPosition);
    String l2 = message.substring(commaPosition+1);
    char radLevel1Char[l1.length()+1];
    char radLevel2Char[l1.length()+1];
    l1.toCharArray(radLevel1Char, l1.length()+1);
    l2.toCharArray(radLevel2Char, l2.length()+1);
    airLevel1 = atoi(radLevel1Char);
    airLevel2 = atoi(radLevel2Char);
    // Serial.println(radLevel1);
    // Serial.println(radLevel2);
  }
  else if(topic == "broker/calibration3"){
    int commaPosition = message.indexOf(',');
    String rl = message.substring(0,commaPosition);
    message = message.substring(commaPosition+1);
    commaPosition = message.indexOf(',');
    String rz = message.substring(0,commaPosition);
    String st = message.substring(commaPosition+1);
    char sampleTimeChar[st.length()+1];
    st.toCharArray(sampleTimeChar, st.length()+1);
    RLOAD = rl.toFloat();
    RZERO = rz.toFloat();
    samplingTime = atoi(sampleTimeChar);
    // Serial.println(RLOAD);
    // Serial.println(RZERO);
    // Serial.println(samplingTime);
  }
}



void setup() {
  //Serial.begin(115200);
  pinMode(LED_WIFI, OUTPUT);
  pinMode(LED_SERVER, OUTPUT);  
  pinMode(LED_GREEN, OUTPUT); 
  pinMode(LED_YELLOW, OUTPUT); 
  pinMode(LED_RED, OUTPUT);  
  pinMode(INTERRUPT_PIN, INPUT);  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  client.begin("192.168.1.5", net);
  client.onMessage(messageReceived);
  connect(); 
}

void loop() {
  client.loop();
  delay(10);  // <- fixes some issues with WiFi stability

  if (!client.connected()) {
    connect();
  }

  if (millis() - lastMillis > 1000) {
    lastMillis = millis();
    countAir++;
    //airLevelFloat += gasSensor.getPPM(RLOAD, RZERO);
    airLevelFloat += gasSensor.getVoltage();
    if(countAir>=samplingTime){
      float x = float(count)/float(samplingTime);
      dose = 0.8225 * x;
      count = (int)x;
      airLevelFloat = airLevelFloat / float(samplingTime);
      airLevel = (int)airLevelFloat;
      publish();
      countAir = 0;
      airLevelFloat = 0.0;
      count = 0;
    }
  }
}