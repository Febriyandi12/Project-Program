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
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>

#define TRIGGER_PIN 0

const int csPin = 5;     // LoRa radio chip select
const int resetPin = 14; // LoRa radio reset
const int irqPin = 2;    // LoRa radio interrupt

bool wifimanager_nonblocking = true;

const char *mqttServer = "test.mosquitto.org";
const int mqttPort = 1883;

WiFiClient espClient;
PubSubClient client(espClient);
WiFiManager wifimanager;
WiFiManagerParameter custom_field;

const uint8_t masterAddress = 0x10;
const char *slaveMacs[] = {
    "OX-1111", // Oksigen
    "NO-1111", // Nitrous Oxide
               // "CO-1111", // Carbon Dioxide
               // "KP-1111", // Kompressor
               // "VK-1111", // Vakum
               // "NG-1111"  // Nitrogen
};

int numSlaves = sizeof(slaveMacs) / sizeof(slaveMacs[0]);

// Waktu tunggu respons dari slave
const long timeout = 100; // 70 ms timeout

// Struktur untuk menyimpan data dari setiap slave
struct SlaveData
{
  String mac;
  float supply;
  float leftBank;
  float rightBank;
};

// Array untuk menyimpan data dari setiap slave
SlaveData slaveData[6];

Scheduler runner;
void t1Callback();
void t2Callback();
void t3Callback();
// void t4Callback();

// Task definitions
Task t1(100, TASK_FOREVER, &t1Callback);
Task t2(2000, TASK_FOREVER, &t2Callback);
Task t3(1000, TASK_FOREVER, &t3Callback);
// Task t4(1000, TASK_FOREVER, &t4Callback);

// Variabel
int currentSlaveIndex = 0;
bool awaitingResponse = false;
unsigned long requestTime = 0;

int currentSlaveToSend = 0;

String receivedData = "";
byte eepromAddressUnit = 28;
uint8_t lastSelectedButton = 0;

int unitState; // 1 = Bar, 2 = KPa, 3 = Psi
int currentState = 0;
int statebutton;
// Buffer untuk menyimpan nilai
char data0[10], data1[10], data2[10], data3[10], data4[10], data5[10], data6[10], data7[10], data8[10];
const char *nilai[] = {data0, data1, data2, data3, data4, data5, data6, data7, data8};

// Satuan, Status, Gas, dan Keterangan
const char *satuan = "Bar";
const char *satuan1 = "mmHg";
const char *satuan2 = "KPa";
const char *satuan3 = "Psi";

const char *status[] = {"Normal", "Low", "Over"};
const char *gas[] = {"Oxygen", "Nitrous Oxide", "Carbondioxide", "Kompressor", "Vakum", "Nitrogen"};
const char *ket[] = {"Supply", "Gas", "Left Bank", "Right Bank", "Medical Air", "Tools Air"};

const char *bar = "1";
const char *kpa = "1";
const char *psi = "1";

// Nextion objects
NexButton bpagelogin = NexButton(0, 18, "bpagelogin");
NexButton bpagehome = NexButton(1, 5, "bpagelogin");
NexButton bsubmit = NexButton(1, 4, "bsubmit");
NexButton balarmsetting = NexButton(1, 25, "b2");
NexButton blogin = NexButton(2, 4, "blogin");
NexButton btutup = NexButton(2, 5, "btutup");
NexButton bback = NexButton(3, 7, "b1");

NexButton bsimpan = NexButton(4, 7, "bsimpan");
NexButton bback1 = NexButton(4, 8, "bback1");

// NexButton bsetting = NexButton(3, 8, "b0");
NexDSButton bBar = NexDSButton(1, 19, "bBar");
NexDSButton bKPa = NexDSButton(1, 20, "bKPa");
NexDSButton bPSi = NexDSButton(1, 21, "bPSi");

