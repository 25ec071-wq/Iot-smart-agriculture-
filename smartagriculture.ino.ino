#define BLYNK_TEMPLATE_ID "YOUR_TEMPLATE_ID"
#define BLYNK_TEMPLATE_NAME "Smart Agriculture"
#define BLYNK_AUTH_TOKEN "YOUR_AUTH_TOKEN"

#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <DHT.h>

// Wi-Fi credentials
char ssid[] = "YOUR_WIFI_NAME";
char pass[] = "YOUR_WIFI_PASSWORD";

// Pin configuration
#define SOIL_PIN 34
#define DHT_PIN 4
#define RELAY_PIN 26

// DHT sensor configuration
#define DHT_TYPE DHT11
DHT dht(DHT_PIN, DHT_TYPE);

BlynkTimer timer;

// Calibrate these values for your soil sensor
const int DRY_VALUE = 3000;
const int WET_VALUE = 1300;

// Irrigation thresholds
const int PUMP_ON_THRESHOLD = 30;
const int PUMP_OFF_THRESHOLD = 45;

// Set according to your relay module
const bool RELAY_ACTIVE_LOW = true;

bool pumpOn = false;

int soilRaw = 0;
int soilMoisture = 0;

float temperature = 0;
float humidity = 0;

// Control water pump
void setPump(bool state) {
  pumpOn = state;

  if (RELAY_ACTIVE_LOW) {
    digitalWrite(RELAY_PIN, state ? LOW : HIGH);
  } else {
    digitalWrite(RELAY_PIN, state ? HIGH : LOW);
  }

  Serial.print("Water Pump: ");
  Serial.println(state ? "ON" : "OFF");

  if (Blynk.connected()) {
    Blynk.virtualWrite(V3, state ? 1 : 0);
  }
}

// Read soil moisture and control irrigation
void readSoilMoisture() {
  long total = 0;

  for (int i = 0; i < 10; i++) {
    total += analogRead(SOIL_PIN);
    delay(5);
  }

  soilRaw = total / 10;

  soilMoisture = map(
    soilRaw,
    DRY_VALUE,
    WET_VALUE,
    0,
    100
  );

  soilMoisture = constrain(soilMoisture, 0, 100);

  Serial.print("Raw Soil Value: ");
  Serial.print(soilRaw);

  Serial.print(" | Soil Moisture: ");
  Serial.print(soilMoisture);
  Serial.println("%");

  // Automatic irrigation with hysteresis
  if (!pumpOn && soilMoisture < PUMP_ON_THRESHOLD) {
    setPump(true);
  } 
  else if (pumpOn && soilMoisture >= PUMP_OFF_THRESHOLD) {
    setPump(false);
  }

  if (Blynk.connected()) {
    Blynk.virtualWrite(V0, soilMoisture);
  }
}

// Read temperature and humidity
void readTemperatureHumidity() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();

  if (isnan(t) || isnan(h)) {
    Serial.println("DHT11 sensor reading failed!");
    return;
  }

  temperature = t;
  humidity = h;

  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println(" C");

  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.println("%");

  if (Blynk.connected()) {
    Blynk.virtualWrite(V1, temperature);
    Blynk.virtualWrite(V2, humidity);
  }
}

// Update all sensors
void updateSensors() {
  readSoilMoisture();
  readTemperatureHumidity();
}

void setup() {
  Serial.begin(115200);

  // Set relay output and initialize pump OFF
  digitalWrite(RELAY_PIN, RELAY_ACTIVE_LOW ? HIGH : LOW);
  pinMode(RELAY_PIN, OUTPUT);

  setPump(false);

  // Configure ADC
  analogReadResolution(12);

  // Initialize temperature and humidity sensor
  dht.begin();

  Serial.println("IoT Smart Agriculture System Starting...");

  // Connect ESP32 to Wi-Fi and Blynk
  Blynk.begin(
    BLYNK_AUTH_TOKEN,
    ssid,
    pass
  );

  // Read sensors every 3 seconds
  timer.setInterval(3000L, updateSensors);

  Serial.println("Smart Agriculture System Ready!");
}

void loop() {
  Blynk.run();
  timer.run();
}