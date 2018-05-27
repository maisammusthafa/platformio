// Program to display DHT22 sensor and HC-SR04 ultrasonic
// sensor data on an OLED display

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <Fonts/FreeSans9pt7b.h>
#include <DHT.h>

#define OLED_ADDR 0x3C
#define DHTPIN 2
#define DHTTYPE DHT22
#define TRIGPIN 5
#define ECHOPIN 6

Adafruit_SSD1306 display(-1);
DHT dht(DHTPIN, DHTTYPE);

float hum;
float temp;
long time;
int dist;

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
  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  display.clearDisplay();
  display.display();

  display.setFont(&FreeSans9pt7b);
  display.setTextSize(1);
  display.setTextColor(WHITE);

  pinMode(TRIGPIN, OUTPUT);
  pinMode(ECHOPIN, INPUT);

  dht.begin();
}

void loop() {
  hum = dht.readHumidity();
  temp = dht.readTemperature();

  digitalWrite(TRIGPIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIGPIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGPIN, LOW);

  time = pulseIn(ECHOPIN, HIGH);
  dist = (time * 0.034) / 2;

  String temp_str = formatData("T: ", temp, " *C");
  String hum_str = formatData("H: ", hum, "%");
  String dist_str = String("D: ") + String(dist) + String(" cm");
  
  display.setCursor(0, 14);
  display.print(temp_str);

  display.setCursor(0, 38);
  display.print(hum_str);

  display.setCursor(0, 61);
  display.print(dist_str);

  display.display();
  delay(500);
  display.clearDisplay();
}
