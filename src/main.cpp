#include <Arduino.h>
#include <DHT.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

#define DHTPIN 5
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

const char* ssid = "Platform_9_3/4";
const char* password = "muriasy3";
const char* serverUrl = "https://crisweatherdashboard.duckdns.org/api/measurements";
const char* apiKey = "cJoseALNAtcQBpZKfrk1FaQ6";

float lastSentTempreature = -999.0;
float lastSentHumidity = -999.0;

unsigned long lastSentTime = 0;

const float TEMPERATURE_THRESHOLD = 0.1;
const float HUMIDITY_THRESHOLD = 1.0;

const unsigned long HEARTBEAT_INTERVAL = 60 * 60 * 1000; 
const unsigned long CHECK_INTERVAL = 20000;           

void setup() {
    Serial.begin(115200);
    dht.begin();
    pinMode(LED_BUILTIN, OUTPUT);

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    Serial.print("Connect to network: ");
    Serial.println(ssid);
}


void sendMeasurement(float temperature, float humidity) {
    digitalWrite(LED_BUILTIN, HIGH);

    WiFiClientSecure client;
    client.setInsecure();
   
    HTTPClient http;
    http.begin(client, serverUrl);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Accept", "application/json");
    http.addHeader("Authorization", String("Bearer ") + apiKey);
    
    String jsonData = "{\"measurement\":{\"temperature\":" + String(temperature, 1) + ",\"humidity\":" + String(humidity, 1) + "}}";
    int httpResponseCode = http.POST(jsonData);
   
    if (httpResponseCode > 0) {
        Serial.print("Response code HTTP: ");
        Serial.println(httpResponseCode);
        
        String serverResponse = http.getString();
        Serial.print("Response: ");
        Serial.println(serverResponse);

        JsonDocument document;
        DeserializationError error = deserializeJson(document, serverResponse);

        if (!error) {
            const char* message = document["message"];
            Serial.print("Message from server: ");
            Serial.println(message);
        } else {
            Serial.println("Error, parse of JSON!");
        }

        lastSentTempreature = temperature;
        lastSentHumidity = humidity;
        lastSentTime = millis();
    } else {
        Serial.print("Error HTTP POST, code: ");
        Serial.println(httpResponseCode);
    }
   
    http.end();
    digitalWrite(LED_BUILTIN, LOW);
}


void loop() {
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();
   
    if (isnan(humidity) || isnan(temperature)) {
        Serial.println("Error, read of sensor");
        delay(CHECK_INTERVAL);
        return;
    }


    bool firstRun = (lastSentTempreature == -999.0);
    bool temperatureChanged = abs(temperature - lastSentTempreature) >= TEMPERATURE_THRESHOLD;
    bool humidityChanged = abs(humidity - lastSentHumidity) >= HUMIDITY_THRESHOLD;
    bool heartbeat = (millis() - lastSentTime) >= HEARTBEAT_INTERVAL;

    if (firstRun || temperatureChanged || humidityChanged || heartbeat) {
        Serial.println("------------------------------------------------");
        Serial.print("Sending -> Temperature: ");
        Serial.print(temperature);
        Serial.print("°C | Humidity: ");
        Serial.print(humidity);
        Serial.println("%");

        sendMeasurement(temperature, humidity);
    } else {
        Serial.print("No change -> Temperature: ");
        Serial.print(temperature);
        Serial.print("°C | Humidity: ");
        Serial.print(humidity);
        Serial.println("%. Skipping POST.");
    }

    delay(CHECK_INTERVAL);
}