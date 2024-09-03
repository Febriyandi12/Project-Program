#include <SPI.h>
#include <Arduino.h>
#include <LoRa.h>
#include <TaskScheduler.h>
#include <SoftwareSerial.h>
#include <Nextion.h>
#include <FS.h>
#include <SD.h>
#include <EEPROM.h>

Scheduler runner;
void loraCallback();
void nextionCallback();

Task t1(0, TASK_FOREVER, &loraCallback);
Task t2(2000, TASK_FOREVER, &nextionCallback);

// Konfigurasi pin LoRa
const int csPin = 5;     // Pin Chip Select
const int resetPin = 14; // Pin Reset
const int irqPin = 2;    // Pin Interrupt

// Alamat LoRa master
const uint8_t masterAddress = 0x10;
const char *slaveMac = "NO-1111";

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

void initializeTask()
{

  runner.init();
  runner.addTask(t1);
  runner.addTask(t2);

  t1.enable();
  t2.enable();
}

void initializeLoRa()
{
  LoRa.setPins(csPin, resetPin, irqPin);
  LoRa.setSpreadingFactor(7); // SF7 for maximum data rate
  LoRa.setSignalBandwidth(500E3);
  LoRa.setTxPower(20);
  if (!LoRa.begin(433E6))
  {
    Serial.println("Starting LoRa failed!");
  }
  dbSerial.println("LoRa Master Initialized, Ready");
}

void loraCallback()
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
        float value1 = randomFloat(2.00, 10.00);
        float value2 = randomFloat(2.00, 10.00);
        float value3 = randomFloat(2.00, 10.00);

        // Menghitung status untuk setiap nilai
        String status1 = perhitunganStatus(value1);
        String status2 = perhitunganStatus(value2);
        String status3 = perhitunganStatus(value3);

        // Mengirim nilai dan status acak ke master
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

        // Output untuk debugging
        Serial.print("Kirim ke master: ");
        Serial.print(value1, 2);
        Serial.print(" (");
        Serial.print(status1);
        Serial.print("), ");
        Serial.print(value2, 2);
        Serial.print(" (");
        Serial.print(status2);
        Serial.print("), ");
        Serial.print(value3, 2);
        Serial.print(" (");
        Serial.print(status3);
        Serial.println(")");
      }
    }
  }
}
void nextionCallback()
{
}
void setup()
{
  Serial.begin(115200);

  initializeTask();
  initializeLoRa();
  randomSeed(analogRead(0));
}

void loop()
{
  //  nexLoop(nex_listen_list);
  runner.execute();
}
