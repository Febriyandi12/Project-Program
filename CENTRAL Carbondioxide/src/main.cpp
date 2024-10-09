#include <SPI.h>
#include <SD.h>
#include <LoRa.h>
#include <TaskScheduler.h>
#include <Nextion.h>
#include <Adafruit_ADS1X15.h>

Adafruit_ADS1115 ads;
// Konfigurasi pin LoRa
const int buzzerPin = 5;
const int SD_CS = 16;
const int LORA_CS = 17;
const int LORA_RESET = 15; //ok
const int LORA_IRQ = 33;   //ok
// Pin Chip Select
Scheduler runner;
void loraCallback();
void nextionCallback();

Task t1(0, TASK_FOREVER, &loraCallback);
Task t2(2000, TASK_FOREVER, &nextionCallback);

// Alamat LoRa slave
const uint8_t masterAddress = 0x10;
const char *slaveMac = "CO-1111";
/*
Serial Number Mac
OX = Oksigen SUDAH
NO = Nitorus Oxide
CO = Carbondioxide SUDAH
NG = Nitrogen  SUDAH
KP = Kompressor
VK = Vakum SUDAH
*/

// Nextion objects
NexButton bpagelogin = NexButton(0, 4, "bpagelogin");
NexButton bpagehome = NexButton(1, 5, "bpagelogin");
NexButton bsubmit = NexButton(1, 4, "bsubmit");
NexButton balarmsetting = NexButton(1, 23, "b2");
NexButton blogin = NexButton(2, 3, "blogin");
NexButton btutup = NexButton(2, 4, "btutup");
NexButton bback = NexButton(3, 7, "b1");

NexButton bsimpan = NexButton(4, 6, "bsimpan");
NexButton bback1 = NexButton(4, 7, "bback1");
NexButton bsetAlarm = NexButton(3, 12, "bsetAlarm");
NexDSButton bBar = NexDSButton(1, 19, "bBar");
NexDSButton bKPa = NexDSButton(1, 20, "bKPa");
NexDSButton bPSi = NexDSButton(1, 21, "bPSi");

NexPage home = NexPage(0, 0, "mainmenu3");
NexPage settingPage = NexPage(1, 0, "menusetting");
NexPage loginpage = NexPage(2, 0, "loginpage");
NexPage alarmpage = NexPage(3, 0, "settingalarm");
NexPage settingsn = NexPage(4, 0, "settingsn");

// input untuk halaman login
NexText password = NexText(2, 2, "tpassword");

// input untuk menyeting password user
NexNumber setpassworduser = NexNumber(1, 17, "npassworduser");
// input password admin
NexNumber setpasswordadmin = NexNumber(4, 16, "npasswordadmin");

// input untuk tampilan di halaman utama
NexNumber inputnumber1 = NexNumber(1, 1, "n0");
NexNumber inputnumber2 = NexNumber(1, 2, "n1");
NexNumber inputnumber3 = NexNumber(1, 3, "n2");
NexNumber inputnumber4 = NexNumber(1, 11, "n3");
NexNumber inputnumber5 = NexNumber(1, 12, "n4");
NexNumber inputnumber6 = NexNumber(1, 15, "n5");

NexNumber tOxmin = NexNumber(3, 11, "tOxmin");
NexNumber tOxmax = NexNumber(3, 12, "tOxmax");
NexNumber tNomin = NexNumber(3, 13, "tNomin");
NexNumber tNomax = NexNumber(3, 14, "tNomax");
NexNumber tComin = NexNumber(3, 15, "tComin");
NexNumber tComax = NexNumber(3, 16, "tComax");
NexNumber tKompressor = NexNumber(3, 17, "tKompressor");
NexNumber tVakum = NexNumber(3, 18, "tVakum");
NexNumber tNgmin = NexNumber(3, 19, "tNgmin");
NexNumber tNgmax = NexNumber(3, 20, "tNgmax");

