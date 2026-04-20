#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define DHTPIN 2
#define DHTTYPE DHT11

#define MQ135 A0
#define MQ9   A1
#define DUST  A2
#define LEDPIN 7

DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);

float predictedAQI = -1;

//CALIBRATED AQI 
float calculateAQI(float co2, float co, float dust) {

  // Normalize (based on real ranges)
  float dustIndex = dust * 0.4;        // PM dominant
  float coIndex   = co * 6;
  float co2Index  = (co2 - 400) * 0.05;

  float aqi = dustIndex + coIndex + co2Index;

  // Calibration factor
  aqi = aqi * 1.2;

  if (aqi < 0) aqi = 0;
  if (aqi > 500) aqi = 500;

  return aqi;
}

void setup() {
  Serial.begin(9600);
  dht.begin();
  lcd.init();
  lcd.backlight();
  pinMode(LEDPIN, OUTPUT);
}

void loop() {

  //READ PREDICTION 
  if (Serial.available()) {
    String data = Serial.readStringUntil('\n');

    if (data.startsWith("P:")) {
      float val = data.substring(2).toFloat();
      if (val > 10 && val < 500) {
        predictedAQI = val;
      }
    }
  }

  float temp = dht.readTemperature();
  float hum  = dht.readHumidity();

  int rawMQ135 = analogRead(MQ135);
  int rawMQ9   = analogRead(MQ9);

  //DUST SENSOR 
  digitalWrite(LEDPIN, LOW);
  delayMicroseconds(280);
  int rawDust = analogRead(DUST);
  delayMicroseconds(40);
  digitalWrite(LEDPIN, HIGH);

  float voltage = rawDust * (5.0 / 1023.0);
  float dustDensity = (voltage - 0.1) * 1000;
  if (dustDensity < 0) dustDensity = 0;

  //GAS SCALING 
  float co2ppm = map(rawMQ135, 0, 1023, 400, 2000);
  float coppm  = map(rawMQ9,   0, 1023, 0, 20);

  float aqi = calculateAQI(co2ppm, coppm, dustDensity);

  //SEND TO ESP 
  Serial.print(temp); Serial.print(",");
  Serial.print(hum); Serial.print(",");
  Serial.print(co2ppm); Serial.print(",");
  Serial.print(coppm); Serial.print(",");
  Serial.print(dustDensity); Serial.print(",");
  Serial.println(aqi);

  //LCD 

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("T:");
  lcd.print(temp);
  lcd.print(" H:");
  lcd.print(hum);
  delay(2000);

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("CO:");
  lcd.print(coppm);
  lcd.setCursor(0,1);
  lcd.print("CO2:");
  lcd.print(co2ppm);
  delay(2000);

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Dust:");
  lcd.print(dustDensity);
  lcd.setCursor(0,1);
  lcd.print("AQI:");
  lcd.print(aqi);
  delay(2000);

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Pred:");
  lcd.setCursor(0,1);

  if (predictedAQI > 0) lcd.print(predictedAQI);
  else lcd.print("Waiting");
  delay(2000);
}