NexPage home = NexPage(0, 0, "mainmenu3");
NexPage settingPage = NexPage(1, 0, "menusetting");
NexPage loginpage = NexPage(2, 0, "loginpage");
NexPage alarmpage = NexPage(3, 0, "settingalarm");
NexPage settingsn = NexPage(4, 0, "settingsn");

// input untuk halaman login
NexText password = NexText(2, 3, "tpassword");
NexText alert = NexText(2, 1, "t1");

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

// input untuk konfigurasi slave
NexText tslave1 = NexText(4, 1, "tslave1");
NexText tslave2 = NexText(4, 2, "tslave2");
NexText tslave3 = NexText(4, 3, "tslave3");
NexText tslave4 = NexText(4, 4, "tslave4");
NexText tslave5 = NexText(4, 5, "tslave5");
NexText tslave6 = NexText(4, 6, "tslave6");

NexNumber xminsensor = NexNumber(3, 8, "xminsensor");
NexNumber xminvakum = NexNumber(3, 10, "xminvakum");
NexNumber xminkompressor = NexNumber(3, 11, "xminkompressor");
NexNumber xmaxsensor = NexNumber(3, 9, "xmaxsensor");

NexText tserialNumber = NexText(1, 20, "tserialnumber");

NexText backgorund[6] = {
    NexText(0, 3, "bg0"),
    NexText(0, 2, "bg1"),
    NexText(0, 1, "bg2"),
    NexText(0, 28, "bg3"),
    NexText(0, 27, "bg4"),
    NexText(0, 63, "bg5")};

NexText gasText[6] = {
    NexText(0, 24, "tGas0"),
    NexText(0, 25, "tGas1"),
    NexText(0, 26, "tGas2"),
    NexText(0, 41, "tGas3"),
    NexText(0, 42, "tGas4"),
    NexText(0, 70, "tGas5")};

NexText satuanText[6][3] = {
    {NexText(0, 55, "tsatuan0_0"), NexText(0, 56, "tsatuan1_0"), NexText(0, 57, "tsatuan2_0")},
    {NexText(0, 54, "tsatuan0_1"), NexText(0, 53, "tsatuan1_1"), NexText(0, 52, "tsatuan2_1")},
    {NexText(0, 51, "tsatuan0_2"), NexText(0, 50, "tsatuan1_2"), NexText(0, 49, "tsatuan2_2")},
    {NexText(0, 43, "tsatuan0_3"), NexText(0, 44, "tsatuan1_3"), NexText(0, 45, "tsatuan2_3")},
    {NexText(0, 48, "tsatuan0_4"), NexText(0, 47, "tsatuan1_4"), NexText(0, 46, "tsatuan2_4")},
    {NexText(0, 73, "tsatuan0_5"), NexText(0, 72, "tsatuan1_5"), NexText(0, 71, "tsatuan2_5")}};

NexText supplyText[6] = {
    NexText(0, 6, "tsupply0"),
    NexText(0, 11, "tsupply1"),
    NexText(0, 13, "tsupply2"),
    NexText(0, 30, "tsupply3"),
    NexText(0, 36, "tsupply4"),
    NexText(0, 65, "tsupply5")};

NexText ketText[6][2] = {
    {NexText(0, 6, "tket0_0"), NexText(0, 7, "tket1_0")},
    {NexText(0, 20, "tket0_1"), NexText(0, 19, "tket1_1")},
    {NexText(0, 14, "tket0_2"), NexText(0, 15, "tket1_2")},
    {NexText(0, 31, "tket0_3"), NexText(0, 32, "tket1_3")},
    {NexText(0, 38, "tket0_4"), NexText(0, 37, "tket1_4")},
    {NexText(0, 67, "tket0_5"), NexText(0, 66, "tket1_5")}};

NexText nilaiText[6][2] = {
    {NexText(0, 8, "nilai0_0"), NexText(0, 9, "nilai1_0")},
    {NexText(0, 21, "nilai0_1"), NexText(0, 22, "nilai1_1")},
    {NexText(0, 16, "nilai0_2"), NexText(0, 17, "nilai1_2")},
    {NexText(0, 33, "nilai0_3"), NexText(0, 34, "nilai1_3")},
    {NexText(0, 39, "nilai0_4"), NexText(0, 40, "nilai1_4")},
    {NexText(0, 68, "nilai0_5"), NexText(0, 69, "nilai1_5")}};

