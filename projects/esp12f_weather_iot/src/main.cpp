// Program to log DHT22 sensor and KG003 soil moisture sensor
// data to Losant to use as an IoT device.

#include "DHT.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Losant.h>
#include <QuickStats.h>
#include <Wire.h>

#define HEAP_MAX 81920
#define OLED_ADDR 0x3C
#define DHTPIN 14
#define DHTTYPE DHT22

Adafruit_SSD1306 oled(-1);
DHT dht(DHTPIN, DHTTYPE);

const char* WIFI_SSID = "***REMOVED***";
const char* WIFI_PASS = "***REMOVED***";

const char* LOSANT_DEVICE_ID = "***REMOVED***";
const char* LOSANT_ACCESS_KEY = "***REMOVED***";
const char* LOSANT_ACCESS_SECRET = "***REMOVED***";

const int M_ANALOG_MIN = 10;
const int M_ANALOG_MAX = 964;
const int SMOOTH_NUM = 100;
const int SMOOTH_INIT_MS = 2000;
const float MODE_EPSILON = 5.00;

int smooth_index = 0;
float smooth_values[SMOOTH_NUM];
float smooth_total = 0;
float analog_values[22];

int timeSinceLastRead = 0;

QuickStats stats;
WiFiClientSecure wifiClient;
LosantDevice device(LOSANT_DEVICE_ID);

void display(int x, int y, String text, bool clear = false, bool flush = false) {
  if (clear) { oled.clearDisplay(); }
  oled.setCursor(x, y);
  oled.print(text);
  if (flush) { oled.display(); }
}

void connect() {
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  display(0, 14, "Connecting to\n" + String(WIFI_SSID) + "...", true, true);

  // WiFi fix: https://github.com/esp8266/Arduino/issues/2186
  WiFi.persistent(false);
  WiFi.mode(WIFI_OFF);
  WiFi.mode(WIFI_STA);
  WiFi.setOutputPower(0);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  unsigned long ConnectStart = millis();

  while (WiFi.status() != WL_CONNECTED) {
    if (WiFi.status() == WL_CONNECT_FAILED) {
      Serial.println("Failed to connect to WIFI. Please verify credentials: ");
      Serial.println();
      Serial.print("SSID: ");
      Serial.println(WIFI_SSID);
      Serial.print("Password: ");
      Serial.println(WIFI_PASS);
      Serial.println();
    }

    delay(500);
    Serial.println("...");

    if(millis() - ConnectStart > 5000) {
      Serial.println("Failed to connect to WiFi");
      Serial.println("Please attempt to send updated configuration parameters.");
      return;
    }
  }

  Serial.println();
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  display(0, 14, "WiFi connected\nIP: " + WiFi.localIP().toString(), true, true);
  delay(1000);

  Serial.print("Authenticating device... ");

  HTTPClient http;
  http.begin("http://api.losant.com/auth/device");
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Accept", "application/json");

  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["deviceId"] = LOSANT_DEVICE_ID;
  root["key"] = LOSANT_ACCESS_KEY;
  root["secret"] = LOSANT_ACCESS_SECRET;
  String buffer;
  root.printTo(buffer);

  int httpCode = http.POST(buffer);

  if(httpCode > 0) {
      if(httpCode == HTTP_CODE_OK) {
          Serial.println("This device is authorized!");
      } else {
        Serial.println("Failed to authorize device to Losant.");
        if(httpCode == 400) {
          Serial.println("Validation error: The device ID, access key, or access secret is not in the proper format.");
        } else if(httpCode == 401) {
          Serial.println("Invalid credentials to Losant: Please double-check the device ID, access key, and access secret.");
        } else {
           Serial.println("Unknown response from API");
        }
      }
    } else {
        Serial.println("Failed to connect to Losant API.");
   }

  http.end();

  Serial.println();
  Serial.print("Connecting to Losant... ");
  display(0, 14, "Connecting to\nLosant...", true, true);

  device.connectSecure(wifiClient, LOSANT_ACCESS_KEY, LOSANT_ACCESS_SECRET);
  ConnectStart = millis();

  while(!device.connected()) {
    if(millis() - ConnectStart > 5000) {
      return;
    }
    delay(500);
    Serial.print("\n...");
  }

  Serial.println("Connected!");
  Serial.println();
  Serial.println("This device is now ready for use!");
  display(0, 14, "Connected!\nDevice is ready for use.", true, true);
}

