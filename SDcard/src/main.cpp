// master
#include <SPI.h>
#include <Arduino.h>
#include <Nextion.h>
#include <SD.h>
#include <TaskScheduler.h>
#include <LoRa.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>

const int TRIGGER_PIN = 4;
const int SD_CS_PIN = 32;
const int csPin = 33;    // LoRa radio chip select  *input only eror
const int resetPin = 15; // LoRa radio reset
const int irqPin = 13;   // LoRa radio interrupt
const int buzzerPin = 5;

// Variabel untuk menyimpan data yang dibaca dari SD Card
String displaySettings;
String buttonState;
String snSlaves;
String adminPassword;
String userPassword;
String sensorSettings;

bool wifimanager_nonblocking = true;

const char *mqttServer = "test.mosquitto.org";
const int mqttPort = 1883;

WiFiClient espClient;
PubSubClient client(espClient);
WiFiManager wifimanager;
WiFiManagerParameter custom_field;

const uint8_t masterAddress = 0x10;
const char *slaveMacs[] = {
    "OX", // Oksigen
    "NO", // Nitrous Oxide
    "CO", // Carbon Dioxide
    "KP", // Kompressor
    "VK", // Vakum
    "NG"  // Nitrogen
};

int numSlaves = sizeof(slaveMacs) / sizeof(slaveMacs[0]);

// Waktu tunggu respons dari slave
const long timeout = 300; // 70 ms timeout

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
void taskLoRa();
void taskNextion();
void taskMQTT();
// void t4Callback();

// Task definitions
Task t1(280, TASK_FOREVER, &taskLoRa);
Task t2(2000, TASK_FOREVER, &taskNextion);
Task t3(1000, TASK_FOREVER, &taskMQTT);

int currentSlaveToSend = 0;

String receivedData = "";
uint8_t lastSelectedButton = 0;

int unitState; // 1 = Bar, 2 = KPa, 3 = Psi
int currentState = 0;
int statebutton;

// Satuan, Status, Gas, dan Keterangan
const char *satuan = "Bar";
const char *satuan1 = "mmHg";
const char *satuan2 = "KPa";
const char *satuan3 = "Psi";

const char *status[] = {"Normal", "Low", "Over"};
const char *gas[] = {"Oxygen", "Nitrous Oxide", "Carbondioxide", "Kompressor", "Vakum", "Nitrogen"};

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
NexText tslave5 = NexText(4, 16, "tslave5");
NexText tslave6 = NexText(4, 6, "tslave6");

NexText oksigenMin = NexText(3, 11, "oksigenMin");
NexText oksigenMax = NexText(3, 12, "oksigenMax");
NexText nitrousMin = NexText(3, 13, "nitrousMin");
NexText nitrousMax = NexText(3, 14, "nitrousMax");
NexText karbondioksidaMin = NexText(3, 15, "karbonMin");
NexText karbondioksidaMax = NexText(3, 16, "karbonMax");
NexText kompressorMin = NexText(3, 17, "kompressorMin");
NexText vakumMin = NexText(3, 18, "vakumMin");
NexText nitrogenMin = NexText(3, 19, "nitrogenMin");
NexText nitrogenMax = NexText(3, 20, "nitrogenMax");

NexText tserialNumber = NexText(1, 20, "tserialnumber");

NexText tKet[6] = {
    NexText(0, 57, "tKet0"),
    NexText(0, 58, "tKet1"),
    NexText(0, 59, "tKet2"),
    NexText(0, 60, "tKet3"),
    NexText(0, 61, "tKet4"),
    NexText(0, 62, "tKet5")};

NexText backgorund[6] = {
    NexText(0, 3, "bg0"),
    NexText(0, 2, "bg1"),
    NexText(0, 1, "bg2"),
    NexText(0, 10, "bg3"),
    NexText(0, 9, "bg4"),
    NexText(0, 18, "bg5")};

NexText gasText[6] = {
    NexText(0, 6, "tGas0"),
    NexText(0, 7, "tGas1"),
    NexText(0, 8, "tGas2"),
    NexText(0, 11, "tGas3"),
    NexText(0, 12, "tGas4"),
    NexText(0, 19, "tGas5")};

NexText satuanText[6][3] = {
    {NexText(0, 51, "tSatuan0"), NexText(0, 45, "tsLeft0"), NexText(0, 24, "tsRight0")},
    {NexText(0, 52, "tSatuan1"), NexText(0, 25, "tsLeft1"), NexText(0, 27, "tsRight1")},
    {NexText(0, 53, "tSatuan2"), NexText(0, 29, "tsLeft2"), NexText(0, 31, "tsRight2")},
    {NexText(0, 54, "tSatuan3"), NexText(0, 33, "tsLeft3"), NexText(0, 35, "tsRight3")},
    {NexText(0, 55, "tSatuan4"), NexText(0, 37, "tsLeft4"), NexText(0, 39, "tsRight4")},
    {NexText(0, 56, "tSatuan5"), NexText(0, 41, "tsLeft5"), NexText(0, 43, "tsRight5")}};