NexText valueText[6] = {
    NexText(0, 4, "value0"),
    NexText(0, 10, "value1"),
    NexText(0, 12, "value2"),
    NexText(0, 29, "value3"),
    NexText(0, 35, "value4"),
    NexText(0, 64, "value5")};

NexTouch *nex_listen_list[] = {&bpagelogin, &bpagehome, &settingPage, &settingsn,
                               &bsubmit, &bsimpan, &bback1, &balarmsetting,
                               &blogin, &btutup, &bBar,
                               &bKPa, &bPSi, &bback, &xminvakum,
                               &xminsensor, &xminkompressor, &xmaxsensor, &tserialNumber, NULL};

String getParam(String name)
{
  // read parameter from server, for customhmtl input
  String value;
  if (wifimanager.server->hasArg(name))
  {
    value = wifimanager.server->arg(name);
  }
  return value;
}

// Fungsi untuk konversi data ke format JSON
String convertDataToJson(const String &mac, float supply, float leftBank, float rightBank)
{
  JsonDocument jsonDoc;
  jsonDoc["mac"] = mac;
  jsonDoc["supply"] = supply;
  jsonDoc["leftBank"] = leftBank;
  jsonDoc["rightBank"] = rightBank;

  String jsonData;
  serializeJson(jsonDoc, jsonData);
  return jsonData;
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

void handleMQTTConnection()
{
  if (!client.connected())
  {
    if (client.connect("ESP32Master"))
    {
      Serial.println("MQTT connected");
    }
    else
    {
      Serial.print("MQTT connection failed, rc=");
      Serial.print(client.state());
    }
  }
  client.loop();
}

// Fungsi untuk mengirim data ke MQTT
void sendToMqtt(const String &mac, const String &jsonData)
{
  String topic = "LoRa1/data/" + mac;
  client.publish(topic.c_str(), jsonData.c_str());
}

void setupwifimanager()
{
  WiFi.mode(WIFI_STA);
  pinMode(TRIGGER_PIN, INPUT);

  if (wifimanager_nonblocking)
    wifimanager.setConfigPortalBlocking(false);
  int customFieldLength = 40;

  wifimanager.addParameter(&custom_field);
  std::vector<const char *> menu = {"wifi", "info", "param", "close", "sep", "erase", "restart", "exit"};
  wifimanager.setMenu(menu);
  wifimanager.setMinimumSignalQuality(40);
  wifimanager.setClass("invert");
  wifimanager.setScanDispPerc(true);
  wifimanager.setBreakAfterConfig(true);
  wifimanager.setConfigPortalTimeout(false); // auto close configportal after n seconds
  wifimanager.setCaptivePortalEnable(true);
  bool res;
  res = wifimanager.autoConnect("Master Alarm", "");
}

void checkButton()
{
  // check for button press
  if (digitalRead(TRIGGER_PIN) == LOW)
  {
    // poor mans debounce/press-hold, code not ideal for production
    delay(50);
    if (digitalRead(TRIGGER_PIN) == LOW)
    {
      dbSerial.println("Button Pressed");
      // still holding button for 3000 ms, reset settings, code not ideaa for production
      delay(3000); // reset delay hold
      if (digitalRead(TRIGGER_PIN) == LOW)
      {
        dbSerial.println("Button Held");
        dbSerial.println("Erasing Config, restarting");
        wifimanager.resetSettings();
        ESP.restart();
      }

      // start portal w delay
      dbSerial.println("Starting config portal");
      wifimanager.setConfigPortalTimeout(120);

      if (!wifimanager.startConfigPortal("Master Alarm", ""))
      {
        dbSerial.println("failed to connect or hit timeout");
        delay(3000);
        // ESP.restart();
      }
      else
      {
        // if you get here you have connected to the WiFi
        dbSerial.println("connected...yeey :)");
      }
    }
  }
}

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

void saveButtonState(uint8_t buttonNumber)
{
  EEPROM.write(eepromAddressUnit, buttonNumber);
  EEPROM.commit();
}

void writeEEprom()
{
  // Array untuk menyimpan semua nilai byte
  byte values[11];

  uint32_t tempValue; // Variabel sementara untuk menyimpan nilai dari Nextion
  uint32_t Xfloat;
  uint32_t passworduser;
  // Ambil nilai dari komponen Nextion
  setpassworduser.getValue(&passworduser);
  EEPROM.put(44, passworduser);

  xminsensor.getValue(&Xfloat);
  EEPROM.put(10, Xfloat);

  xminvakum.getValue(&Xfloat);
  EEPROM.put(13, Xfloat);

  xminkompressor.getValue(&Xfloat);
  EEPROM.put(16, Xfloat);

  xmaxsensor.getValue(&Xfloat);
  EEPROM.put(19, tempValue);

  inputnumber1.getValue(&tempValue);
  values[0] = (byte)tempValue;

  inputnumber2.getValue(&tempValue);
  values[1] = (byte)tempValue;

  inputnumber3.getValue(&tempValue);
  values[2] = (byte)tempValue;

  inputnumber4.getValue(&tempValue);
  values[3] = (byte)tempValue;

  inputnumber5.getValue(&tempValue);
  values[4] = (byte)tempValue;

  inputnumber6.getValue(&tempValue);
  values[5] = (byte)tempValue;

  // xminsensor.getValue(&tempValue);
  // values[7] = (byte)tempValue;

  // xminvakum.getValue(&tempValue);
  // values[8] = (byte)tempValue;

  // xminkompressor.getValue(&tempValue);
  // values[9] = (byte)tempValue;

  // xmaxsensor.getValue(&tempValue);
  // values[10] = (byte)tempValue;

  // Tulis semua nilai ke EEPROM
  for (int i = 0; i < 11; i++)
  {
    EEPROM.write(i, values[i]);
  }

  EEPROM.commit(); // Simpan semua perubahan ke EEPROM

  // Cetak nilai untuk memastikan mereka ditulis dengan benar
  // Serial.println("Tulis input");
  // Serial.println(values[10]);
  // Serial.println(values[13]);
  // Serial.println(values[16]);
  // Serial.println(values[19]);
}

void readEEprom()
{
  // Array untuk menyimpan semua nilai byte
  byte values[11];
  uint32_t passworduser, passwordadmin;

  EEPROM.get(44, passworduser);
  // int sandi1 = passworduser;

  EEPROM.get(38, passwordadmin);
  // int sandi2 = passwordadmin;

  // Baca semua nilai dari EEPROM
  for (int i = 0; i < 11; i++)
  {
    values[i] = EEPROM.read(i);
  }

  // Set nilai pada komponen Nextion
  inputnumber1.setValue(values[0]);
  inputnumber2.setValue(values[1]);
  inputnumber3.setValue(values[2]);
  inputnumber4.setValue(values[3]);
  inputnumber5.setValue(values[4]);
  inputnumber6.setValue(values[5]);
  setpassworduser.setValue(passworduser);
  setpasswordadmin.setValue(passwordadmin);

  xminsensor.setValue(values[10]);
  xminvakum.setValue(values[13]);
  xminkompressor.setValue(values[16]);
  xmaxsensor.setValue(values[19]);

  // Cetak nilai yang dibaca untuk verifikasi
}

// Function to read EEPROM values
void readEEPROMValues(byte *numberValue)
{
  numberValue[0] = EEPROM.read(0);
  numberValue[1] = EEPROM.read(1);
  numberValue[2] = EEPROM.read(2);
  numberValue[3] = EEPROM.read(3);
  numberValue[4] = EEPROM.read(4);
  numberValue[5] = EEPROM.read(5);
}

// Fungsi untuk mendapatkan status tombol dari EEPROM

// Fungsi untuk memperbarui warna tombol berdasarkan tombol yang dipilih
void updateButtonColors(uint8_t selectedButton)
{
  selectedButton = EEPROM.read(16);
  uint32_t colorInactive = 65535; // Warna default (misalnya putih)
  uint32_t colorActive = 2016;    // Warna aktif (misalnya biru)

  // Perbarui warna untuk masing-masing tombol
  bBar.Set_state0_color_bco0(selectedButton == 1 ? colorActive : colorInactive);
  bKPa.Set_state0_color_bco0(selectedButton == 2 ? colorActive : colorInactive);
  bPSi.Set_state0_color_bco0(selectedButton == 3 ? colorActive : colorInactive);
}

void clearinputnilai()
{
  inputnumber1.setValue(0);
  inputnumber2.setValue(0);
  inputnumber3.setValue(0);
  inputnumber4.setValue(0);
  inputnumber5.setValue(0);
  inputnumber6.setValue(0);
}

void verifikasilogin()
{
  uint32_t adminpassword, userpassword;

  EEPROM.get(44, userpassword);
  EEPROM.get(38, adminpassword);

  int sandi1 = adminpassword;
  int sandi2 = userpassword;

  char passwordValue[6];

  // Mengambil teks dari input Nextion
  password.getText(passwordValue, sizeof(passwordValue)); // Pastikan `password` adalah objek Nextion Text input

  dbSerial.println("Password Entered: ");
  dbSerial.println(passwordValue); // Cetak teks password untuk debug

  dbSerial.println("password admin ubah ke int: ");
  dbSerial.println(sandi1); // Cetak teks password untuk debug

  dbSerial.println("password admin biasa: ");
  dbSerial.println(adminpassword);

  // Konversi teks menjadi integer untuk perbandingan
  int passwordInt = atoi(passwordValue);

  // Verifikasi password untuk admin
  if (passwordInt == sandi1)
  {
    dbSerial.println("Admin login successful");

    password.setText("");
    settingsn.show();
    // Arahkan ke halaman settings
    readEEprom(); // Baca nilai dari EEPROM
  }
  // Verifikasi password untuk user
  else if (passwordInt == sandi2)
  {
    dbSerial.println("User login successful");
    password.setText("");
    settingPage.show(); // Arahkan ke halaman settingPage
    readEEprom();       // Baca nilai dari EEPROM
  }
  else
  {
    dbSerial.println("Password salah"); // Pesan jika password salah
    password.setText("");               // Kosongkan input password
  }
}

void handleReceivedData()
{
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, receivedData);

  const char *device_id = doc["device_id"];
  // const char *serialNumber = doc["serialnumber"];
  const char *source_left = doc["source_left"];
  const char *source_right = doc["source_right"];
  const char *temperature = doc["temperature"];
  const char *flow = doc["flow"];

  const char *serialNumber = doc["serialnumber"];

  // Clear receivedData
  receivedData = "";
}

