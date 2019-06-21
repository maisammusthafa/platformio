// Program to log DHT22 sensor and light sensor
// data to Losant to use as an IoT device.

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Losant.h>
#include "DHTesp.h"
// #include <QuickStats.h>
#include <Wire.h>

#define HEAP_MAX 532480
#define OLED_ADDR 0x3C
#define DHTTYPE DHT22
#define DHTPIN 4
// #define LDR_PIN 32

Adafruit_SSD1306 oled(128, 64);
DHTesp DHT;

const char* WIFI_SSID = "***REMOVED***";
const char* WIFI_PASS = "***REMOVED***";

const char* LOSANT_DEVICE_ID = "***REMOVED***";
const char* LOSANT_ACCESS_KEY = "***REMOVED***";
const char* LOSANT_ACCESS_SECRET = "***REMOVED***";

// const int L_ANALOG_MIN = 0;
// const int L_ANALOG_MAX = 4095;
// const int SMOOTH_NUM = 20;
// const int SMOOTH_INIT_MS = 2000;
// const float MODE_EPSILON = 5.00;

// int smooth_index = 0;
// float smooth_values[SMOOTH_NUM];
// float smooth_total = 0;
// float analog_values[22];

int timeSinceLastRead = 0;
int counter = 1;

// QuickStats stats;
WiFiClientSecure wifiClient;
LosantDevice device(LOSANT_DEVICE_ID);

void display(int x, int y, String text, bool clear = false, bool flush = false) {
    if (clear) { oled.clearDisplay(); }
    oled.setCursor(x, y);
    oled.print(text);
    if (flush) { oled.display(); }
}

