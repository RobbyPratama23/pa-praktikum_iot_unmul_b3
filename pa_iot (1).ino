#define BLYNK_TEMPLATE_ID "TMPL6uKaXUc0_"
#define BLYNK_TEMPLATE_NAME "Smart Pet Care"
#define BLYNK_AUTH_TOKEN "TJ-PoUtmlxM2lg-11SKgLDJ0O-PQr6WZ"
#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <BlynkSimpleEsp32.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <ESP32Servo.h>

// WiFi
char ssid[] = "bebek goreng";
char pass[] = "123bebek";

// Telegram
#define BOT_TOKEN "8509132064:AAESKrRlHa5tg9zcyB4YbZfjoOqMWISW8kg"
#define CHAT_ID "5569342487"

WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);

// Pin Komponen
#define DHTPIN 2
#define DHTTYPE DHT11
#define WATER_PIN 3
#define SERVO_PIN 8

DHT dht(DHTPIN, DHTTYPE);
Servo myServo;
BlynkTimer timer;

bool tempNotified = false;
bool waterNotified = false;

void feedPet() {
  Serial.println("Memberi makan...");
  myServo.write(90);
  
  unsigned long startDelay = millis();
  while (millis() - startDelay < 2000) {
    Blynk.run(); 
  }
  
  myServo.write(0);
  Serial.println("Selesai.");
}

BLYNK_WRITE(V4) {
  if (param.asInt() == 1) {
    feedPet();
    Blynk.virtualWrite(V4, 0);
    bot.sendMessage(CHAT_ID, "✅ Pakan diberikan via Blynk", "");
  }
}

void sendSensorData() {
  // Suhu
  float t = dht.readTemperature();
  if (!isnan(t)) {
    Blynk.virtualWrite(V1, t);
    if (t < 20 || t > 26) {
      if (!tempNotified) {
        Blynk.logEvent("peringatan_suhu", "Suhu di luar batas!");
        bot.sendMessage(CHAT_ID, "⚠️ PERINGATAN SUHU: " + String(t) + "°C", "");
        tempNotified = true;
      }
    } else { tempNotified = false; }
  }

  // Water Level
  int waterRaw = analogRead(WATER_PIN);
  int waterPercent = map(waterRaw, 0, 4095, 0, 100);
  waterPercent = constrain(waterPercent, 0, 100);

  Blynk.virtualWrite(V2, waterPercent);
  if (waterPercent <= 10) {
    if (!waterNotified) {
      Blynk.logEvent("peringatan_air", "Air hampir habis!");
      bot.sendMessage(CHAT_ID, "💧 PERINGATAN AIR: " + String(waterPercent) + "%", "");
      waterNotified = true;
    }
  } else { waterNotified = false; }
}

void handleTelegramMessages() {
  int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
  while (numNewMessages) {
    for (int i = 0; i < numNewMessages; i++) {
      String chat_id = bot.messages[i].chat_id;
      if (chat_id != CHAT_ID) continue;

      String text = bot.messages[i].text;
      if (text == "/status") {
        float t = dht.readTemperature();
        int w = map(analogRead(WATER_PIN), 0, 4095, 0, 100);
        bot.sendMessage(CHAT_ID, "🌡 Suhu: " + String(t) + "°C\n💧 Air: " + String(constrain(w,0,100)) + "%", "");
      } 
      else if (text == "/makan") {
        feedPet();
        bot.sendMessage(CHAT_ID, "✅ Pakan diberikan!", "");
      }
    }
    numNewMessages = bot.getUpdates(bot.last_message_received + 1);
  }
}

void setup() {
  Serial.begin(115200);
  
  // Koneksi WiFi via Blynk
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // Bypass SSL Telegram untuk kestabilan ESP32-C3
  secured_client.setInsecure();

  dht.begin();
  
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  
  myServo.setPeriodHertz(50);
  if (!myServo.attach(SERVO_PIN, 500, 2400)) {
    Serial.println("Gagal attach Servo ke Pin D8");
  }
  
  myServo.write(0); 

  timer.setInterval(3000L, sendSensorData);
  timer.setInterval(1000L, handleTelegramMessages);
}

void loop() {
  Blynk.run();
  timer.run();
}