void bPageloginPopCallback(void *ptr) // menu setting home
{
  t1.enable();
  t2.disable();
  loginpage.show();
}

void bSubmitPopCallback(void *ptr)
{
  writeEEprom();
}

void bpPasswordPopCallback(void *ptr) // button submit password
{
  verifikasilogin();
}

void badminpasswordPopCallback(void *ptr)
{
  uint32_t passwordadmin;
  // Ambil nilai dari komponen Nextion

  setpasswordadmin.getValue(&passwordadmin);
  EEPROM.put(38, passwordadmin);
  dbSerial.println("seting diadmin");
  dbSerial.println(passwordadmin);

  EEPROM.commit();
}

void balarmsettingPopCallback(void *ptr) // button back home
{
  alarmpage.show();
}

void bbackPopCallback(void *ptr) // button back home
{
  settingPage.show();
}

void bPagehomePopCallback(void *ptr) // button back home
{
  home.show();
  esp_restart();
  clearinputnilai();
}

void bBarPopCallback(void *ptr)
{
  lastSelectedButton = 1;
  saveButtonState(lastSelectedButton);
  updateButtonColors(lastSelectedButton);
}

void bKPaPopCallback(void *ptr)
{
  lastSelectedButton = 2;
  saveButtonState(lastSelectedButton);
  updateButtonColors(lastSelectedButton);
}