NexText nilaiText[6][2] = {
    {NexText(0, 22, "tLeft0"), NexText(0, 23, "tRight0")},
    {NexText(0, 26, "tLeft1"), NexText(0, 28, "tRight1")},
    {NexText(0, 30, "tLeft2"), NexText(0, 32, "tRight2")},
    {NexText(0, 34, "tLeft3"), NexText(0, 36, "tRight3")},
    {NexText(0, 38, "tLeft4"), NexText(0, 40, "tRight4")},
    {NexText(0, 42, "tLeft5"), NexText(0, 44, "tRight5")}};

NexText supplyText[6] = {
    NexText(0, 21, "tSupply0"),
    NexText(0, 46, "tSupply1"),
    NexText(0, 47, "tSupply2"),
    NexText(0, 48, "tSupply3"),
    NexText(0, 49, "tSupply4"),
    NexText(0, 50, "tSupply5")};

NexTouch *nex_listen_list[] = {&bpagelogin, &bpagehome, &settingPage, &settingsn,
                               &bsubmit, &bsimpan, &bback1, &balarmsetting,
                               &blogin, &btutup, &bBar,
                               &bKPa, &bPSi, &bback, &tserialNumber, NULL};

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
void indikatorError(int tipeError)
{
  switch (tipeError)
  {
  case 1: // SDCard tidak terbaca
    digitalWrite(buzzerPin, HIGH);
    delay(400);
    digitalWrite(buzzerPin, LOW);
    delay(10);
    digitalWrite(buzzerPin, HIGH);
    delay(400);
    digitalWrite(buzzerPin, LOW);
    delay(10);
    digitalWrite(buzzerPin, HIGH);
    delay(1000);
    digitalWrite(buzzerPin, LOW);
    delay(10);
    break;
  case 2: // LoRa error
    digitalWrite(buzzerPin, HIGH);
    delay(1000);
    digitalWrite(buzzerPin, LOW);
    delay(10);
    digitalWrite(buzzerPin, HIGH);
    delay(1000);
    digitalWrite(buzzerPin, LOW);
    delay(10);
    digitalWrite(buzzerPin, HIGH);
    delay(400);
    digitalWrite(buzzerPin, LOW);
    delay(10);
    break;
  case 3: // Error normal
    digitalWrite(buzzerPin, HIGH);
    delay(1000);
    digitalWrite(buzzerPin, LOW);
    delay(500);
    break;
  case 4: // Pilih
    digitalWrite(buzzerPin, HIGH);
    delay(100);
    digitalWrite(buzzerPin, LOW);
    delay(100);
    break;
  }
}

void activateSDCard()
{
  digitalWrite(SD_CS_PIN, LOW); // Aktifkan SD Card
  digitalWrite(csPin, HIGH);    // Nonaktifkan LoRa
}

void activateLoRa()
{
  digitalWrite(csPin, LOW);
  digitalWrite(SD_CS_PIN, HIGH); // Nonaktifkan SD Card     // Aktifkan LoRa
}

void setupLoRa()
{
  activateLoRa(); // Nonaktifkan SD Card dan aktifkan LoRa
  LoRa.setPins(csPin, resetPin, irqPin);

  if (LoRa.begin(433E6))
  {
    LoRa.setSpreadingFactor(7);
    LoRa.setSignalBandwidth(125E3);
    LoRa.setCodingRate4(5);
    LoRa.setTxPower(20);
  }
  else
  {
    indikatorError(2);
  }
}

void setupSDCard()
{
  activateSDCard(); // Aktifkan SD Card

  SD.begin(SD_CS_PIN);

  if (!SD.exists("/setting"))
  {
    SD.mkdir("/setting");
  }
}

void writeToSDCard(const char *filename, const String &data)
{
  activateSDCard();
  File file = SD.open("/setting/" + String(filename), FILE_WRITE);
  if (file)
  {
    file.print(data);
    file.close();
    dbSerial.println("Berhasil menulis ke file " + String(filename));
  }
  else
  {
    dbSerial.println("Gagal membuka file " + String(filename));
    indikatorError(1);
  }
}

String readFromSDCard(const char *filename)
{
  activateSDCard();
  File file = SD.open("/setting/" + String(filename));
  if (file)
  {
    String data = file.readString();
    file.close();
    dbSerial.println("Berhasil membaca dari file " + String(filename));
    return data;
  }
  else
  {
    dbSerial.println("Gagal membuka file " + String(filename));
    indikatorError(1);
    return "";
  }
}

void readAllSettingsFromSD()
{
  displaySettings = readFromSDCard("display_settings.txt");
  buttonState = readFromSDCard("button_state.txt");
  snSlaves = readFromSDCard("sn_slaves.txt");
  adminPassword = readFromSDCard("admin_password.txt");
  userPassword = readFromSDCard("user_password.txt");
  sensorSettings = readFromSDCard("sensor_settings.txt");
}

