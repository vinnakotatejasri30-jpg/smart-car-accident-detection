#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>
#include <TinyGPS++.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

String botToken = "YOUR_TELEGRAM_BOT_TOKEN";
String chatID = "YOUR_TELEGRAM_CHAT_ID";
WiFiClientSecure client;
UniversalTelegramBot bot(botToken, client);

// GPS
TinyGPSPlus gps;
HardwareSerial gpsSerial(2);

// Pins
#define TRIG_PIN 5
#define ECHO_PIN 18
#define IR_PIN 19
#define BUZZER 2
#define LED 4
#define ALCOHOL_PIN 34

LiquidCrystal_I2C lcd(0x27, 16, 2);
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);

long duration;
float distance;

bool alertSent = false;

void setup() {
  Serial.begin(115200);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(IR_PIN, INPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(LED, OUTPUT);

  lcd.init();
  lcd.backlight();

  // WiFi
  WiFi.begin(ssid, password);
  lcd.print("Connecting WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  lcd.clear();
  lcd.print("WiFi Connected");

  client.setInsecure();

  // GPS
  gpsSerial.begin(9600, SERIAL_8N1, 16, 17);

  // ADXL
  if (!accel.begin()) {
    lcd.print("ADXL ERROR");
    while (1);
  }

  delay(2000);
  lcd.clear();
}

void loop() {

  // -------- GPS --------
  while (gpsSerial.available()) {
    gps.encode(gpsSerial.read());
  }

  float lat = gps.location.lat();
  float lon = gps.location.lng();

  // -------- Ultrasonic --------
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  duration = pulseIn(ECHO_PIN, HIGH, 30000); // FIXED timeout
  distance = duration * 0.034 / 2;

  // -------- IR --------
  int irState = digitalRead(IR_PIN);

  // -------- Alcohol --------
  int alcoholValue = analogRead(ALCOHOL_PIN);

  // -------- ADXL --------
  sensors_event_t event;
  accel.getEvent(&event);

  float totalAccel = sqrt(
    event.acceleration.x * event.acceleration.x +
    event.acceleration.y * event.acceleration.y +
    event.acceleration.z * event.acceleration.z
  );

  bool accident = (totalAccel > 20);

  // 🚨 Buzzer + LED only for accident
  if (accident) {
    digitalWrite(BUZZER, HIGH);
    digitalWrite(LED, HIGH);
  } else {
    digitalWrite(BUZZER, LOW);
    digitalWrite(LED, LOW);
    alertSent = false;
  }

  // 📲 Telegram
  if (accident && !alertSent && gps.location.isValid()) {

    String message = "🚨 ACCIDENT DETECTED!\n";
    message += "Alcohol: " + String(alcoholValue) + "\n";
    message += "Location:\n";
    message += "https://maps.google.com/?q=";
    message += String(lat, 6) + "," + String(lon, 6);

    bot.sendMessage(chatID, message, "");

    alertSent = true;
  }

  // -------- DISPLAY --------

  // Line 1: Distance
  lcd.setCursor(0, 0);
  lcd.print("D:");
  lcd.print(distance);
  lcd.print("cm   ");

  // Line 2: IR + Alcohol + Accident
  lcd.setCursor(0, 1);

  lcd.print("I:");
  lcd.print(irState == LOW ? "Y" : "N");

  lcd.print(" A:");
  lcd.print(alcoholValue / 100);

  if (accident) {
    lcd.print(" ACC");
  } else {
    lcd.print(" OK ");
  }

  delay(500);
}