void connect() {
    printf("\n\n");
    printf("Connecting to %s", WIFI_SSID);

    display(0, 14, "Connecting to\n" + String(WIFI_SSID) + "...", true, true);

    // WiFi fix: https://github.com/esp8266/Arduino/issues/2186
    WiFi.persistent(false);
    WiFi.mode(WIFI_OFF);
    WiFi.mode(WIFI_STA);
    // WiFi.setOutputPower(0);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    unsigned long ConnectStart = millis();

    while (WiFi.status() != WL_CONNECTED) {
        if (WiFi.status() == WL_CONNECT_FAILED) {
            printf("Failed to connect to WIFI. Please verify credentials.\n");
            printf("SSID: %s", WIFI_SSID);
            printf("Password: %s\n\n", WIFI_PASS);
        }

        delay(500);
        printf("...\n");

        if (millis() - ConnectStart > 5000) {
            printf("Failed to connect to WiFi\n");
            printf("Please attempt to send updated configuration parameters.\n");
            return;
        }
    }

    printf("\n");
    printf("WiFi connected\n");
    printf("IP address: %s\n\n", WiFi.localIP().toString().c_str());

    display(0, 14, "WiFi connected\nIP: " + WiFi.localIP().toString(), true, true);
    delay(1000);

    printf("Authenticating device... ");

    HTTPClient http;
    http.begin("http://api.losant.com/auth/device");
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Accept", "application/json");

    StaticJsonDocument<200> jsonBuffer;
    JsonObject root = jsonBuffer.to<JsonObject>();
    root["deviceId"] = LOSANT_DEVICE_ID;
    root["key"] = LOSANT_ACCESS_KEY;
    root["secret"] = LOSANT_ACCESS_SECRET;
    String buffer;
    serializeJson(root, buffer);

    int httpCode = http.POST(buffer);

    if (httpCode > 0) {
            if (httpCode == HTTP_CODE_OK) {
                    printf("This device is authorized!\n");
            } else {
                printf("Failed to authorize device to Losant.\n");
                if (httpCode == 400) {
                    printf("Validation error: The device ID, access key, or access secret is not in the proper format.\n");
                } else if (httpCode == 401) {
                    printf("Invalid credentials to Losant: Please double-check the device ID, access key, and access secret.\n");
                } else {
                    printf("Unknown response from API.\n");
                }
            }
        } else {
            printf("Failed to connect to Losant API.\n");
     }

    http.end();

    printf("\n");
    printf("Connecting to Losant... ");
    display(0, 14, "Connecting to\nLosant...", true, true);

    device.connectSecure(wifiClient, LOSANT_ACCESS_KEY, LOSANT_ACCESS_SECRET);
    ConnectStart = millis();

    while (!device.connected()) {
        if (millis() - ConnectStart > 5000) {
            return;
        }
        delay(500);
        printf("...\n");
    }

    printf("Connected!\n");
    printf("This device is now ready for use!\n\n");
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

void report(double temperature, double humidity, double heat_index) {
    StaticJsonDocument<500> jsonBuffer;
    JsonObject root = jsonBuffer.to<JsonObject>();
    root["temperature"] = temperature;
    root["humidity"] = humidity;
    root["heat_index"] = heat_index;
    device.sendState(root);
}

void setup() {
    // analogSetWidth(12);

    oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
    oled.setFont(&FreeSans9pt7b);
    oled.setTextSize(1);
    oled.setTextColor(WHITE);

    Serial.begin(115200);
    Serial.setTimeout(2000);

    printf("Device Started.\n");
    printf("Running DHT...\n");

    display(0, 14, "Device started.\nRunning DHT...", true, true);

    DHT.setup(DHTPIN, DHTesp::DHT22);

    connect();

    // for (int i = 0; i < SMOOTH_NUM; i++) {
    //     smooth_values[i] = float(analogRead(LDR_PIN));
    //     smooth_total += smooth_values[i];
    //     delay(SMOOTH_INIT_MS / SMOOTH_NUM);
    // }
}

void loop() {
    bool toReconnect = false;

    if (WiFi.status() != WL_CONNECTED) {
        printf("Disconnected from WiFi\n");
        toReconnect = true;
    }

    if (!device.connected()) {
        printf("Disconnected from MQTT. Client state: %d\n", device.mqttClient.state());
        toReconnect = true;
    }

    if (toReconnect) {
        connect();
    }

    device.loop();

    // analog_values[(timeSinceLastRead / 100)] = float(analogRead(LDR_PIN));

    if (timeSinceLastRead > 3000) {
        TempAndHumidity newValues = DHT.getTempAndHumidity();
        if (DHT.getStatus() != 0) {
            printf(DHT.getStatusString());
            printf("\n");
            timeSinceLastRead = 0;
            return;
        }
        float temperature = newValues.temperature;
        float humidity = newValues.humidity;
        float heat_index = round(DHT.computeHeatIndex(temperature, humidity) * 10) / 10.0;
        float free_heap = 100 - (ESP.getFreeHeap() / float (HEAP_MAX) * 100);

        // int l_analog_value = analogRead(LDR_PIN);

        // smooth_total -= smooth_values[smooth_index];
        // smooth_values[smooth_index] = stats.mode(analog_values, 22, MODE_EPSILON);
        // smooth_total += smooth_values[smooth_index];
        // smooth_index++;

        // if (smooth_index >= SMOOTH_NUM) {
        //     smooth_index = 0;
        // }

        // int smooth_avg = smooth_total / SMOOTH_NUM;
        // float light = map(constrain(
        //     smooth_avg, L_ANALOG_MIN, L_ANALOG_MAX), L_ANALOG_MAX, L_ANALOG_MIN, 0, 1000) / 10.0;

        printf("[%06d] Temperature: %0.2f *C      Humidity: %0.2f%%      Heat Index: %0.2f%%      RAM: %0.2f%%\n",
            counter, temperature, humidity, heat_index, free_heap);

        report(temperature, humidity, heat_index);

        oled.clearDisplay();
        display(0, 14, formatData("T: ", temperature, " *C"), true, false);
        display(0, 38, formatData("H: ", humidity, "%"), false, false);
        display(0, 61, formatData("I:  ", heat_index, " *C"), false, true);

        timeSinceLastRead = 0;
        counter++;
    }

    delay(100);
    timeSinceLastRead += 100;
}