void writePasswordToSD()
{
  uint32_t passworduser;
  setpassworduser.getValue(&passworduser);
  String userPasswordStr = String(passworduser);
  writeToSDCard("user_password.txt", userPasswordStr);
}

void writeSettingsToSD()
{
  String data;
  uint32_t value;

  inputnumber1.getValue(&value);
  data += String(value) + ",";
  inputnumber2.getValue(&value);
  data += String(value) + ",";
  inputnumber3.getValue(&value);
  data += String(value) + ",";
  inputnumber4.getValue(&value);
  data += String(value) + ",";
  inputnumber5.getValue(&value);
  data += String(value) + ",";
  inputnumber6.getValue(&value);
  data += String(value);

  // Simpan pengaturan tampilan ke file terpisah
  writeToSDCard("display_settings.txt", data);
}

void writeSensorSettingsToSD()
{
  dbSerial.println("Menyimpan pengaturan sensor ke SD card...");

  // Buffer ukuran cukup untuk nilai float seperti "123.45" plus terminator null
  char buffer0[6] = {0}, buffer1[6] = {0}, buffer2[6] = {0}, buffer3[6] = {0},
       buffer4[6] = {0}, buffer5[6] = {0}, buffer6[6] = {0}, buffer7[6] = {0},
       buffer8[6] = {0}, buffer9[6] = {0};

  String sensorData = "";

  // Mendapatkan nilai Min dari sensor
  oksigenMin.getText(buffer0, sizeof(buffer0));
  sensorData += String(buffer0) + ","; // Mendapatkan nilai Max dari sensor
  oksigenMax.getText(buffer6, sizeof(buffer6));
  sensorData += String(buffer6) + ",";

  nitrousMin.getText(buffer1, sizeof(buffer1));
  sensorData += String(buffer1) + ",";
  nitrousMax.getText(buffer7, sizeof(buffer7));
  sensorData += String(buffer7) + ",";

  karbondioksidaMin.getText(buffer2, sizeof(buffer2));
  sensorData += String(buffer2) + ",";
  karbondioksidaMax.getText(buffer8, sizeof(buffer8));
  sensorData += String(buffer8) + ",";

  kompressorMin.getText(buffer3, sizeof(buffer3));
  sensorData += String(buffer3) + ",";

  vakumMin.getText(buffer4, sizeof(buffer4));
  sensorData += String(buffer4) + ",";

  nitrogenMin.getText(buffer5, sizeof(buffer5));
  sensorData += String(buffer5) + ",";
  nitrogenMax.getText(buffer9, sizeof(buffer9));
  sensorData += String(buffer9);

  // Menghilangkan koma terakhir jika ada
  if (sensorData.endsWith(","))
  {
    sensorData = sensorData.substring(0, sensorData.length() - 1);
  }

  // Menulis ke SD Card
  dbSerial.println("Menulis data ke SD card...");
  writeToSDCard("sensor_settings.txt", sensorData);
  dbSerial.println("Data sensor telah tersimpan.");
}

