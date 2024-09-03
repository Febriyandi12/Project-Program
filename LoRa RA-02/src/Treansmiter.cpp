#include <SPI.h>
#include <LoRa.h>

const int csPin = 5;
const int resetPin = 14;
const int irqPin = 2;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println("LoRa Receiver");
  
  // Atur pin untuk modul LoRa
  LoRa.setPins(csPin, resetPin, irqPin);

  // Inisialisasi modul LoRa
  if (!LoRa.begin(433E6)) {  // 433E6 - Asia, 866E6 - Europe, 915E6 - North America
    Serial.println("Starting LoRa failed!");
    while (1);
  }

  Serial.println("LoRa Initializing OK!");
}

void loop() {
  // Mencoba untuk memparsing paket yang masuk
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    // Jika ada paket yang diterima, tampilkan pesan
    Serial.print("Received packet: '");

    // Membaca paket
    while (LoRa.available()) {
      String LoRaData = LoRa.readString();
      Serial.print(LoRaData);
    }

    // Menampilkan RSSI dari paket yang diterima
    Serial.print("' with RSSI ");
    Serial.println(LoRa.packetRssi());
  }
}