void bPSiPopCallback(void *ptr)
{
  lastSelectedButton = 3;
  saveButtonState(lastSelectedButton);
  updateButtonColors(lastSelectedButton);
}

// Function to update the display based on the gas type
void updateGasUI(int currentIndex, byte gasType)
{
  byte currentIndexByte = (byte)currentIndex;
  readEEPROMValues(&currentIndexByte);
  unitState = EEPROM.read(eepromAddressUnit);

  const char *selectedSatuan;

  // Menentukan satuan berdasarkan unitState yang dibaca dari EEPROM
  if (unitState == 2)
  {
    selectedSatuan = satuan2; // KPa
  }
  else if (unitState == 3)
  {
    selectedSatuan = satuan3; // Psi
  }
  else
  {
    selectedSatuan = satuan; // Default Bar
  }

  // Setel gambar latar belakang
  backgorund[currentIndex].Set_background_image_pic(6);

  // Setel teks gas
  gasText[currentIndex].setText(gas[gasType - 1]);

  // Reset nilai teks satuan
  satuanText[currentIndex][0].setText(selectedSatuan);
  satuanText[currentIndex][1].setText("");
  satuanText[currentIndex][2].setText("");

  // Reset teks status dan supply
  supplyText[currentIndex].setText(ket[0]); // Default ke "Supply"

  switch (gasType)
  {
  case 1: // Oxygen
          // Tampilkan semua nilai

    supplyText[currentIndex].setText(ket[0]);            // Default ke "Supply"
    ketText[currentIndex][0].setText(ket[2]);            // Left Bank
    ketText[currentIndex][1].setText(ket[3]);            // Right Bank
    satuanText[currentIndex][1].setText(selectedSatuan); // Satuan Left Bank
    satuanText[currentIndex][2].setText(selectedSatuan); // Satuan Right Bank
    break;
  case 2: // Nitrous Oxide

    supplyText[currentIndex].setText(ket[0]);            // Default ke "Supply"
    ketText[currentIndex][0].setText(ket[2]);            // Left Bank
    ketText[currentIndex][1].setText(ket[3]);            // Right Bank
    satuanText[currentIndex][1].setText(selectedSatuan); // Satuan Left Bank
    satuanText[currentIndex][2].setText(selectedSatuan);
    break;
  case 3: // Carbondioxide

    supplyText[currentIndex].setText(ket[0]);            // Default ke "Supply"
    ketText[currentIndex][0].setText(ket[2]);            // Left Bank
    ketText[currentIndex][1].setText(ket[3]);            // Right Bank
    satuanText[currentIndex][1].setText(selectedSatuan); // Satuan Left Bank
    satuanText[currentIndex][2].setText(selectedSatuan);
    break;
  case 6: // Nitrogen

    supplyText[currentIndex].setText(ket[0]);            // Default ke "Supply"
    ketText[currentIndex][0].setText(ket[2]);            // Left Bank
    ketText[currentIndex][1].setText(ket[3]);            // Right Bank
    satuanText[currentIndex][1].setText(selectedSatuan); // Satuan Left Bank
    satuanText[currentIndex][2].setText(selectedSatuan);
    break;

  case 4: // Kompressor

    supplyText[currentIndex].setText(ket[0]); // Default ke "Supply"
    break;

  case 5: // Vakum
          // Hanya tampilkan nilai utama

    supplyText[currentIndex].setText(ket[0]); // Default ke "Supply"
    satuanText[currentIndex][0].setText(satuan1);
    break;
  }

  // Setel warna latar belakang dan teks
  uint16_t backgroundColor = 14791;

  nilaiText[currentIndex][0].Set_background_color_bco(backgroundColor);
  nilaiText[currentIndex][1].Set_background_color_bco(backgroundColor);

  valueText[currentIndex].Set_background_color_bco(backgroundColor);

  satuanText[currentIndex][0].Set_background_color_bco(backgroundColor);
  satuanText[currentIndex][1].Set_background_color_bco(backgroundColor);
  satuanText[currentIndex][2].Set_background_color_bco(backgroundColor);

  supplyText[currentIndex].Set_background_color_bco(backgroundColor);

  ketText[currentIndex][0].Set_background_color_bco(backgroundColor);
  ketText[currentIndex][1].Set_background_color_bco(backgroundColor);
}

