#include <Wire.h>
#include <WiFi.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <HTTPClient.h>

// Wi-Fi settings
const char* ssid = "BATTLESTATION_MINI";
const char* password = "nemmondommeg";

// ThingSpeak settings
const char* server = "api.thingspeak.com";
String apiKey = "VPIB6GOEEX0TLHT4"; // Replace with your ThingSpeak API key

// Pins
#define SDA_PIN 8 
#define SCL_PIN 9
#define ADC_PIN 0 // GPIO pin for voltage measurement

// Constants for battery voltage calculation
const float R1 = 10000.0; // Resistance in ohms
const float R2 = 68000.0; // Resistance in ohms
const float ADC_REF_VOLTAGE = 3.3; // ESP32 ADC reference voltage
const int ADC_RESOLUTION = 4095; // 12-bit ADC resolution

// Sensor settings
Adafruit_BME280 bme;

// Deep sleep interval
const uint64_t SLEEP_DURATION = 60000000; // 60 seconds in microseconds

void setup() {
  Serial.begin(115200);

  // Initialize I2C for BME280
  Wire.begin(SDA_PIN, SCL_PIN);

  // Initialize BME280
  if (!bme.begin(0x76, &Wire)) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1);
  }

  // Configure ADC for voltage measurement
  analogReadResolution(12); // 12-bit ADC resolution
  analogSetAttenuation(ADC_11db); // Allows measurement up to ~3.6V
}

void loop() {
  // Read sensor data
  float temperature = bme.readTemperature();
  float humidity = bme.readHumidity();
  float pressure = bme.readPressure() / 100.0F; // Convert to hPa

  // Measure battery voltage
  int adcRaw = analogRead(ADC_PIN);
  float adcVoltage = (adcRaw * ADC_REF_VOLTAGE) / ADC_RESOLUTION;
  float batteryVoltage = adcVoltage * ((R1 + R2) / R2);
  float batteryPercentage = calculateBatteryPercentage(batteryVoltage);

  // Print data to Serial Monitor
  Serial.printf("Temperature: %.2f °C, Humidity: %.2f %%, Pressure: %.2f hPa\n",
                temperature, humidity, pressure);
  Serial.printf("Battery Voltage: %.2f V, Battery Percentage: %.2f %%\n",
                batteryVoltage, batteryPercentage);

  // Send data to ThingSpeak
  if (connectToWiFi()) {
    sendToThingSpeak(temperature, humidity, pressure, batteryVoltage, batteryPercentage);
    WiFi.disconnect(true);
    Serial.println("Wi-Fi disconnected.");
  } else {
    Serial.println("Failed to connect to Wi-Fi, skipping upload.");
  }

  // Enter deep sleep
  enterDeepSleep();
}

// Function to connect to Wi-Fi
bool connectToWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000) { // 10-second timeout
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  return WiFi.status() == WL_CONNECTED;
}

// Function to send data to ThingSpeak
void sendToThingSpeak(float temperature, float humidity, float pressure, float batteryVoltage, float batteryPercentage) {
  HTTPClient http;
  String url = String("http://") + server + "/update?api_key=" + apiKey +
               "&field1=" + String(temperature) +
               "&field2=" + String(humidity) +
               "&field3=" + String(pressure) +
               "&field4=" + String(batteryVoltage) +
               "&field5=" + String(batteryPercentage);

  http.begin(url);
  int httpCode = http.GET();

  if (httpCode > 0) {
    Serial.println("Data sent to ThingSpeak successfully.");
  } else {
    Serial.print("Failed to send data to ThingSpeak. Error: ");
    Serial.println(http.errorToString(httpCode));
  }

  http.end(); // Ensure cleanup 
}

// Function to calculate battery percentage
float calculateBatteryPercentage(float voltage) {
  const float MAX_VOLTAGE = 8.4; // Fully charged voltage for 2S Li-ion
  const float MIN_VOLTAGE = 6.0; // Minimum safe voltage for 2S Li-ion
  if (voltage >= MAX_VOLTAGE) return 100.0;
  if (voltage <= MIN_VOLTAGE) return 0.0;
  return ((voltage - MIN_VOLTAGE) / (MAX_VOLTAGE - MIN_VOLTAGE)) * 100.0;
}

// Function to enter deep sleep
void enterDeepSleep() {
  Serial.println("Entering deep sleep for 60 seconds...");
  esp_sleep_enable_timer_wakeup(SLEEP_DURATION);
  esp_deep_sleep_start();
}