String formatData(String prefix, float value, String suffix, int len = 6, int minWidth = 4, int precision = 2) {
  char chars[len];
  String result = prefix;

  dtostrf(value, minWidth, precision, chars);

  for (int i = 0; i < sizeof(chars); i++) {
      result += chars[i];
  }

  result += suffix;
  return result;
}

void setup() {
  oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  oled.setFont(&FreeSans9pt7b);
  oled.setTextSize(1);
  oled.setTextColor(WHITE);

  Serial.begin(115200);
  Serial.setTimeout(2000);

  while(!Serial) { }

  Serial.println("Device Started.");
  Serial.println("Running DHT...");

  display(0, 14, "Device started.\nRunning DHT...", true, true);

  connect();

  for (int i = 0; i < SMOOTH_NUM; i++) {
    smooth_values[i] = float(analogRead(A0));
    smooth_total += smooth_values[i];
    delay(SMOOTH_INIT_MS / SMOOTH_NUM);
  }
}

void report(double temperature, double humidity, double moisture, double analog) {
  StaticJsonBuffer<500> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["temperature"] = temperature;
  root["humidity"] = humidity;
  root["moisture"] = moisture;
  root["analog"] = analog;
  device.sendState(root);
  Serial.println("\tReported!");
}

void loop() {
  bool toReconnect = false;

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Disconnected from WiFi");
    toReconnect = true;
  }

  if (!device.connected()) {
    Serial.println("Disconnected from MQTT");
    Serial.println(device.mqttClient.state());
    toReconnect = true;
  }

  if (toReconnect) {
    connect();
  }

  device.loop();

  analog_values[(timeSinceLastRead / 100)] = float(analogRead(A0));

  if(timeSinceLastRead > 2000) {
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();

    if (isnan(temperature) || isnan(humidity)) {
      Serial.println("Failed to read data from DHT sensor!");
      timeSinceLastRead = 0;
      return;
    }

    smooth_total -= smooth_values[smooth_index];
    smooth_values[smooth_index] = stats.mode(analog_values, 22, MODE_EPSILON);
    smooth_total += smooth_values[smooth_index];
    smooth_index++;

    if (smooth_index >= SMOOTH_NUM) {
      smooth_index = 0;
    }

    int smooth_avg = smooth_total / SMOOTH_NUM;
    float moisture = map(constrain(
      smooth_avg, M_ANALOG_MIN, M_ANALOG_MAX), M_ANALOG_MAX, M_ANALOG_MIN, 0, 1000) / 10.0;

    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.print(" *C\t");
    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.print("%\t");
    Serial.print("Moisture: ");
    Serial.print(moisture);
    Serial.print("%, ");
    Serial.print(smooth_avg);
    Serial.print(", ");
    Serial.print(analogRead(A0));
    Serial.print("\tRAM: ");
    Serial.print(100 - (ESP.getFreeHeap() / float (HEAP_MAX) * 100));
    Serial.print("%");
    report(temperature, humidity, moisture, 100.0 - (smooth_avg / 10.0));

    oled.clearDisplay();
    display(0, 14, formatData("T: ", temperature, " *C"), true, false);
    display(0, 38, formatData("H: ", humidity, "%"), false, false);
    display(0, 61, formatData("M: ", moisture, "%"), false, true);

    timeSinceLastRead = 0;
  }

  delay(100);
  timeSinceLastRead += 100;
}