// Function to clear unused displays
void clearUnusedDisplays()
{
  for (int i = 0; i < 6; i++)
  {

    gasText[i].setText("-");
    valueText[i].setText("");
    supplyText[i].setText("");
    ketText[i][0].setText("");
    ketText[i][1].setText("");
    satuanText[i][0].setText("");
    satuanText[i][1].setText("");
    satuanText[i][2].setText("");

    gasText[i].Set_font_color_pco(0);
    valueText[i].Set_background_color_bco(0);
    gasText[i].Set_background_color_bco(0);
    satuanText[i][0].Set_background_color_bco(0);
    satuanText[i][1].Set_background_color_bco(0);
    satuanText[i][2].Set_background_color_bco(0);
    supplyText[i].Set_background_color_bco(0);
    ketText[i][0].Set_background_color_bco(0);
    ketText[i][1].Set_background_color_bco(0);
  }
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

    dbSerial.print(masterAddress);
    dbSerial.println(slaveMac);
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
          String receivedData = "";
          while (LoRa.available())
          {
            char c = (char)LoRa.read();
            receivedData += c;
            dbSerial.print(c);
            if (c == '#')
            { // If end marker received
              break;
            }
          }

          // Parsing and storing the received data
          if (receivedData.endsWith("#"))
          {
            receivedData = receivedData.substring(0, receivedData.length() - 1); // Remove the end marker
            int commaIndex = receivedData.indexOf(',');
            if (commaIndex != -1)
            {
              String mac = receivedData.substring(0, commaIndex);
              String values = receivedData.substring(commaIndex + 2);
              int secondComma = values.indexOf(',');
              String value1 = values.substring(0, secondComma);
              values = values.substring(secondComma + 2);
              secondComma = values.indexOf(',');
              String value2 = values.substring(0, secondComma);
              String value3 = values.substring(secondComma + 2);

              // Store the values in the slaveData array
              for (int i = 0; i < numSlaves; i++)
              {
                if (slaveData[i].mac == mac)
                {
                  slaveData[i].supply = value1.toFloat();
                  slaveData[i].leftBank = value2.toFloat();
                  slaveData[i].rightBank = value3.toFloat();
                  break;
                }
              }
            }
          }
        }
      }
    }
    else
    {
      awaitingResponse = false;                                // Reset if timeout occurs
      currentSlaveIndex = (currentSlaveIndex + 1) % numSlaves; // Berpindah ke slave berikutnya
    }
  }
}

