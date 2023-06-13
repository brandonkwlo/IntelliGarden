#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_TSL2591.h>
#include <DHT.h>
#include <SPI.h>
#if defined(ESP32) || defined(ARDUINO_RASPBERRY_PI_PICO_W)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>

#define WIFI_SSID "UCInet Mobile Access"
#define WIFI_PASSWORD "\0"
#define FIREBASE_HOST HOST
#define API_KEY APIKEY
#define FIREBASE_PROJECT_ID PROJID
#define USER_EMAIL EMAIL
#define USER_PASSWORD PSWRD

#define DHTPIN 33
#define DHTTYPE DHT11
#define soilPin 36
#define numOfReadings 30
#define relayPin 17
int soilReadings[numOfReadings];
int soilValue = 0.0;
float minValue = 5000;
float maxValue = -5000;

Adafruit_TSL2591 uvsensor = Adafruit_TSL2591 (2591);
DHT dht(DHTPIN, DHTTYPE);

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long dataMillis = 0;
String manualWaterStat = "false";

double celToFah(int temp){
  double fah = 0.0;
  return (double)(temp * 9) / 5 + 32;
}

void setup()
{
  Serial.begin(115200);
  dht.begin(); 
  uvsensor.begin();
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, HIGH);
  delay(10000);
    Serial.println("Calibration starting...");
    for (int i = 0; i < numOfReadings; ++i) {
        soilValue = analogRead(soilPin); 
        soilReadings[i] = soilValue;
        if (soilValue < minValue) {
            minValue = soilValue;
        }
        if (soilValue > maxValue) {
            maxValue = soilValue;
        }
        delay(1000); 
    }
  Serial.println("End Calibration");
  delay(8000);


  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  config.host = FIREBASE_HOST;
  config.api_key = API_KEY;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void loop()
{
  int h = dht.readHumidity();
  int t = dht.readTemperature();
  double fahrenheit = celToFah(t);
  uint32_t lum = uvsensor.getFullLuminosity();
  uint16_t ir, full;
  ir = lum >> 16;
  full = lum & 0xFFFF;
  soilValue = analogRead(soilPin);

  float moisturePercentage = map(soilValue, maxValue, minValue, 0, 100);
  if (Firebase.ready() && (millis() - dataMillis > 5000 || dataMillis == 0))
  {
    dataMillis = millis();
    FirebaseJson content;

    String documentPath = "Data/TEMPORARY";
    String documentPath2 = "Data/MANUAL";

    content.clear();
    content.set("fields/humidity/integerValue", String(h).c_str());
    content.set("fields/temp/doubleValue", fahrenheit);
    content.set("fields/uv/doubleValue", full);
    content.set("fields/soil/doubleValue", moisturePercentage);
    Serial.print("Update a document... ");

    if (Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.raw(), "humidity,temp,uv,soil"))
        Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
    else
        Serial.println(fbdo.errorReason());

    Serial.print("Get a document... ");
    if (Firebase.Firestore.getDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath2.c_str(), "water"))
        Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
    else
        Serial.println(fbdo.errorReason());
    
    content.setJsonData(fbdo.payload());
    FirebaseJsonData result;
    content.get(result, "fields/water/booleanValue");
    manualWaterStat = result.to<String>();
    if(manualWaterStat == "true")
    {
      manualWaterStat = "false";
      content.clear();
      content.set("fields/water/booleanValue", false);
      Serial.print("Update a document... ");
      if (Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath2.c_str(), content.raw(), "water"))
        Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
      else
        Serial.println(fbdo.errorReason());

      digitalWrite(relayPin, LOW);
      delay(2000);
      digitalWrite(relayPin, HIGH);
    }
  }
  delay(1000);
}
