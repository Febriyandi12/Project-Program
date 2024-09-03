#include <SPI.h>
#include <LoRa.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <PubSubClient.h>

// LoRa pins
const int csPin = 5;   // Chip select for LoRa
const int resetPin = 15; // Reset for LoRa
const int irqPin = 33; // Interrupt for LoRa

// WiFi and MQTT settings
const char* ssid = "Rakyat_Jelata";
const char* password = "passwordsalah";
const char* mqtt_server = "broker.mqtt-dashboard.com";
const int mqtt_port = 1883;
const char* mqtt_topic = "loRa/data";

WiFiClient espClient;
PubSubClient client(espClient);

// Slot TDMA
unsigned long lastTime = 0;
const unsigned long interval = 10000; // 10 detik
const unsigned long responseTime = 15000; // 15 detik delay untuk menunggu balasan

void sendToMQTT(const String& payload) {
  client.publish(mqtt_topic, payload.c_str());
  Serial.print("Sent to MQTT: ");
  Serial.println(payload);
}

void reconnectMQTT() {
  // Loop hingga berhasil terhubung
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32Master")) {
      Serial.println("connected");
      // Subscribe to topics if needed
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  
  // Initialize LoRa
  LoRa.setPins(csPin, resetPin, irqPin);
  if (!LoRa.begin(433E6)) { // Initialize with frequency 433 MHz
    Serial.println("Starting LoRa failed!");
    while (1);
  }

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to WiFi");

  // Connect to MQTT
  client.setServer(mqtt_server, mqtt_port);
  reconnectMQTT();
}

void loop() {
  unsigned long currentTime = millis();

  if (currentTime - lastTime >= interval) {
    lastTime = currentTime;

    // Mengirimkan MAC address untuk slave
    int slaveId = (currentTime / interval) % 8; // Mengatur slave ID dari 0 hingga 7
    String macAddress = "0x00" + String(slaveId, HEX); // Format MAC address

    LoRa.beginPacket();
    LoRa.print(macAddress);
    LoRa.endPacket();

    Serial.print("Sent MAC Address: ");
    Serial.println(macAddress);

    // Menunggu selama 15 detik untuk menerima balasan dari slave
    unsigned long waitStart = millis();
    String receivedData = "";
    while (millis() - waitStart < responseTime) {
      int packetSize = LoRa.parsePacket();
      if (packetSize) {
        while (LoRa.available()) {
          receivedData += (char)LoRa.read();
        }

        // Menampilkan data yang diterima dari slave
        Serial.print("Received from Slave: ");
        Serial.println(receivedData);

        // Mengirim data ke MQTT
        if (client.connected()) {
          client.loop();
          sendToMQTT(receivedData);
        } else {
          reconnectMQTT();
        }
      }
    }
  }
}
