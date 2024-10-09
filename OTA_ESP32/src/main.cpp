#include <Arduino.h>
#include <otadrive_esp.h>
#include <WiFi.h>

#define LED_PIN 2

void setup() {
  // Inisialisasi serial monitor
  Serial.begin(115200);
  // Inisialisasi pin LED sebagai output
  pinMode(LED_PIN, OUTPUT);
  // Inisialisasi OTAdrive
  OTADRIVE.setInfo("ea7d2d48-f17a-4ac1-82ae-f7b2999ef231", "1.1.5");
  WiFi.begin("DESKTOP-R8HB7UM 3514", "12345678");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Terhubung ke WiFi");
  OTADRIVE.updateFirmware();
}

void loop() {
  // Blink LED
  digitalWrite(LED_PIN, HIGH);
  delay(100);
  digitalWrite(LED_PIN, LOW);
  delay(100);
  // Tampilkan pesan di serial monitor
  Serial.println("OTA Update");
  Serial.println("Version : 1.1.5");
  Serial.println("LED berkedip versi 1.1.5");
}