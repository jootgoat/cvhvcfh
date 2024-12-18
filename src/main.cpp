#include "secrets.h"
#include <WiFiClientSecure.h>
#include <MQTTClient.h>
#include <ArduinoJson.h>
#include "WiFi.h"

// The MQTT topics that this device should publish/subscribe
#define AWS_IOT_PUBLISH_TOPIC   "esp32/pub"
#define AWS_IOT_SUBSCRIBE_TOPIC "esp32/sub"

#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  30        /* Time ESP32 will go to sleep (in seconds) */

//PN532 CODING BEGINS
#include <Wire.h>
#include <Adafruit_PN532.h>
 
#define SDA_PIN 21
#define SCL_PIN 22
 
Adafruit_PN532 nfc(SDA_PIN, SCL_PIN);
int motorPin = 4;
//PN532 CODING ENDS

WiFiClientSecure net = WiFiClientSecure();
MQTTClient client = MQTTClient(256);

void publishMessage()
{
  StaticJsonDocument<200> doc;
  doc["time"] = millis();
  doc["sensor_a0"] = analogRead(0);
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer); // print to client

  client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
}

void messageHandler(String &topic, String &payload) {
  Serial.println("incoming: " + topic + " - " + payload);

//  StaticJsonDocument<200> doc;
//  deserializeJson(doc, payload);
//  const char* message = doc["message"];
}

void connectAWS()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.println("Connecting to Wi-Fi");

  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }

  // Configure WiFiClientSecure to use the AWS IoT device credentials
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);

  // Connect to the MQTT broker on the AWS endpoint we defined earlier
  client.begin(AWS_IOT_ENDPOINT, 8883, net);

  // Create a message handler
  client.onMessage(messageHandler);

  Serial.print("Connecting to AWS IOT");

  while (!client.connect(THINGNAME)) {
    Serial.print(".");
    delay(100);
  }

  if(!client.connected()){
    Serial.println("AWS IoT Timeout!");
    return;
  }

  // Subscribe to a topic
  client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);

  Serial.println("AWS IoT Connected!");
}


void setup() {
  Serial.begin(9600);
  connectAWS();
  //PN532 CODING BEGINS
  //Serial.begin(115200);
  Serial.println("Hello!");
 
  nfc.begin();
 
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.print("Didn't find PN53x board");
    while (1);
  }
 
  nfc.SAMConfig();
  Serial.println("Waiting for an NFC card ...");
  pinMode(motorPin, OUTPUT);
  //PN532 CODING ENDS
}

void loop() {
  publishMessage();
  //client.loop();
  delay(1000);
  
  //PN532 CODING BEGINS
  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };
  uint8_t uidLength;
  String vall = "";

  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
 
  if (success) {
    Serial.println("Found an NFC card!");
 
    Serial.print("UID Length: ");Serial.print(uidLength, DEC);Serial.println(" bytes");
    Serial.print("UID Value: ");
    for (uint8_t i=0; i < uidLength; i++) {
      Serial.print(" 0x");Serial.print(uid[i], HEX);
    }
    for (uint8_t i=0; i < uidLength; i++) {
      vall += String(uid[i], HEX);
      Serial.print(" 0x");Serial.print(uid[i], HEX);
    }
    Serial.println("");
    Serial.println(vall);
    if(vall == "41f97d57800"){
      Serial.println("Welcome");
      digitalWrite(motorPin, HIGH);
      delay(2000);
      digitalWrite(motorPin, LOW);
      esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
    }
 
    delay(1000);
  }

  //PN532 CODING ENDS
}