void loadDisplaySettings()
{
  if (displaySettings.length() > 0)
  {
    int index = 0;
    String value;

    value = displaySettings.substring(index, displaySettings.indexOf(',', index));
    inputnumber1.setValue(value.toInt());
    index = displaySettings.indexOf(',', index) + 1;

    value = displaySettings.substring(index, displaySettings.indexOf(',', index));
    inputnumber2.setValue(value.toInt());
    index = displaySettings.indexOf(',', index) + 1;

    value = displaySettings.substring(index, displaySettings.indexOf(',', index));
    inputnumber3.setValue(value.toInt());
    index = displaySettings.indexOf(',', index) + 1;

    value = displaySettings.substring(index, displaySettings.indexOf(',', index));
    inputnumber4.setValue(value.toInt());
    index = displaySettings.indexOf(',', index) + 1;

    value = displaySettings.substring(index, displaySettings.indexOf(',', index));
    inputnumber5.setValue(value.toInt());
    index = displaySettings.indexOf(',', index) + 1;

    value = displaySettings.substring(index, displaySettings.indexOf(',', index));
    if (value.length() > 0)
      inputnumber6.setValue(value.toInt());
    index = displaySettings.indexOf(',', index) + 1;

    if (sensorSettings.length() > 0)
    {
      int sensorIndex = 0;
      String sensorValue;

      sensorValue = sensorSettings.substring(sensorIndex, sensorSettings.indexOf(',', sensorIndex));
      oksigenMin.setText(sensorValue.c_str());
      sensorIndex = sensorSettings.indexOf(',', sensorIndex) + 1;

      sensorValue = sensorSettings.substring(sensorIndex, sensorSettings.indexOf(',', sensorIndex));
      oksigenMax.setText(sensorValue.c_str());
      sensorIndex = sensorSettings.indexOf(',', sensorIndex) + 1;

      sensorValue = sensorSettings.substring(sensorIndex, sensorSettings.indexOf(',', sensorIndex));
      nitrousMin.setText(sensorValue.c_str());
      sensorIndex = sensorSettings.indexOf(',', sensorIndex) + 1;

      sensorValue = sensorSettings.substring(sensorIndex, sensorSettings.indexOf(',', sensorIndex));
      nitrousMax.setText(sensorValue.c_str());
      sensorIndex = sensorSettings.indexOf(',', sensorIndex) + 1;

      sensorValue = sensorSettings.substring(sensorIndex, sensorSettings.indexOf(',', sensorIndex));
      karbondioksidaMin.setText(sensorValue.c_str());
      sensorIndex = sensorSettings.indexOf(',', sensorIndex) + 1;

      sensorValue = sensorSettings.substring(sensorIndex, sensorSettings.indexOf(',', sensorIndex));
      karbondioksidaMax.setText(sensorValue.c_str());
      sensorIndex = sensorSettings.indexOf(',', sensorIndex) + 1;

      sensorValue = sensorSettings.substring(sensorIndex, sensorSettings.indexOf(',', sensorIndex));
      kompressorMin.setText(sensorValue.c_str());
      sensorIndex = sensorSettings.indexOf(',', sensorIndex) + 1;

      sensorValue = sensorSettings.substring(sensorIndex, sensorSettings.indexOf(',', sensorIndex));
      vakumMin.setText(sensorValue.c_str());
      sensorIndex = sensorSettings.indexOf(',', sensorIndex) + 1;

      sensorValue = sensorSettings.substring(sensorIndex, sensorSettings.indexOf(',', sensorIndex));
      nitrogenMin.setText(sensorValue.c_str());
      sensorIndex = sensorSettings.indexOf(',', sensorIndex) + 1;

      sensorValue = sensorSettings.substring(sensorIndex);
      nitrogenMax.setText(sensorValue.c_str());
    }
  }

  if (adminPassword.length() > 0)
  {
    setpasswordadmin.setValue(adminPassword.toInt());
  }

  if (userPassword.length() > 0)
  {
    setpassworduser.setValue(userPassword.toInt());
  }
}

// Fungsi untuk memfilter karakter non-printable
String filterNonPrintable(String input)
{
  String output = "";
  for (unsigned int i = 0; i < input.length(); i++)
  {
    char c = input.charAt(i);
    if (isPrintable(c))
    {
      output += c;
    }
  }
  return output;
}

void writeSNslave()
{
  String data;
  char buffer0[10] = {0}; // Inisialisasi buffer ke nilai 0
  char buffer1[10] = {0};
  char buffer2[10] = {0};
  char buffer3[10] = {0};
  char buffer4[10] = {0};
  char buffer5[10] = {0};

  tslave1.getText(buffer0, sizeof(buffer0));
  if (strlen(buffer0) > 0)
    data += String(buffer0) + ",";

  tslave2.getText(buffer1, sizeof(buffer1));
  if (strlen(buffer1) > 0)
    data += String(buffer1) + ",";

  tslave3.getText(buffer2, sizeof(buffer2));
  if (strlen(buffer2) > 0)
    data += String(buffer2) + ",";

  tslave4.getText(buffer3, sizeof(buffer3));
  if (strlen(buffer3) > 0)
    data += String(buffer3) + ",";

  tslave5.getText(buffer4, sizeof(buffer4));
  if (strlen(buffer4) > 0)
    data += String(buffer4) + ",";

  tslave6.getText(buffer5, sizeof(buffer5));
  if (strlen(buffer5) > 0)
    data += String(buffer5);

  // Hapus koma terakhir jika ada
  if (data.endsWith(","))
  {
    data = data.substring(0, data.length() - 1);
  }

  // Filter data dari karakter non-printable sebelum menulis ke SD Card
  data = filterNonPrintable(data);

  if (!data.isEmpty())
  {
    writeToSDCard("sn_slaves.txt", data);
  }
}

void loadSNslave()
{
  int index = 0;
  int nextComma;
  String value;

  // Fungsi untuk mengambil nilai dan memperbarui indeks
  auto getNextValue = [&]()
  {
    nextComma = snSlaves.indexOf(',', index);
    if (nextComma == -1)
    {
      value = snSlaves.substring(index);
      index = snSlaves.length();
    }
    else
    {
      value = snSlaves.substring(index, nextComma);
      index = nextComma + 1;
    }
    return value;
  };

  // Mengatur nilai untuk setiap slave
  tslave1.setText(getNextValue().c_str());
  if (index < snSlaves.length())
    tslave2.setText(getNextValue().c_str());
  if (index < snSlaves.length())
    tslave3.setText(getNextValue().c_str());
  if (index < snSlaves.length())
    tslave4.setText(getNextValue().c_str());
  if (index < snSlaves.length())
    tslave5.setText(getNextValue().c_str());
  if (index < snSlaves.length())
    tslave6.setText(getNextValue().c_str());
}

