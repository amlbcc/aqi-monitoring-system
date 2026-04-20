#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

const char* ssid = "Redmi Note 12 Pro 5G";
const char* password = "atlantis";

String writeAPI = "VVHKBIU17Z53C9J9";
String channelID = "3304823";

WiFiClient client;

float temp, hum, co2ppm, coppm, dust, aqi;
float predictedAQI = -1;

unsigned long lastRead = 0;

void setup() {
  Serial.begin(9600);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
}

// -------- READ PREDICTION --------
void readPrediction() {
  if (WiFi.status() == WL_CONNECTED) {

    HTTPClient http;
    String url = "http://api.thingspeak.com/channels/" + channelID + "/fields/7/last.txt";

    http.begin(client, url);   // FIXED API

    int httpCode = http.GET();

    if (httpCode > 0) {
      String payload = http.getString();
      float val = payload.toFloat();

      if (val > 10 && val < 500) {
        predictedAQI = val;
      }
    }

    http.end();
  }
}

void loop() {

  // -------- RECEIVE FROM ARDUINO --------
  if (Serial.available()) {

    String data = Serial.readStringUntil('\n');

    if (!data.startsWith("P:")) {

      sscanf(data.c_str(), "%f,%f,%f,%f,%f,%f",
             &temp, &hum, &co2ppm, &coppm, &dust, &aqi);

      // -------- UPLOAD --------
      if (client.connect("api.thingspeak.com", 80)) {

        String url = "/update?api_key=" + writeAPI +
                     "&field1=" + String(temp) +
                     "&field2=" + String(hum) +
                     "&field3=" + String(co2ppm) +
                     "&field4=" + String(coppm) +
                     "&field5=" + String(dust) +
                     "&field6=" + String(aqi);

        client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                     "Host: api.thingspeak.com\r\n" +
                     "Connection: close\r\n\r\n");
      }

      client.stop();
    }
  }

  // -------- FETCH PREDICTION --------
  if (millis() - lastRead > 30000) {
    readPrediction();
    lastRead = millis();
  }

  // -------- SEND BACK --------
  if (predictedAQI > 0) {
    Serial.print("P:");
    Serial.println(predictedAQI);
  }

  delay(15000);
}