void t2Callback()
{
  byte numberValue[6];
  readEEPROMValues(numberValue);

  int currentIndex = 0;
  for (int i = 0; i < 6; i++)
  {
    if (numberValue[i] >= 1 && numberValue[i] <= 6)
    {
      int slaveIndex = numberValue[i] - 1; // Sesuaikan index ke 0-based untuk slaveData

      // Set nilai supply
      snprintf((char *)nilai[currentIndex], sizeof(data0), "%.2f", slaveData[slaveIndex].supply);
      valueText[currentIndex].setText(nilai[currentIndex]);

      // Tampilkan leftBank dan rightBank hanya jika nomor tidak 4 atau 5
      if (numberValue[i] != 4 && numberValue[i] != 5)
      {
        snprintf((char *)data1, sizeof(data1), "%.2f", slaveData[slaveIndex].leftBank);
        snprintf((char *)data2, sizeof(data2), "%.2f", slaveData[slaveIndex].rightBank);

        nilaiText[currentIndex][0].setText(data1);
        nilaiText[currentIndex][1].setText(data2);
      }

      // Tentukan status berdasarkan nilai sensor
      if (slaveData[slaveIndex].supply < 4.00)
      {
        supplyText[currentIndex].Set_background_color_bco(60516); // Low
      }
      else if (slaveData[slaveIndex].supply >= 7.00)
      {
        supplyText[currentIndex].Set_background_color_bco(63488); // High
      }
      else
      {
        supplyText[currentIndex].Set_background_color_bco(2016); // Normal
      }

      currentIndex++;
    }
  }
}

