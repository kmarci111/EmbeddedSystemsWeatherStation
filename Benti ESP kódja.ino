#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>  // JSON feldolgozó könyvtár


// WiFi beállítások
const char *ssid = "BATTLESTATION_MINI";
const char *password = "nemmondommeg";

// ThingSpeak beállítások
const String apiKey = "VPIB6GOEEX0TLHT4";
const String channelID = "2759654";
const String fieldTemperature = "field1";
const String fieldHumidity = "field2";
const String fieldPressure = "field3";
const String fieldBatteryVoltage = "field4";
const String fieldBatteryPercent = "field5";

// Gomb pin
#define BUTTON_PIN 23

// OLED I2C cím
#define OLED_I2C_ADDR 0x3C

// OLED kijelző beállítások
Adafruit_SSD1306 display(128, 64, &Wire, -1);


// Állapotok és változók
bool isEnglish = false;  // Alapértelmezett magyar
bool isImperial = false; // Alapértelmezett SI mértékegységek
unsigned long lastFetchTime = 0;
String language = "HU";  // Alapértelmezett nyelv: magyar

// Adatok változók
float temperature = 0.0;
float humidity = 0.0;
float pressure = 0.0;
float batteryVoltage = 0.0;
float batteryPercent = 0.0;

// Az adatokat egy listában tároljuk
String data[] = {
  "Temperature: ",
  "Humidity: ",
  "Pressure: ",
  "Battery Voltage: ",
  "Battery Percent: "
};

int currentIndex = 0;  // Aktuális adat index, amit megjelenítünk

// Gomb nyomás érzékeléséhez használt változók
bool lastButtonState = HIGH;  // Az előző állapot, kezdetben HIGH (nem nyomott)
bool buttonPressed = false;

void setup() {
  Serial.begin(115200);

  // WiFi csatlakozás
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("WiFi connected");

  // OLED inicializálás
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDR)) {
    Serial.println(F("OLED nem található!"));
    while (1);  // Végtelen ciklus, ha nincs OLED
  }

  // Töröljük a kijelzőt, hogy ne jelenjen meg az Adafruit logó
  display.clearDisplay();
  display.display();
  delay(2000);  // OLED bekapcsolás ideje

  // Gomb konfigurálás
  pinMode(BUTTON_PIN, INPUT_PULLUP);
}

void loop() {
  bool currentButtonState = digitalRead(BUTTON_PIN);

  // Ha a gomb állapota változik és a gomb lenyomásra került
  if (lastButtonState == HIGH && currentButtonState == LOW) {
    buttonPressed = true;
  }

  if (buttonPressed) {
    // Nyelv és mértékegység váltás azonnal
    isEnglish = !isEnglish;
    isImperial = !isImperial;
    language = isEnglish ? "EN" : "HU";
    updateDisplay();  // Frissíti a kijelzőt a változtatásoknak megfelelően
    buttonPressed = false;  // A váltás után állítsuk vissza a nyomva tartást
  }

  // Ha a gombot elengedtük, állítsuk vissza az előző állapotot
  if (currentButtonState == HIGH) {
    lastButtonState = HIGH;
  } else {
    lastButtonState = LOW;
  }

  // Adatok letöltése és kijelző frissítése
  if (millis() - lastFetchTime > 5000) {
    fetchData();
    updateDisplay();
    lastFetchTime = millis();
  }
}

void fetchData() {
  String url = "http://api.thingspeak.com/channels/" + channelID + "/feeds.json?api_key=" + apiKey + "&results=1";
  HTTPClient http;
  http.begin(url);
  int httpCode = http.GET();
  
  if (httpCode > 0) {
    String payload = http.getString();
    Serial.println(payload);  // Debugging: JSON válasz kinyomtatása
    parseJSON(payload);  // JSON feldolgozás
  } else {
    Serial.println("HTTP GET failed");
  }
  http.end();
}

void parseJSON(String json) {
  // JSON feldolgozása ArduinoJson könyvtárral
  StaticJsonDocument<1024> doc;
  DeserializationError error = deserializeJson(doc, json);
  if (error) {
    Serial.println("JSON parsing failed!");
    return;
  }

  // Adatok kinyerése a JSON-ból
  temperature = doc["feeds"][0]["field1"].as<float>();
  humidity = doc["feeds"][0]["field2"].as<float>();
  pressure = doc["feeds"][0]["field3"].as<float>();
  batteryVoltage = doc["feeds"][0]["field4"].as<float>();
  batteryPercent = doc["feeds"][0]["field5"].as<float>();
}

void updateDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Az aktuális adat megjelenítése
  display.setCursor(0, 0);
  if (isEnglish) {
    display.print("Temperature: ");
    display.print(isImperial ? temperature * 9 / 5 + 32 : temperature);  // Celsius to Fahrenheit
    display.print(" ");
    display.print((char)247);
    display.print(isImperial ? "F" : "C");
    display.setCursor(0, 10);
    display.print("Humidity: ");
    display.print(humidity);
    display.print(" %");
    display.setCursor(0, 20);
    display.print("Pressure: ");
    display.print(pressure);
    display.print(isImperial ? " hPa" : " hPa");
    display.setCursor(0, 30);
    display.print("Battery: ");
    display.print(batteryVoltage);
    display.print(" V");
    display.setCursor(0, 40);
    display.print("Battery %: ");
    display.print(batteryPercent);
    display.print(" %");
  } else {
    display.print("Homerseklet: ");
    display.print(isImperial ? temperature * 9 / 5 + 32 : temperature);  // Celsius to Fahrenheit
    display.print(" ");
    display.print((char)247);
    display.print(isImperial ? "F" : "C");
    display.setCursor(0, 10);
    display.print("Paratartalom: ");
    display.print(humidity);
    display.print(" %");
    display.setCursor(0, 20);
    display.print("Nyomas: ");
    display.print(pressure);
    display.print(isImperial ? " hPa" : " hPa");
    display.setCursor(0, 30);
    display.print("Akkufeszultseg: ");
    display.print(batteryVoltage);
    display.print(" V");
    display.setCursor(0, 40);
    display.print("Akkumulator %: ");
    display.print(batteryPercent);
    display.print(" %");
  }
  
  display.display();
}