void handleMQTTConnection()
{
  if (!client.connected())
  {
    int attempts = 0;
    while (!client.connected() && attempts < 3)
    {
      if (client.connect("ESP32Master"))
      {
        break;
      }
      else
      {
        attempts++;
      }
    }
    if (!client.connected())
    {
      return;
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
  wifimanager.setConfigPortalTimeout(120); // auto close configportal after n seconds
  wifimanager.setCaptivePortalEnable(true);
  bool res;
  res = wifimanager.autoConnect("Master Alarm", "");
}

void checkButton()
{
  // check for button press
  if (digitalRead(TRIGGER_PIN) == HIGH)
  {
    digitalWrite(TRIGGER_PIN, LOW);
    dbSerial.println("Buzzer OFF");
    delay(50);
    if (digitalRead(TRIGGER_PIN) == HIGH)
    {
      dbSerial.println("Button Pressed");
      delay(3000);
      if (digitalRead(TRIGGER_PIN) == HIGH)
      {
        wifimanager.resetSettings();
      }

      wifimanager.setConfigPortalTimeout(120);

      if (!wifimanager.startConfigPortal("Master Alarm", ""))
      {
        delay(3000);
        // ESP.restart();
      }
    }
  }
}

void saveButtonState(uint8_t buttonNumber)
{
  writeToSDCard("button_state.txt", String(buttonNumber));
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

  int sandi1 = adminPassword.toInt();
  int sandi2 = userPassword.toInt();

  char passwordValue[6];

  // Mengambil teks dari input Nextion
  password.getText(passwordValue, sizeof(passwordValue)); // Pastikan password adalah objek Nextion Text input

  // Konversi teks menjadi integer untuk perbandingan
  int passwordInt = atoi(passwordValue);

  // Verifikasi password untuk admin
  if (passwordInt == sandi1)
  {
    password.setText("");
    settingsn.show();
    // Arahkan ke halaman settings
    loadSNslave();
    loadDisplaySettings(); // Baca nilai dari SD Card
  }
  // Verifikasi password untuk user
  else if (passwordInt == sandi2)
  {
    password.setText("");
    settingPage.show();    // Arahkan ke halaman settingPage
    loadDisplaySettings(); // Baca nilai dari SD Card
  }
  else
  {
    password.setText(""); // Kosongkan input password
  }
}

void bPageloginPopCallback(void *ptr) // menu setting home
{
  indikatorError(4);
  t1.disable();
  t2.disable();
  loginpage.show();
}

void bSubmitPopCallback(void *ptr) // ini simpan tampilan, satuan, dan password
{
  indikatorError(4);
  writePasswordToSD();
  writeSettingsToSD();
}

void bpPasswordPopCallback(void *ptr) // button submit password
{
  indikatorError(4);
  verifikasilogin();
}

void badminpasswordPopCallback(void *ptr)
{
  indikatorError(4);
  uint32_t passwordadmin;

  writeSNslave();
  setpasswordadmin.getValue(&passwordadmin);
  String adminPasswordStr = String(passwordadmin);
  writeToSDCard("admin_password.txt", adminPasswordStr);
}

void balarmsettingPopCallback(void *ptr)
{
  indikatorError(4);
  loadDisplaySettings();
  alarmpage.show();
}

void bbackPopCallback(void *ptr)
{
  indikatorError(4);
  writeSensorSettingsToSD();
  settingPage.show();
}

void bPagehomePopCallback(void *ptr)
{
  indikatorError(4);
  home.show();
  esp_restart();
  clearinputnilai();
}

void bBarPopCallback(void *ptr)
{
  indikatorError(4);
  lastSelectedButton = 1;
  saveButtonState(lastSelectedButton);
}

void bKPaPopCallback(void *ptr)
{
  indikatorError(4);
  lastSelectedButton = 2;
  saveButtonState(lastSelectedButton);
}

void bPSiPopCallback(void *ptr)
{
  indikatorError(4);
  lastSelectedButton = 3;
  saveButtonState(lastSelectedButton);
}

void updateGasUI(int currentIndex, byte gasType)
{
  byte currentIndexByte = (byte)currentIndex;

  const char *selectedSatuan;

  switch (buttonState.toInt())
  {
  case 2:
    selectedSatuan = satuan2;
    break;
  case 3:
    selectedSatuan = satuan3;
    break;
  default:
    selectedSatuan = satuan;
  }

  // Setel gambar latar belakang
  backgorund[currentIndex].Set_background_image_pic(6);

  // Setel teks gas
  gasText[currentIndex].setText(gas[gasType - 1]);

  switch (gasType)
  {
  case 1: // Oxygen
  case 2: // Nitrous Oxide
  case 3: // Carbondioxide
  case 6: // Nitrogen
    satuanText[currentIndex][0].setText(selectedSatuan);
    satuanText[currentIndex][1].setText(selectedSatuan);
    satuanText[currentIndex][2].setText(selectedSatuan);
    break;

  case 4: // Kompressor
    satuanText[currentIndex][0].setText(selectedSatuan);
    break;

  case 5: // Vakum
    satuanText[currentIndex][0].setText(satuan1);
    break;
  }

  uint16_t backgroundColor = 14791;

  nilaiText[currentIndex][0].Set_background_color_bco(backgroundColor);
  nilaiText[currentIndex][1].Set_background_color_bco(backgroundColor);

  supplyText[currentIndex].Set_background_color_bco(backgroundColor);

  satuanText[currentIndex][0].Set_background_color_bco(backgroundColor);
  satuanText[currentIndex][1].Set_background_color_bco(backgroundColor);
  satuanText[currentIndex][2].Set_background_color_bco(backgroundColor);
}

void clearUnusedDisplays()
{
  gasText[0].setText("-");
  gasText[1].setText("-");
  gasText[2].setText("-");
  gasText[3].setText("-");
  gasText[4].setText("-");
  gasText[5].setText("-");
}

void processReceivedData(const String &receivedData, int currentSlaveIndex)
{
  dbSerial.println("Received raw data: " + receivedData);

  String cleanedData = receivedData;
  cleanedData.trim();
  if (cleanedData.startsWith(","))
  {
    cleanedData = cleanedData.substring(1);
  }

  int firstComma = cleanedData.indexOf(',');
  int secondComma = cleanedData.indexOf(',', firstComma + 1);
  int thirdComma = cleanedData.indexOf(',', secondComma + 1);

  String mac = cleanedData.substring(0, firstComma);
  mac.trim();

  float supply = 0.0;
  float leftBank = 0.0;
  float rightBank = 0.0;

  if (firstComma != -1 && secondComma != -1)
  {
    supply = cleanedData.substring(firstComma + 1, secondComma).toFloat();
  }

  if (!(mac.startsWith("KP") || mac.startsWith("VK")))
  {
    if (secondComma != -1 && thirdComma != -1)
    {
      leftBank = cleanedData.substring(secondComma + 1, thirdComma).toFloat();
      rightBank = cleanedData.substring(thirdComma + 1).toFloat();
    }
  }
  else
  {
    if (firstComma != -1)
    {
      supply = cleanedData.substring(firstComma + 1).toFloat();
    }
  }
  slaveData[currentSlaveIndex].mac = mac;
  slaveData[currentSlaveIndex].supply = supply;
  slaveData[currentSlaveIndex].leftBank = leftBank;
  slaveData[currentSlaveIndex].rightBank = rightBank;
}

void taskLoRa()
{
  static int currentSlaveIndex = 0;
  static bool awaitingResponse = false;
  static unsigned long requestTime = 0;

  if (!awaitingResponse && millis() - requestTime >= timeout)
  {
    int commaIndex = -1;
    for (int i = 0; i < currentSlaveIndex; i++)
    {
      commaIndex = snSlaves.indexOf(',', commaIndex + 1);
    }
    String currentSlaveMac = snSlaves.substring(commaIndex + 1, snSlaves.indexOf(',', commaIndex + 1));
    currentSlaveMac.trim();

    if (LoRa.begin(433E6))
    {
      activateLoRa();
      LoRa.beginPacket();
      LoRa.write(masterAddress);
      LoRa.print(", ");
      LoRa.print(currentSlaveMac);
      LoRa.endPacket(true);

      LoRa.receive();
      dbSerial.println("Kirim permintaan ke slave: " + currentSlaveMac);

      awaitingResponse = true;
      requestTime = millis();
    }
    else
    {
      dbSerial.println("Koneksi LoRa terputus, mencoba menginisialisasi ulang...");
      setupLoRa();
      indikatorError(2);
    }
  }

  if (awaitingResponse && millis() - requestTime < timeout)
  {
    int packetSize = LoRa.parsePacket();
    if (packetSize)
    {
      uint8_t receivedAddress = LoRa.read();
      if (receivedAddress == masterAddress)
      {
        String receivedData = LoRa.readString();
        receivedData.trim();
        processReceivedData(receivedData, currentSlaveIndex);

        awaitingResponse = false;
        currentSlaveIndex = (currentSlaveIndex + 1) % numSlaves;
      }
    }
  }
  else if (awaitingResponse && millis() - requestTime >= timeout)
  {
    awaitingResponse = false;
    currentSlaveIndex = (currentSlaveIndex + 1) % numSlaves;
  }
}

void taskNextion()
{
  byte numberValue[6];
  char buffer[10]; // Buffer untuk menyimpan teks dari SD card

  int index = 0;
  for (int i = 0; i < 6; i++)
  {
    int commaIndex = displaySettings.indexOf(',', index);
    if (commaIndex == -1 && i < 5)
    {
      // Error handling jika format tidak sesuai
      dbSerial.println("Error: Format display_settings.txt tidak valid");
      return;
    }
    numberValue[i] = displaySettings.substring(index, commaIndex).toInt();
    index = commaIndex + 1;
  }

  const char *macPrefixes[] = {"OX", "NO", "CO", "KP", "VK", "NG"};
  int displayIndex = 0;

  // Baca satuan yang dipilih dari SD card
  String selectedUnit = buttonState.substring(0, buttonState.indexOf(','));

  for (int i = 0; i < 6; i++)
  {
    if (numberValue[i] >= 1 && numberValue[i] <= 6)
    {
      int gasType = numberValue[i] - 1;
      const char *macPrefix = macPrefixes[gasType];
      bool found = false;

      for (int j = 0; j < numSlaves; j++)
      {
        if (slaveData[j].mac.startsWith(macPrefix))
        {
          char supply[10], left[10], right[10];

          // Konversi nilai supply berdasarkan satuan yang dipilih
          float supplyValue = slaveData[j].supply;
          if (selectedUnit == "3")
          {
            supplyValue *= 14.5038;                                // Konversi dari bar ke psi
            snprintf(supply, sizeof(supply), "%.0f", supplyValue); // Tampilkan nilai bulat
          }
          else if (selectedUnit == "2")
          {
            supplyValue *= 100;                                    // Konversi dari bar ke kPa
            snprintf(supply, sizeof(supply), "%.0f", supplyValue); // Tampilkan 3 digit
          }
          else
          {
            snprintf(supply, sizeof(supply), "%.2f", supplyValue); // Default bar
          }

          supplyText[displayIndex].setText(supply);

          if (strcmp(macPrefix, "KP") != 0 && strcmp(macPrefix, "VK") != 0)
          {
            float leftValue = slaveData[j].leftBank;
            float rightValue = slaveData[j].rightBank;

            if (selectedUnit == "3")
            {
              leftValue *= 14.5038;                               // Konversi dari bar ke psi
              rightValue *= 14.5038;                              // Konversi dari bar ke psi
              snprintf(left, sizeof(left), "%.0f", leftValue);    // Tampilkan nilai bulat
              snprintf(right, sizeof(right), "%.0f", rightValue); // Tampilkan nilai bulat
            }
            else if (selectedUnit == "2")
            {
              leftValue *= 100;                                   // Konversi dari bar ke kPa
              rightValue *= 100;                                  // Konversi dari bar ke kPa
              snprintf(left, sizeof(left), "%.0f", leftValue);    // Tampilkan 3 digit
              snprintf(right, sizeof(right), "%.0f", rightValue); // Tampilkan 3 digit
            }
            else
            {
              snprintf(left, sizeof(left), "%.2f", leftValue);    // Default bar
              snprintf(right, sizeof(right), "%.2f", rightValue); // Default bar
            }

            nilaiText[displayIndex][0].setText(left);
            nilaiText[displayIndex][1].setText(right);
          }

          found = true;

          // Ambil nilai min dan max dari sensorSettings yang disimpan di SD card
          float minVal = 0.0, maxVal = 0.0;
          int sensorIndex = 0;
          String sensorValue0, sensorValue1, sensorValue2, sensorValue3, sensorValue4, sensorValue5, sensorValue6, sensorValue7, sensorValue8, sensorValue9;

          switch (gasType)
          {
          case 0: // Oksigen
            sensorValue0 = sensorSettings.substring(sensorIndex, sensorSettings.indexOf(',', sensorIndex));
            minVal = sensorValue0.toFloat();
            sensorIndex = sensorSettings.indexOf(',', sensorIndex) + 1;
            sensorValue1 = sensorSettings.substring(sensorIndex, sensorSettings.indexOf(',', sensorIndex));
            maxVal = sensorValue1.toFloat();
            sensorIndex = sensorSettings.indexOf(',', sensorIndex) + 1;
            break;
          case 1: // Nitrous Oxide
            sensorValue2 = sensorSettings.substring(sensorIndex, sensorSettings.indexOf(',', sensorIndex));
            minVal = sensorValue2.toFloat();
            sensorIndex = sensorSettings.indexOf(',', sensorIndex) + 1;
            sensorValue3 = sensorSettings.substring(sensorIndex, sensorSettings.indexOf(',', sensorIndex));
            maxVal = sensorValue3.toFloat();
            sensorIndex = sensorSettings.indexOf(',', sensorIndex) + 1;
            break;
          case 2: // Carbon Dioxide
            sensorValue4 = sensorSettings.substring(sensorIndex, sensorSettings.indexOf(',', sensorIndex));
            minVal = sensorValue4.toFloat();
            sensorIndex = sensorSettings.indexOf(',', sensorIndex) + 1;
            sensorValue5 = sensorSettings.substring(sensorIndex, sensorSettings.indexOf(',', sensorIndex));
            maxVal = sensorValue5.toFloat();
            sensorIndex = sensorSettings.indexOf(',', sensorIndex) + 1;
            break;
          case 3: // Kompressor
            sensorValue6 = sensorSettings.substring(sensorIndex, sensorSettings.indexOf(',', sensorIndex));
            minVal = sensorValue6.toFloat();
            sensorIndex = sensorSettings.indexOf(',', sensorIndex) + 1;
            break;
          case 4: // Vakum
            sensorValue7 = sensorSettings.substring(sensorIndex, sensorSettings.indexOf(',', sensorIndex));
            minVal = sensorValue7.toFloat();
            sensorIndex = sensorSettings.indexOf(',', sensorIndex) + 1;
            break;
          case 5: // Nitrogen
            sensorValue8 = sensorSettings.substring(sensorIndex, sensorSettings.indexOf(',', sensorIndex));
            minVal = sensorValue8.toFloat();
            sensorIndex = sensorSettings.indexOf(',', sensorIndex) + 1;
            sensorValue9 = sensorSettings.substring(sensorIndex, sensorSettings.indexOf(',', sensorIndex));
            maxVal = sensorValue9.toFloat();
            sensorIndex = sensorSettings.indexOf(',', sensorIndex) + 1;
            break;
          }

          // Konversi nilai min dan max berdasarkan satuan yang dipilih
          if (selectedUnit == "3")
          {
            minVal *= 14.5038; // Konversi dari bar ke psi
            maxVal *= 14.5038; // Konversi dari bar ke psi
          }
          else if (selectedUnit == "2")
          {
            minVal *= 100; // Konversi dari bar ke kPa
            maxVal *= 100; // Konversi dari bar ke kPa
          }

          // Set background image based on min and max values
          if (supplyValue < minVal)
          {
            // indikatorError(3);
            tKet[displayIndex].Set_background_image_pic(17); // Low
          }
          else if (supplyValue > maxVal)
          {
            // indikatorError(3);
            tKet[displayIndex].Set_background_image_pic(15); // High
          }
          else
          {
            tKet[displayIndex].Set_background_image_pic(16); // Normal
          }

          displayIndex++;
          break;
        }
      }

      if (!found)
      {
        supplyText[displayIndex].setText("Err");
        if (strcmp(macPrefix, "KP") != 0 && strcmp(macPrefix, "VK") != 0)
        {
          nilaiText[displayIndex][0].setText("Err");
          nilaiText[displayIndex][1].setText("Err");
        }
        else
        {
          nilaiText[displayIndex][0].setText("Err");
          nilaiText[displayIndex][1].setText("Err");
        }
        indikatorError(4);
        displayIndex++;
      }
    }
  }
}

void taskMQTT()
{
  handleMQTTConnection();
  if (client.connected())
  {
    if (currentSlaveToSend < numSlaves)
    {
      // Ambil data dari slaveData[currentSlaveToSend]
      String mac = slaveData[currentSlaveToSend].mac;
      float supply = slaveData[currentSlaveToSend].supply;
      float leftBank = slaveData[currentSlaveToSend].leftBank;
      float rightBank = slaveData[currentSlaveToSend].rightBank;

      String jsonData = convertDataToJson(mac, supply, leftBank, rightBank);
      sendToMqtt(mac, jsonData);

      currentSlaveToSend = (currentSlaveToSend + 1) % numSlaves;
    }
  }
}

void readAndProcessValues(byte *numberValue, int &currentIndex)
{
  currentIndex = 0;
  int index = 0;
  for (int i = 0; i < 6; i++)
  {
    int commaIndex = displaySettings.indexOf(',', index);
    if (commaIndex == -1 && i == 5)
    {
      // Untuk elemen terakhir
      numberValue[i] = displaySettings.substring(index).toInt();
    }
    else if (commaIndex != -1)
    {
      numberValue[i] = displaySettings.substring(index, commaIndex).toInt();
      index = commaIndex + 1;
    }
    else
    {
      numberValue[i] = 0;
    }

    if (numberValue[i] >= 1 && numberValue[i] <= 6)
    {
      updateGasUI(currentIndex, numberValue[i]);
      currentIndex++;
    }
  }
}

void setupNextiontombol()
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

void setup()
{
  nexInit();
  clearUnusedDisplays();
  setupSDCard();
  delay(100);
  pinMode(buzzerPin, OUTPUT);
  pinMode(TRIGGER_PIN, INPUT);

  digitalWrite(buzzerPin, LOW);
  runner.init();
  runner.addTask(t1);
  runner.addTask(t2);
  runner.addTask(t3);
  t1.enable();
  t2.enable();
  t3.enable();

  SPI.begin();

  setupNextiontombol();
  setupwifimanager();

  readAllSettingsFromSD();
  delay(100);
  // Inisialisasi MQTT
  client.setServer(mqttServer, mqttPort);

  for (int i = 0; i < numSlaves; i++)
  {
    slaveData[i].mac = slaveMacs[i];
  }

  byte numberValue[6];
  int currentIndex = 0;
  readAndProcessValues(numberValue, currentIndex);

  activateLoRa();
  setupLoRa();
}

void loop()
{
  checkButton();
  nexLoop(nex_listen_list);
  runner.execute();

  wifimanager.process();
}