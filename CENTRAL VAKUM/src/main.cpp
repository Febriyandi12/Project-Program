#include <SPI.h>
#include <LoRa.h>

// Konfigurasi pin LoRa
const int csPin = 5;     // Pin Chip Select
const int resetPin = 14; // Pin Reset
const int irqPin = 2;    // Pin Interrupt

// Alamat LoRa slave
const uint8_t masterAddress = 0x10;
const char *slaveMac = "VK-1111";

// Fungsi untuk menghasilkan nilai float acak dalam rentang tertentu
float randomFloat(float minVal, float maxVal)
{
  return minVal + ((float)random() / (float)RAND_MAX) * (maxVal - minVal);
}

// Fungsi untuk menghitung status berdasarkan nilai yang diterima
String perhitunganStatus(float value)
{
  if (value > -300.00)
  {
    return "high";
  }
  else if (value < -700.00)
  {
    return "low";
  }
  else
  {
    return "normal";
  }
}

void setup()
{
  Serial.begin(115200);
  while (!Serial)
    ;

  // Inisialisasi LoRa
  LoRa.setPins(csPin, resetPin, irqPin);
  if (!LoRa.begin(433E6))
  {
    Serial.println("Gagal menginisialisasi LoRa");
    while (1)
      ;
  }
  // Inisialisasi seed untuk random number generator
  Serial.println("Berhasil menginisialisasi LoRa");
  randomSeed(analogRead(0));
}

void loop()
{
  int packetSize = LoRa.parsePacket();
  if (packetSize)
  {
    // Baca alamat dari master
    String receivedString = "";
    while (LoRa.available())
    {
      char c = (char)LoRa.read();
      receivedString += c;
    }

    // Memisahkan master address dan MAC address dari paket yang diterima
    int commaIndex = receivedString.indexOf(',');
    if (commaIndex != -1)
    {
      String receivedAddress = receivedString.substring(0, commaIndex);
      String receivedMac = receivedString.substring(commaIndex + 2); // Menghilangkan spasi setelah koma

      // Jika MAC address cocok, kirim data kembali ke master
      if (receivedMac == slaveMac)
      {
        // Menghasilkan nilai float acak antara -200 dan -800
        float value = randomFloat(-800.00, -200.00);

        String status = perhitunganStatus(value);

        // Mengirim nilai acak ke master
        LoRa.beginPacket();
        LoRa.write(masterAddress);
        LoRa.print(slaveMac);
        LoRa.print(", ");
        LoRa.print(value, 2);
        LoRa.print("#");
        LoRa.endPacket();
        Serial.print("Nilai: ");
        Serial.print(value, 2);
        Serial.print(" mmHg, Status: ");
        Serial.println(status);
      }
    }
  }
}