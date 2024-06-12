// Total project
#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>
#include "DHT.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_NeoPixel.h>

// WiFi credentials
const char* ssid = "embed";
const char* password = "weareincontrol";

const char* temperatureground_topic = "home/livingroom/temperature_ground";
const char* temperatureair_topic = "home/livingroom/temperatureair";
const char* humidityair_topic = "home/livingroom/humidityair";
const char* humidityground_topic = "home/livingroom/humidityground";
const char* light_topic = "home/livingroom/light";
const char* waterlevel_topic = "home/livingroom/waterlevel";
const char* water_percent_topic = "home/livingroom/waterpercent";

const char* mqtt_server = "192.168.1.248";
const int mqtt_port = 1883;
const char* mqtt_user = "dylanpi";           // If using authentication
const char* mqtt_password = "dylanpi";       // If using authentication
const char* clientID = "client_livingroom";  // MQTT client ID

WiFiClient espClient;
PubSubClient client(espClient);
// Ledring

#define RGBLedPin 27
#define NUMPIXELS 12
Adafruit_NeoPixel pixels(NUMPIXELS, RGBLedPin, NEO_GRB + NEO_KHZ800);

// LCD settings
#define LCD_ADDR 0x27  // Change if needed
#define LCD_COLUMNS 20
#define LCD_ROWS 4

// RFID settings
#define SS_PIN 5
#define RST_PIN 4  // Changed from 22 to 4 to avoid conflict with I2C SCL

#define DHT11PIN 25
// ground temp sensor
#define ONE_WIRE_BUS 13
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);
float groundtemp;

MFRC522 mfrc522(SS_PIN, RST_PIN);                        // Create MFRC522 instance
LiquidCrystal_I2C lcd(LCD_ADDR, LCD_COLUMNS, LCD_ROWS);  // Create LCD instance
DHT dht(DHT11PIN, DHT11);

float AirHum;
float AirTem;
float WaterPercent;
int LCDState ;

#define GroundHumPin 34
int GroundHumValue;

#define LightPin 35
int LightValue;

#define trigPin 32
#define echoPin 33
float WaterLevel;

#define FanPin 14
#define WaterPump 12
#define RGBLight 27

int TargetGroundTemp;
int TargetAirTemp;
int TargetGroundHum;
int TargetAirHum;
int TargetLight;
int TargetWater;
long tagValue;
/////////////////////////////////////////////// test waarden
int value1 = 10;
int value2 = 10;
int value3 = 10;
int value4 = 10;
int value5 = 10;
int value6 = 10;
////////////////////////////////////////////////


void setup() {
  Serial.begin(115200);
  // Initialize I2C LCD
  digitalWrite(WaterPump,LOW);
  lcd.init();
  lcd.backlight();
  lcd.print("Scan an RFID tag");
  pixels.begin();

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);

  // Initialize SPI and RFID reader
  SPI.begin();
  mfrc522.PCD_Init();

  /* Start the DHT11 Sensor */
  dht.begin();
  sensors.begin();
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(FanPin, OUTPUT);
  pinMode(WaterPump, OUTPUT);
  pinMode(RGBLight, OUTPUT);


  Serial.println("Ready to scan an RFID tag...");
}

