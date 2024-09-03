#include <SPI.h>
#include <Arduino.h>
#include <SoftwareSerial.h>
#include <Nextion.h>
#include <FS.h>
#include <SD.h>
#include <EEPROM.h>
#include <TaskScheduler.h>
#include <esp_system.h>
#include <LoRa.h>
#include <ArduinoJson.h>
#include <WiFi.h>

// Konfigurasi pin LoRa
const int csPin = 5;     // Pin Chip Select
const int resetPin = 14; // Pin Reset
const int irqPin = 2;    // Pin Interrupt

// Alamat LoRa untuk master dan slave
const uint8_t masterAddress = 0x10;
const char *slaveMacs[] = {
    "OX-1111", // Oksigen
    "NO-1111", // Nitrous Oxide
    "CO-1111", // Carbon Dioxide
    "KP-1111", // Kompressor
    "VK-1111", // Vakum
    "NG-1111"  // Nitrogen
};
int numSlaves = sizeof(slaveMacs) / sizeof(slaveMacs[0]);

// Waktu tunggu respons dari slave
const long timeout = 70; // 70 ms timeout

// Struktur untuk menyimpan data dari setiap slave
struct SlaveData
{
  String mac;
  float value1;
  float value2;
  float value3;
};

// Array untuk menyimpan data dari setiap slave
SlaveData slaveData[6];

// Scheduler dan Task
Scheduler runner;
void t1Callback();
Task t1(0, TASK_FOREVER, &t1Callback);

int currentSlaveIndex = 0;
bool awaitingResponse = false;
unsigned long requestTime = 0;

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
  Serial.println("LoRa Master siap!");

  // Inisialisasi Scheduler
  runner.init();
  runner.addTask(t1);
  t1.enable();

  // Inisialisasi MAC address untuk setiap slave
  for (int i = 0; i < numSlaves; i++)
  {
    slaveData[i].mac = slaveMacs[i];
  }
}

void loop()
{
  runner.execute();
}

void t1Callback()
{
  if (!awaitingResponse)
  {
    // Kirim permintaan data ke slave
    const char *slaveMac = slaveMacs[currentSlaveIndex];
    LoRa.beginPacket();
    LoRa.print(masterAddress); // Kirim alamat master
    LoRa.print(", ");
    LoRa.print(slaveMac); // Kirim MAC slave
    LoRa.endPacket();
    LoRa.receive();

    awaitingResponse = true;
    requestTime = millis();
  }
  else
  {
    // Tunggu respons dari slave
    if (millis() - requestTime < timeout)
    {
      int packetSize = LoRa.parsePacket();
      if (packetSize)
      {
        uint8_t receivedAddress = LoRa.read();

        if (receivedAddress == masterAddress)
        {

          // Membaca data dari slave dan menyimpannya berdasarkan slaveMac
          String mac = "";
          String value1 = "", value2 = "", value3 = "";
          bool parsingMac = true, parsingValue1 = false, parsingValue2 = false, parsingValue3 = false;

          while (LoRa.available())
          {
            char c = (char)LoRa.read();
            Serial.print(c);

            if (c == ',')
            {
              if (parsingMac)
              {
                parsingMac = false;
                parsingValue1 = true;
              }
              else if (parsingValue1)
              {
                parsingValue1 = false;
                parsingValue2 = true;
              }
              else if (parsingValue2)
              {
                parsingValue2 = false;
                parsingValue3 = true;
              }
            }
            else
            {
              if (parsingMac)
              {
                mac += c;
              }
              else if (parsingValue1)
              {
                value1 += c;
              }
              else if (parsingValue2)
              {
                value2 += c;
              }
              else if (parsingValue3)
              {
                value3 += c;
              }
            }
          }
          Serial.println();

          // Simpan data ke array berdasarkan mac
          for (int i = 0; i < numSlaves; i++)
          {
            if (slaveData[i].mac == mac)
            {
              slaveData[i].value1 = value1.toFloat();
              slaveData[i].value2 = value2.toFloat();
              slaveData[i].value3 = value3.toFloat();
              Serial.print("Data tersimpan untuk ");
              Serial.println(slaveData[i].mac);
              Serial.print("Nilai 1: ");
              Serial.println(slaveData[i].value1);
              Serial.print("Nilai 2: ");
              Serial.println(slaveData[i].value2);
              Serial.print("Nilai 3: ");
              Serial.println(slaveData[i].value3);
              break;
            }
          }

          awaitingResponse = false;
          currentSlaveIndex = (currentSlaveIndex + 1) % numSlaves; // Berpindah ke slave berikutnya
        }
      }
    }
    else
    {
      awaitingResponse = false;
      currentSlaveIndex = (currentSlaveIndex + 1) % numSlaves; // Berpindah ke slave berikutnya
    }
  }
}