// input untuk konfigurasi slave
NexText tslave1 = NexText(4, 1, "tslave1");
NexText tslave2 = NexText(4, 2, "tslave2");
NexText tslave3 = NexText(4, 3, "tslave3");
NexText tslave4 = NexText(4, 4, "tslave4");
NexText tslave5 = NexText(4, 16, "tslave5");
NexText tslave6 = NexText(4, 6, "tslave6");

NexText tserialNumber = NexText(1, 20, "tserialnumber");

NexText tKet[6] = {
    NexText(0, 33, "tKet0"),
    NexText(0, 34, "tKet1"),
    NexText(0, 35, "tKet2"),
    NexText(0, 36, "tKet3"),
    NexText(0, 37, "tKet4"),
    NexText(0, 38, "tKet5")}; // Sudah diubah

NexText backgorund[6] = {
    NexText(0, 3, "bg0"),
    NexText(0, 2, "bg1"),
    NexText(0, 1, "bg2"),
    NexText(0, 10, "bg3"),
    NexText(0, 9, "bg4"),
    NexText(0, 18, "bg5")}; // Sudah diubah

NexText gasText[6] = {
    NexText(0, 6, "tGas0"),
    NexText(0, 7, "tGas1"),
    NexText(0, 8, "tGas2"),
    NexText(0, 11, "tGas3"),
    NexText(0, 12, "tGas4"),
    NexText(0, 19, "tGas5")}; // Sudah diubah

NexText satuanText[6][1] = {
    {NexText(0, 27, "tSatuan0")},
    {NexText(0, 28, "tSatuan1")},
    {NexText(0, 29, "tSatuan2")},
    {NexText(0, 30, "tSatuan3")},
    {NexText(0, 31, "tSatuan4")},
    {NexText(0, 32, "tSatuan5")}}; // Sudah diubah

NexText supplyText[6] = {
    NexText(0, 21, "tSupply0"),
    NexText(0, 46, "tSupply1"),
    NexText(0, 47, "tSupply2"),
    NexText(0, 48, "tSupply3"),
    NexText(0, 49, "tSupply4"),
    NexText(0, 50, "tSupply5")};

NexTouch *nex_listen_list[] = {&bpagelogin, &bpagehome, &settingPage, &settingsn,
                               &bsubmit, &bsimpan, &bback1, &balarmsetting,
                               &bsetAlarm, &blogin, &btutup, &bBar,
                               &bKPa, &bPSi, &bback, &tserialNumber, NULL};

void activateSDCard()
{
  digitalWrite(SD_CS, LOW);    // Aktifkan SD Card
  digitalWrite(LORA_CS, HIGH); // Nonaktifkan LoRa
}

void activateLoRa()
{
  digitalWrite(LORA_CS, LOW);
  digitalWrite(SD_CS, HIGH); // Nonaktifkan SD Card     // Aktifkan LoRa
}
void setupLoRa()
{
  activateLoRa(); 
  LoRa.setPins(LORA_CS, LORA_RESET, LORA_IRQ);

  if (LoRa.begin(433E6))
  {
    LoRa.setSpreadingFactor(7);
    LoRa.setSignalBandwidth(125E3);
    LoRa.setCodingRate4(5);
    LoRa.setTxPower(20);
  }
  else
  {
    dbSerial.println("Gagal menginisialisasi LoRa");
    // indikatorError(2);
  }
}

void setupSDCard()
{
  activateSDCard(); 

  if (SD.begin(SD_CS))
  {
    if (!SD.exists("/setting"))
    {
      SD.mkdir("/setting");
    }
    dbSerial.println("Berhasil menginisialisasi SD Card");
  }
  else
  {
    dbSerial.println("Gagal menginisialisasi SD Card");
    // indikatorError(1);
  }
}

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
  nexInit();
  setupSDCard();
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, LOW);

  runner.init();
  runner.addTask(t1);
  runner.addTask(t2);

  t1.enable();
  t2.enable();

  randomSeed(analogRead(0));
  Serial.println(slaveMac);
  activateLoRa();
  setupLoRa();
}

void nextionCallback()
{
}
void loraCallback()
{
  if (LoRa.begin(433E6))
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
}
void loop()
{
  nexLoop(nex_listen_list);
  runner.execute();
}