void t3Callback()
{
  handleMQTTConnection();
  // Periksa apakah ada slave yang valid di indeks saat ini
  if (currentSlaveToSend < numSlaves)
  {
    // Ambil data dari slaveData[currentSlaveToSend]
    String mac = slaveData[currentSlaveToSend].mac;
    float supply = slaveData[currentSlaveToSend].supply;
    float leftBank = slaveData[currentSlaveToSend].leftBank;
    float rightBank = slaveData[currentSlaveToSend].rightBank;

    // Konversi data ke format JSON
    String jsonData = convertDataToJson(mac, supply, leftBank, rightBank);
    // Kirim data JSON ke MQTT
    sendToMqtt(mac, jsonData);

    // Beralih ke slave berikutnya pada siklus berikutnya
    currentSlaveToSend = (currentSlaveToSend + 1) % numSlaves;
  }
}

void readAndProcessEEPROMValues(byte *numberValue, int &currentIndex)
{
  readEEPROMValues(numberValue);
  currentIndex = 0;
  for (int i = 0; i < 6; i++)
  {
    if (numberValue[i] >= 1 && numberValue[i] <= 6)
    {
      updateGasUI(currentIndex, numberValue[i]);
      currentIndex++;
    }
  }
}
void setuptombol()
{
  blogin.attachPop(bpPasswordPopCallback, &blogin);
  bpagelogin.attachPop(bPageloginPopCallback, &bpagelogin);
  bsubmit.attachPop(bSubmitPopCallback, &bsubmit);
  btutup.attachPop(bPagehomePopCallback, &btutup);
  bpagehome.attachPop(bPagehomePopCallback, &bpagehome);
  bback1.attachPop(bPagehomePopCallback, &bback1);
  balarmsetting.attachPop(balarmsettingPopCallback, &balarmsetting);
  bsimpan.attachPop(badminpasswordPopCallback, &bsimpan);

  bback.attachPop(bbackPopCallback, &bback);
  bBar.attachPop(bBarPopCallback, &bBar);
  bKPa.attachPop(bKPaPopCallback, &bKPa);
  bPSi.attachPop(bPSiPopCallback, &bPSi);
}

void initializeTask()
{

  runner.init();
  runner.addTask(t1);
  runner.addTask(t2);
  runner.addTask(t3);

  t1.enable();
  t2.enable();
  t3.enable();
}

void setup()
{
  nexInit();
  clearUnusedDisplays();
  initializeTask();
  setupwifimanager();
  setuptombol();
  EEPROM.begin(350);

  byte numberValue[6];
  int currentIndex = 0;
  readAndProcessEEPROMValues(numberValue, currentIndex);

  initializeLoRa();

  // Connect to MQTT
  client.setServer(mqttServer, mqttPort);
  // Perbarui warna tombol berdasarkan status terakhir
  updateButtonColors(lastSelectedButton);

  for (int i = 0; i < numSlaves; i++)
  {
    slaveData[i].mac = slaveMacs[i];
  }
}

void loop()
{
  nexLoop(nex_listen_list);
  runner.execute();

  if (wifimanager_nonblocking)
    wifimanager.process(); // avoid delays() in loop when non-blocking and other long running code
  checkButton();
}