void loop() {
  client.loop();

  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    if (client.connect("ESP32Client", mqtt_user, mqtt_password)) {
      Serial.println("connected");
    } else {
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(5000);
    }
  }

  // Continuously check for RFID tags
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    // Show UID on Serial Monitor and LCD
    Serial.print("Card UID:");
    String content = "";
    //long tagValue = 0;
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
      Serial.print(mfrc522.uid.uidByte[i], HEX);
      content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
      content.concat(String(mfrc522.uid.uidByte[i], HEX));
      tagValue = tagValue * 256 + mfrc522.uid.uidByte[i];
    }
    Serial.println();
    Serial.print("Tag value in decimal: ");
    Serial.println(tagValue);
  


    // Halt briefly to avoid immediate re-reading of the same tag
    delay(300);
  }
  ////////////////////////////////////////////
  String payload = String("sensors value1=") + String(value1) + ",value2=" + String(value2) + ",value3=" + String(value3) + ",value4=" + String(value4) + ",value5=" + String(value5) + ",value6=" + String(value6);


 client.publish("esp32/data", (char*)payload.c_str());

  ////////////////////////////////////////////////////
  // Perform other tasks here if necessary
  IDCheck();

  AirTempHumSensor();

  GroundTemp();

  GroundMoisture();

  LightSens();

  WaterLevelSens();
  LCDDisplay();
  SendDataToMqtt();



  // Small delay to prevent excessive CPU usage
  delay(500);
}
void LCDDisplay(){
  lcd.clear();
  if (LCDState == 0){
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(AirHum);
    lcd.setCursor(4,0);
    lcd.print("Percent AH");
    lcd.setCursor(0,1);
    lcd.print(AirTem);
    lcd.setCursor(4,1);
    lcd.print("Deg AirT");
    lcd.setCursor(0,2);
    lcd.print(groundtemp);
    lcd.setCursor(4,2);
    lcd.print("Deg GroundT");
    lcd.setCursor(0,3);
    lcd.print(GroundHumValue);
    lcd.setCursor(4,3);
    lcd.print("Hum Percent G");
    LCDState = 1;
  }
  else {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(LightValue);
    lcd.setCursor(4,0);
    lcd.print("Percent Light");
    lcd.setCursor(0,1);
    lcd.print(WaterLevel);
    lcd.setCursor(4,1);
    lcd.print("Cm");
    lcd.setCursor(0,2);
    lcd.print(WaterPercent);
    lcd.setCursor(4,2);
    lcd.print("Percent Full");
    LCDState = 0;    
  }
}
void LedOn(){
  pixels.setPixelColor(1, pixels.Color(255, 255, 255));
  pixels.setPixelColor(2, pixels.Color(255, 255, 255));
  pixels.setPixelColor(3, pixels.Color(255, 255, 255));
  pixels.setPixelColor(4, pixels.Color(255, 255, 255));
  pixels.setPixelColor(5, pixels.Color(255, 255, 255));
  pixels.setPixelColor(6, pixels.Color(255, 255, 255));
  pixels.setPixelColor(7, pixels.Color(255, 255, 255));
  pixels.setPixelColor(8, pixels.Color(255, 255, 255));
  pixels.setPixelColor(9, pixels.Color(255, 255, 255));
  pixels.setPixelColor(10, pixels.Color(255, 255, 255));
  pixels.setPixelColor(11, pixels.Color(255, 255, 255));
  pixels.setPixelColor(12, pixels.Color(255, 255, 255));
  pixels.show();
  delay(200);
  pixels.clear();
  delay(10);  
}
void LedOff(){
  pixels.setPixelColor(1, pixels.Color(0, 0, 0));
  pixels.setPixelColor(2, pixels.Color(0, 0, 0));
  pixels.setPixelColor(3, pixels.Color(0, 0, 0));
  pixels.setPixelColor(4, pixels.Color(0, 0, 0));
  pixels.setPixelColor(5, pixels.Color(0, 0, 0));
  pixels.setPixelColor(6, pixels.Color(0, 0, 0));
  pixels.setPixelColor(7, pixels.Color(0, 0, 0));
  pixels.setPixelColor(8, pixels.Color(0, 0, 0));
  pixels.setPixelColor(9, pixels.Color(0, 0, 0));
  pixels.setPixelColor(10, pixels.Color(0, 0, 0));
  pixels.setPixelColor(11, pixels.Color(0, 0, 0));
  pixels.setPixelColor(12, pixels.Color(0, 0, 0));
  pixels.show();
  delay(200);
  pixels.clear();
  delay(10); 

}
void IDCheck(){
  // Give each plant dedicated target values it want's to reach
  if (tagValue == 1666537181){
    //Basil plant
    TargetGroundTemp = 26;
    TargetAirTemp = 26;
    TargetGroundHum = 80;
    TargetAirHum = 10;
    TargetLight = 80;
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Basil");
  }
  else if (tagValue == -479294755){  
  // CherryTomato
    TargetGroundTemp = 29;
    TargetAirTemp = 29;
    TargetGroundHum = 10;
    TargetAirHum = 70;
    TargetLight = 80;
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Kerstomaat");
  }
  else if (tagValue == -1016820723)
  // Cilantro 
    TargetGroundTemp = 20;
    TargetAirTemp = 20;
    TargetGroundHum = 30; 
    TargetAirHum = 60;
    TargetLight = 80;
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Koriander");
}

