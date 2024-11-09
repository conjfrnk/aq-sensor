#include <DHT.h>
#include <Wire.h>
#include "rgb_lcd.h"            // Grove LCD library
#include "SparkFun_SGP30_Arduino_Library.h" // SGP30 library

// DHT11 Setup
#define DHTPIN 3                // Define the DHT11 data pin
#define DHTTYPE DHT11           // Define the sensor type
DHT dht(DHTPIN, DHTTYPE);

// MS1100 VOC Sensor Setup
#define Aout A0                 // Define the analog output pin for the MS1100

// Grove LCD Setup
rgb_lcd lcd;

// SGP30 Setup
SGP30 mySensor;                 // Create an SGP30 sensor object
long lastMeasurementTime = 0;

void setup() {
  Serial.begin(9600);
  dht.begin();
  pinMode(Aout, INPUT);

  // Initialize the Grove LCD
  lcd.begin(16, 2);             // 16 columns, 2 rows
  lcd.setRGB(0, 128, 255);      // Set LCD backlight color (optional)

  Serial.println("Starting sensor readings...");

  // Initialize SGP30 Sensor
  Wire.begin();
  if (mySensor.begin() == false) {
    Serial.println("No SGP30 Detected. Check connections.");
    while (1);
  }
  mySensor.initAirQuality();
}

void loop() {
  // Read temperature in Celsius and humidity from the DHT11
  float tempC = dht.readTemperature();
  float humidity = dht.readHumidity();

  // Check for failed readings
  if (isnan(tempC) || isnan(humidity)) {
    Serial.println("Failed to read from DHT sensor!");
    lcd.clear();
    lcd.print("DHT Error");
    delay(1000);
    return;
  }

  // Convert temperature to Fahrenheit
  float tempF = tempC * 1.8 + 32;

  // Read analog value from MS1100
  int analogValue = analogRead(Aout);
  float voltage = analogValue * (5.0 / 1023.0);

  // Calculate VOC concentrations in ppm based on formulas

  // Formaldehyde (Linear)
  float formaldehyde_ppm = pow(10, (-1.095) + 0.627 * voltage);

  // Toluene (Linear)
  float toluene_ppm = pow(10, (-3.478) + 1.104 * voltage);

  // Convert ppm to mg/m³
  float formaldehyde_mg_m3 = formaldehyde_ppm * (30.03 / 24.45); // 30.03 g/mol for formaldehyde
  float toluene_mg_m3 = toluene_ppm * (92.14 / 24.45);           // 92.14 g/mol for toluene

  // Calculate TVOC as the sum of individual VOCs in mg/m³
  float tvoc_mg_m3 = formaldehyde_mg_m3 + toluene_mg_m3;

  // SGP30 Sensor Data
  if (millis() - lastMeasurementTime >= 1000) {
    lastMeasurementTime = millis();
    mySensor.measureAirQuality();

    Serial.print("SGP30 eCO2: ");
    Serial.print(mySensor.CO2);
    Serial.print(" ppm\tSGP30 TVOC: ");
    Serial.print(mySensor.TVOC);
    Serial.println(" ppb");

    mySensor.measureRawSignals();
    Serial.print("SGP30 Raw H2: ");
    Serial.print(mySensor.H2);
    Serial.print("\tSGP30 Raw Ethanol: ");
    Serial.println(mySensor.ethanol);
  }

  // Print temperature, humidity, and VOC data to Serial Monitor
  Serial.print("Temperature: ");
  Serial.print(tempF);
  Serial.print("°F   Humidity: ");
  Serial.print((int)humidity);  // Cast humidity to an integer
  Serial.print("% RH   MS1100 TVOC (mg/m³): ");
  Serial.println(tvoc_mg_m3);

  // Display data on the Grove LCD
  lcd.clear();
  lcd.setCursor(0, 0);  // First line
  lcd.print(tempF);
  lcd.print("F  ");
  lcd.print((int)humidity);  // Display humidity as an integer
  lcd.print("%");

  lcd.setCursor(0, 1);  // Second line
  lcd.print(tvoc_mg_m3);
  lcd.print("VOC ");
  lcd.print(mySensor.CO2);
  lcd.print("CO2");

  // Wait 1 second before next reading
  delay(1000);
}
