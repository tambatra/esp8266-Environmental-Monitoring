#include <ESP8266WiFi.h>
#include <MQTT.h>
#include <Wire.h>
#include "DHT.h"
#include <Ticker.h>
#include "MQ135.h"

#define ANALOGPIN A0
#define DHTPIN D7 
#define LED_WIFI D6
#define LED_SERVER D8
#define LED_GREEN D3
#define LED_YELLOW D2
#define LED_RED D5
#define DHTTYPE DHT22 

//const char ssid[] = "BM652w-nxyKe4";
//const char pass[] = "N6ryT7nUq9";

const char ssid[] = "IotWAP";
const char pass[] = "tamb@871128";

WiFiClient net;
MQTTClient client;
Ticker tickerBlink;
DHT dht(DHTPIN, DHTTYPE);
MQ135 gasSensor = MQ135(ANALOGPIN);

volatile float RLOAD = 18.0;
volatile float RZERO = 73.0;
volatile int airLevel1 = 600;
volatile int airLevel2 = 1200;
volatile int samplingTime = 15;
volatile int airLevel = 0;
volatile float airLevelFloat = 0.0;

unsigned long lastMillis = 0;
int countAir = 0;

void turOffLEDs(){
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_YELLOW, LOW);
  digitalWrite(LED_RED, LOW);
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
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  if (isnan(h) || isnan(t)) {
    //Serial.println(F("Failed to read from DHT sensor!"));
  }
  else{
    h = h/0.8716;
    String temp = String(t);
    String hum = String(h);
    String ppm = String(airLevel);
    String payload = "{\"temperature\":" +temp+ ",\"humidity\":"+hum+ ",\"airQuality\":"+ppm+"}";
    client.publish("iot/zone2", payload);
    countAir = 0;
    airLevelFloat = 0.0;
    //Serial.println(payload);
  }
}

void connect() {
  //Serial.print("Checking wifi...");
  tickerBlink.detach();
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
  while (!client.connect("zone 2", "try", "try")) {
    //Serial.print(".");
    digitalWrite(LED_SERVER, HIGH);
    delay(250);
    digitalWrite(LED_SERVER, LOW);
    delay(250);
  }
  client.subscribe("broker/connection2");
  client.subscribe("broker/calibration2");
  tickerBlink.attach_ms(500, blink);
  //Serial.println("\nconnected!");
  client.publish("iot/connection/zone2", "connected");
}

void messageReceived(String &topic, String &m_payload) {
  //Serial.println("incoming: " + topic + " - " + payload);
  String message = m_payload;
  if(topic == "broker/connection2"){
    int commaPosition = message.indexOf(',');
    String l1 = message.substring(0,commaPosition);
    String l2 = message.substring(commaPosition+1);
    char airLevel1Char[l1.length()+1];
    char airLevel2Char[l1.length()+1];
    l1.toCharArray(airLevel1Char, l1.length()+1);
    l2.toCharArray(airLevel2Char, l2.length()+1);
    airLevel1 = atoi(airLevel1Char);
    airLevel2 = atoi(airLevel2Char);
    // Serial.println(airLevel1);
    // Serial.println(airLevel2);
  }
  if(topic == "broker/calibration2"){
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
  dht.begin();  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  client.begin("192.168.43.164", net);
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
    airLevelFloat += gasSensor.getPPM(RLOAD, RZERO);
    //airLevelFloat += gasSensor.getVoltage();
    if(countAir>=samplingTime){
      airLevelFloat = airLevelFloat / float(samplingTime);
      airLevel = (int)airLevelFloat;
      publish();
    }
  }
}