void AirTempHumSensor() {
  AirHum = dht.readHumidity();
  AirTem = dht.readTemperature();
  Serial.print("Temperature: ");
  Serial.print(AirTem);
  Serial.print("ºC ");
  Serial.print("Humidity: ");
  Serial.println(AirHum);
  if (AirHum > TargetAirHum){
    digitalWrite(FanPin, HIGH);
  }
  else {
    digitalWrite(FanPin, LOW);
  }
}

void GroundTemp() {
  // call sensors.requestTemperatures() to issue a global temperature
  // request to all devices on the bus
  Serial.print("Requesting temperatures...");
  sensors.requestTemperatures();  // Send the command to get temperatures
  Serial.println("DONE");
  // After we got the temperatures, we can print them here.
  // We use the function ByIndex, and as an example get the temperature from the first sensor only.
  groundtemp = sensors.getTempCByIndex(0);

  // Check if reading was successful
  if (groundtemp != DEVICE_DISCONNECTED_C) {
    Serial.print("Temperature for the device 1 (index 0) is: ");
    Serial.println(groundtemp);
  } else {
    Serial.println("Error: Could not read temperature data");
  }
  delay(10);
}

void GroundMoisture() {
  GroundHumValue = analogRead(GroundHumPin);
  GroundHumValue = map(GroundHumValue, 0, 4095, 100, 0);
  Serial.print("Mositure : ");
  Serial.print(GroundHumValue);
  Serial.println("%");
  delay(10);

  Serial.println(GroundHumValue);
  Serial.println(TargetGroundHum);
  if (GroundHumValue < TargetGroundHum){
    //digitalWrite(WaterPump, HIGH);
    delay(1000);
    digitalWrite(WaterPump,LOW);

  }
}

void LightSens() {
  LightValue = analogRead(LightPin);
  LightValue = map(LightValue, 0, 4095, 0, 100);
  Serial.println(LightValue);
  Serial.print("Light %");
  delay(10);
  if (LightValue < TargetLight){
    LedOn();
  }
  else {
    digitalWrite(RGBLight,LOW);
  }
}

void WaterLevelSens() {
  long duration, distance;
  digitalWrite(trigPin, LOW);  
  delayMicroseconds(2);        
  digitalWrite(trigPin, HIGH);
  
  delayMicroseconds(10);  
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  distance = (duration / 2) / 29.1;
  WaterLevel = (float)distance;
  Serial.print(distance);
  Serial.println(" cm");
  WaterPercent = map(distance, 0,9, 100,0);  
  Serial.println(WaterPercent);
  }



void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32Client", mqtt_user, mqtt_password)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void SendDataToMqtt() {
  // Stuurt data naar de Raspberry Pi

  String LightMqtt = String((float)LightValue);
  String GroundTempMqtt = String((float)groundtemp);
  String AirtempMqtt = String((float)AirTem);
  String AirHumMqtt = String((float)AirHum);
  String GroundHumMqtt = String((float)GroundHumValue);
  String WaterLevelMqtt = String((float)WaterLevel);
  String WaterPercentMqtt = String((float)WaterPercent);



  client.publish(light_topic, LightMqtt.c_str());
  delay(1000);
  client.publish(temperatureground_topic, GroundTempMqtt.c_str());
  delay(1000);
  client.publish(temperatureair_topic, AirtempMqtt.c_str());
  delay(1000);
  client.publish(humidityair_topic, AirHumMqtt.c_str());
  delay(1000);
  client.publish(humidityground_topic, GroundHumMqtt.c_str());
  delay(1000);
  client.publish(waterlevel_topic, WaterLevelMqtt.c_str());
  delay(1000);
  client.disconnect();  // disconnect from the MQTT broker
}