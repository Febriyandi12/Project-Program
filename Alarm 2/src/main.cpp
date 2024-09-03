#include <SPI.h>
#include <LoRa.h>

// Konfigurasi pin LoRa
const int csPin = 5;     // Pin Chip Select
const int resetPin = 14; // Pin Reset
const int irqPin = 2;    // Pin Interrupt

// Alamat LoRa slave
const uint8_t masterAddress = 0x10;
const char *slaveMac = "OX-1111";
/*
Serial Number Mac
OX = Oksigen
NO = Nitorus Oxide
CO = Carbondioxide
KP = Kompressor
VK = Vakum
NG = Nitrogen
*/

// Fungsi untuk menghasilkan nilai float acak dalam rentang tertentu
float randomFloat(float minVal, float maxVal)
{
  return minVal + ((float)random() / (float)RAND_MAX) * (maxVal - minVal);
}

// Fungsi untuk menghitung status berdasarkan nilai yang diterima
String perhitunganStatus(float value)
{
  if (value < 4.00)
  {
    return "low";
  }
  else if (value > 8.00)
  {
    return "high";
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
        // Menghasilkan 3 nilai float acak antara 2.00 dan 9.00
        float value1 = randomFloat(2.00, 9.00);
        float value2 = randomFloat(2.00, 9.00);
        float value3 = randomFloat(2.00, 9.00);

        String status1 = perhitunganStatus(value1);
        String status2 = perhitunganStatus(value2);
        String status3 = perhitunganStatus(value3);

        // Mengirim nilai-nilai acak ke master
        LoRa.beginPacket();
        LoRa.write(masterAddress);
        LoRa.print(slaveMac);
        LoRa.print(", ");
        LoRa.print(value1, 2);
        LoRa.print(", ");
        LoRa.print(value2, 2);
        LoRa.print(", ");
        LoRa.print(value3, 2);
        LoRa.print("#");
        LoRa.endPacket();
        Serial.print("Nilai 3: ");
        Serial.println(value3, 2);
      }
    }
  }
}
