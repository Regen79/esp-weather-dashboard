#include <Arduino.h>
#include <DHT.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#define DHTPIN 5
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

const char* ssid = "Platform_9_3/4";
const char* password = "muriasy3";
const char* serverUrl = "http://192.168.1.127:3000/api/measurements";
const char* apiKey = "yQ2Xe3JHc8zSvwCjV2vniPad";

void setup(){
    Serial.begin(115200);
    dht.begin();
    pinMode(LED_BUILTIN, OUTPUT);

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED){
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    Serial.print("Connect to network: ");
    Serial.println(ssid);
}

void loop(){
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();
   
    if (isnan(humidity) || isnan(temperature)){
        Serial.println("Error, read of sensor");
        delay(2000);
        return;
    }

    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.print("% ;");
    Serial.print(" Temperature: ");
    Serial.print(temperature);
    Serial.println("°C ");

    digitalWrite(LED_BUILTIN, HIGH);
    delay(1000);
    digitalWrite(LED_BUILTIN, LOW);
   
    HTTPClient http;
    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Accept","application/json");
    http.addHeader("Authorization", String("Bearer ") + apiKey);
    
    String jsonData = "{\"measurement\":{\"temperature\":" + String(temperature) + ",\"humidity\":" + String(humidity) + "}}";
    int httpResponseCode = http.POST(jsonData);
   
    if (httpResponseCode > 0){
        Serial.print("Response code HTTP: ");
        Serial.println(httpResponseCode);
        
        String serverResponse = http.getString();
        Serial.print("Response: ");
        Serial.println(serverResponse);

        JsonDocument document;
        DeserializationError error = deserializeJson(document, serverResponse);

        if(!error) {
            const char* message = document["message"];
            Serial.print("Message from server: ");
            Serial.println(message);
        } else {
            Serial.println("Error, parse of JSON!");
        }
    } else {
        Serial.print("Error HTTP POST, code: ");
        Serial.println(httpResponseCode);
    }
   
    http.end();
    delay(60000);
}