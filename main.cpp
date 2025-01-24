#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <Wire.h>
#include <ArduinoJson.h>
#include <UniversalTelegramBot.h>
#include <LiquidCrystal_I2C.h>

const int buzzerPinP = 25;
const int ledRed1 = 33;
const int ledRed2 = 32;
const int ledGreen = 26; // Pin untuk LED hijau
const int pirPin = 19; // Pin untuk sensor PIR
volatile bool motionDetected = false; // Variabel volatile untuk interrupt

// Inisialisasi LCD (ganti 0x27 jika alamat I2C berbeda)
const int col = 16; // Jumlah kolom pada LCD
const int row = 2;  // Jumlah baris pada LCD
LiquidCrystal_I2C lcd(0x27, col, row);

// WiFi
const char* ssid = "Your WiFi SSID";// Nama WiFi Anda
const char* pass = "Your WiFi Password";// Password WiFi Anda

// Telegram
#define BOTtoken "Your Bot Token, get it from BotFather"// Token Bot Anda
#define CHAT_ID "Your Chat ID, get it from @userinfobot"//  ID Chat Anda
WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

void IRAM_ATTR detectsMovement() {
  motionDetected = true;
}

void setup() {
  // Inisialisasi
  Serial.begin(115200);
  lcd.init();
  lcd.backlight();

  // Mengatur pin
  pinMode(buzzerPinP, OUTPUT);
  pinMode(pirPin, INPUT);
  pinMode(ledRed1, OUTPUT);
  pinMode(ledRed2, OUTPUT);
  pinMode(ledGreen, OUTPUT); // Inisialisasi pin LED hijau

  // Telegram
  client.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Tambahkan root certificate untuk Telegram API
  attachInterrupt(digitalPinToInterrupt(pirPin), detectsMovement, RISING);

  // Menampilkan pesan awal di LCD
  lcd.setCursor(0, 0);
  lcd.print("Connecting...");
  lcd.setCursor(0, 1);
  lcd.print("to WiFi");

  // Memulai koneksi WiFi
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting...");
    lcd.setCursor(0, 1); // Menampilkan titik-titik progresif
    static int dotCount = 0;
    dotCount = (dotCount + 1) % 4; // Jumlah titik maksimal adalah 3
    String dots = "";
    for (int i = 0; i < dotCount; i++) {
      dots += ".";
    }
    lcd.print("            "); // Clear baris kedua
    lcd.setCursor(0, 1);
    lcd.print(dots);
  }

  // Jika koneksi berhasil
  Serial.println("WiFi Connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi Connected");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP()); // Tampilkan IP Address
  delay(3000); // Tahan tampilan selama 3 detik
  lcd.clear();

  bot.sendMessage(CHAT_ID, "Sistem Telah Aktif", "");
}

void sirene() {
  for (int i = 0; i < 8; i++) { // Perulangan 8 kali untuk 8 kedipan
    digitalWrite(buzzerPinP, HIGH); 
    digitalWrite(ledRed1, HIGH);  // Nyalakan LED Red1
    digitalWrite(ledRed2, LOW);   // Matikan LED Red2
    delay(250);                    
    digitalWrite(ledRed1, LOW);   // Matikan LED Red1
    digitalWrite(ledRed2, HIGH);  // Nyalakan LED Red2
    delay(250);
  }
  digitalWrite(ledRed1, LOW);
  digitalWrite(ledRed2, LOW);
  digitalWrite(buzzerPinP, LOW);
}

void checkTelegramMessages() {
  static unsigned long lastTimeBotRan = 0;  // Waktu terakhir bot berjalan
  const int botRequestDelay = 1000; // Interval polling bot (1 detik)

  if (millis() - lastTimeBotRan > botRequestDelay) {
    lastTimeBotRan = millis();
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1); // Ambil pesan baru

    while (numNewMessages) {
      for (int i = 0; i < numNewMessages; i++) {
        String text = bot.messages[i].text; // Ambil teks pesan
        String chat_id = String(bot.messages[i].chat_id); // Ambil chat ID

        if (text == "/status") {
          digitalWrite(ledGreen, HIGH); // Nyalakan LED hijau
          bot.sendMessage(chat_id, "Sistem masih berjalan.", ""); // Kirim pesan status
          delay(5000); // Tunggu selama 5 detik
          digitalWrite(ledGreen, LOW); // Matikan LED hijau
        }
      }
      numNewMessages = bot.getUpdates(bot.last_message_received + 1); // Ambil pesan baru lagi
    }
  }
}

void loop() {
  // Jika gerakan terdeteksi
  if (motionDetected) {
    motionDetected = false; // Reset status gerakan

    // Kirim pesan ke Telegram
    bot.sendMessage(CHAT_ID, "Gerakan Terdeteksi!!!", "");
    Serial.println("Motion detected!");

    // Tampilkan pesan di LCD
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Motion detected!");
    delay(1000);
    lcd.clear();
    lcd.setCursor(4, 0);
    lcd.print("AREA INI");
    lcd.setCursor(2, 1);
    lcd.print("DIAWASI CCTV");
    sirene();
    delay(8000); // Tampilkan pesan selama 5 detik
    lcd.clear();
  }

  // Jika tidak ada gerakan
  else {
    lcd.setCursor(0, 0);
    lcd.print("NOTIFICATION SENT");
    delay(2000); // Tampilkan pesan selama 2 detik
    lcd.clear();
    lcd.noBacklight();
  }
  checkTelegramMessages(); // Periksa pesan Telegram
}