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
#include <otadrive_esp.h>
// #include "RTClib.h"
// #include <esp_task_wdt.h>
#define PIN_LORA_CS 33
#define PIN_LORA_RST 15
#define PIN_LORA_DIO0 13
#define LORA_FREQUENCY 433E6

#define TRIGGER_PIN 4
#define SD_CS_PIN 32

#define BUZZER_PIN 5

String displaySettings;
String buttonState;
String snSlaves;
String adminPassword;
String userPassword;
String sensorSettings;
char previousSupply[6][10]; // Buffer untuk menyimpan nilai supply yang terakhir dikirim
char previousLeft[6][10];   // Buffer untuk menyimpan nilai left yang terakhir dikirim
char previousRight[6][10];  // Buffer untuk menyimpan nilai right yang terakhir dikirim

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

const long timeout = 500;
struct SlaveData
{
  String mac;
  float supply;
  float leftBank;
  float rightBank;
};

SlaveData slaveData[6];

Scheduler runner;
void taskLoRa();
void taskNextion();
void taskMQTT();
// void t4Callback();

Task t1(560, TASK_FOREVER, &taskLoRa);
Task t2(1500, TASK_FOREVER, &taskNextion);
Task t3(1000, TASK_FOREVER, &taskMQTT);

int currentSlaveToSend = 0;

String receivedData = "";
uint8_t lastSelectedButton = 0;

int unitState;
int currentState = 0;
int statebutton;

const char *satuan = "Bar";
const char *satuan1 = "mmHg";
const char *satuan2 = "KPa";
const char *satuan3 = "Psi";

const char *status[] = {"Normal", "Low", "Over"};
const char *gas[] = {"Oxygen", "Nitrous Oxide", "Carbondioxide", "Kompressor", "Vakum", "Nitrogen"};

NexButton bpagelogin = NexButton(0, 19, "bpagelogin");       // sudah
NexButton bpagehome = NexButton(1, 15, "bpagehome");         // sudah
NexButton bsubmit = NexButton(1, 17, "bsubmit");             // sudah
NexButton balarmsetting = NexButton(1, 16, "balarmsetting"); // sudah
NexButton blogin = NexButton(2, 2, "blogin");                // sudah
NexButton btutup = NexButton(2, 3, "btutup");                // sudah
NexButton bsubmit1 = NexButton(3, 19, "bsubmit");            // sudah
NexButton bsimpan = NexButton(4, 15, "bsimpan");             // sudah
NexButton bback = NexButton(4, 16, "btutup");                // sudah

NexDSButton bBar = NexDSButton(1, 18, "bBar"); // sudah
NexDSButton bKPa = NexDSButton(1, 19, "bKPa"); // sudah
NexDSButton bPSi = NexDSButton(1, 20, "bPSi"); // sudah

NexPage home = NexPage(0, 0, "mainmenu");               // sudah
NexPage settingPage = NexPage(1, 0, "menusetting");     // sudah
NexPage loginpage = NexPage(2, 0, "loginpage");         // sudah
NexPage alarmpage = NexPage(3, 0, "settingalarm");      // sudah
NexPage settingsn = NexPage(4, 0, "settingsn");         // sudah
NexPage setupPassword = NexPage(5, 0, "setupPassword"); // sudah

NexText password = NexText(2, 1, "tpassword"); // sudah

NexNumber setpassworduser = NexNumber(1, 13, "npassworduser");  // sudah
NexNumber setpasswordadmin = NexNumber(4, 7, "npasswordadmin"); // sudah
NexNumber setAdmin = NexNumber(5, 3, "npasswordAdmin");         // sudah
NexNumber setUser = NexNumber(5, 4, "npasswordUser");           // sudah

NexButton bsave = NexButton(5, 5, "bsave"); // sudah

NexNumber inputnumber1 = NexNumber(1, 1, "n0"); // sudah
NexNumber inputnumber2 = NexNumber(1, 2, "n1"); // sudah
NexNumber inputnumber3 = NexNumber(1, 3, "n2"); // sudah
NexNumber inputnumber4 = NexNumber(1, 4, "n3"); // sudah
NexNumber inputnumber5 = NexNumber(1, 5, "n4"); // sudah
NexNumber inputnumber6 = NexNumber(1, 6, "n5"); // sudah

NexText warning = NexText(2, 1, "twarning");
NexText tloginAdmin = NexText(4, 17, "tloginAdmin");
NexText tloginUser = NexText(1, 23, "tloginUser");

NexText tslave1 = NexText(4, 1, "tslave1"); // sudah
NexText tslave2 = NexText(4, 2, "tslave2"); // sudah
NexText tslave3 = NexText(4, 3, "tslave3"); // sudah
NexText tslave4 = NexText(4, 4, "tslave4"); // sudah
NexText tslave5 = NexText(4, 5, "tslave5"); // sudah
NexText tslave6 = NexText(4, 6, "tslave6"); // sudah

NexText oksigenMin = NexText(3, 1, "oksigenMin");
NexText oksigenMax = NexText(3, 2, "oksigenMax");
NexText nitrousMin = NexText(3, 3, "nitrousMin");
NexText nitrousMax = NexText(3, 4, "nitrousMax");
NexText karbondioksidaMin = NexText(3, 5, "karbonMin");
NexText karbondioksidaMax = NexText(3, 6, "karbonMax");
NexText kompressorMin = NexText(3, 7, "kompressorMin");
NexText vakumMin = NexText(3, 8, "kompressorMax");
NexText nitrogenMin = NexText(3, 9, "vakumMin");
NexText nitrogenMax = NexText(3, 10, "vakumMax");
NexText kompressorMax = NexText(3, 20, "nitrogenMin");
NexText vakumMax = NexText(3, 21, "nitrogenMax");

NexText tserialNumber = NexText(1, 20, "tserialnumber");
NexText tStatus[6] = {
    NexText(0, 63, "tStatus0"),
    NexText(0, 64, "tStatus1"),
    NexText(0, 65, "tStatus2"),
    NexText(0, 66, "tStatus3"),
    NexText(0, 67, "tStatus4"),
    NexText(0, 68, "tStatus5")};
NexText tKet[6] = {
    NexText(0, 52, "tKet0"),
    NexText(0, 52, "tKet1"),
    NexText(0, 53, "tKet2"),
    NexText(0, 54, "tKet3"),
    NexText(0, 55, "tKet4"),
    NexText(0, 56, "tKet5")};

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
                               &bsubmit, &bsubmit1, &bsimpan, &balarmsetting,
                               &blogin, &btutup, &bBar,
                               &bKPa, &bPSi, &bsave, &bback, &tserialNumber, NULL};

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
void clearUnusedDisplays()
{
  gasText[0].setText("-");
  gasText[1].setText("-");
  gasText[2].setText("-");
  gasText[3].setText("-");
  gasText[4].setText("-");
  gasText[5].setText("-");
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

  backgorund[currentIndex].Set_background_image_pic(12);

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
void indikator(int tipeIndikator)
{
  switch (tipeIndikator)
  {
  case 1:
    warning.setText("Password salah!!!");
    delay(3000);
    warning.setText("");
    break;
  case 2:
    warning.setText("Anda sudah login User!!!");
    delay(3000);
    warning.setText("");
    break;
  case 3:
    warning.setText("Anda sudah login Admin!!!");
    delay(3000);
    warning.setText("");
    break;
  }
}
void indikatorBuzzer(int tipeError)
{
  switch (tipeError)
  {
  case 1:
    digitalWrite(BUZZER_PIN, HIGH);
    delay(100);
    digitalWrite(BUZZER_PIN, LOW);
    delay(10);
    digitalWrite(BUZZER_PIN, HIGH);
    delay(100);
    digitalWrite(BUZZER_PIN, LOW);
    delay(10);
    digitalWrite(BUZZER_PIN, HIGH);
    delay(400);
    digitalWrite(BUZZER_PIN, LOW);
    delay(10);
    break;
  case 2:
    digitalWrite(BUZZER_PIN, HIGH);
    delay(1000);
    digitalWrite(BUZZER_PIN, LOW);
    delay(110);
    digitalWrite(BUZZER_PIN, HIGH);
    delay(1000);
    digitalWrite(BUZZER_PIN, LOW);
    delay(110);
    digitalWrite(BUZZER_PIN, HIGH);
    delay(400);
    digitalWrite(BUZZER_PIN, LOW);
    delay(110);
    break;
  case 3:
    digitalWrite(BUZZER_PIN, HIGH);
    delay(90);
    digitalWrite(BUZZER_PIN, LOW);
    delay(20);
    digitalWrite(BUZZER_PIN, HIGH);
    delay(90);
    digitalWrite(BUZZER_PIN, LOW);
    delay(20);
    digitalWrite(BUZZER_PIN, HIGH);
    delay(90);
    digitalWrite(BUZZER_PIN, LOW);
    break;
  case 4:
    digitalWrite(BUZZER_PIN, HIGH);
    delay(100);
    digitalWrite(BUZZER_PIN, LOW);
    delay(100);
    break;
  }
}

void activateSDCard()
{
  digitalWrite(SD_CS_PIN, LOW);
  digitalWrite(PIN_LORA_CS, HIGH);
}

void activateLoRa()
{
  digitalWrite(PIN_LORA_CS, LOW);
  digitalWrite(SD_CS_PIN, HIGH);
}

void setupLoRa()
{
  activateLoRa();
  LoRa.setPins(PIN_LORA_CS, PIN_LORA_RST, PIN_LORA_DIO0);

  if (LoRa.begin(LORA_FREQUENCY))
  {
    LoRa.setSpreadingFactor(7);
    LoRa.setSignalBandwidth(125E3);
    LoRa.setCodingRate4(5);
    LoRa.setTxPower(20);
    dbSerial.println("LoRa berhasil diinisialisasi");
  }
  else
  {
    dbSerial.println("Gagal menginisialisasi LoRa");
    // indikatorBuzzer(2);
  }
}

void setupSDCard()
{
  activateSDCard();

  if (SD.begin(SD_CS_PIN))
  {
    dbSerial.println("SD Card berhasil diinisialisasi");
  }
  else
  {
    dbSerial.println("Gagal menginisialisasi SD Card");
    // indikatorBuzzer(1);
  }

  if (!SD.exists("/setting"))
  {
    SD.mkdir("/setting");
    dbSerial.println("Folder /setting berhasil dibuat");
  }
  else
  {
    dbSerial.println("Folder /setting sudah ada");
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
    // indikatorBuzzer(1);
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
    // indikatorBuzzer(1);
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

  writeToSDCard("display_settings.txt", data);
}

void writeSensorSettingsToSD()
{
  dbSerial.println("Menyimpan pengaturan sensor ke SD card...");

  char buffer0[6] = {0}, buffer1[6] = {0}, buffer2[6] = {0}, buffer3[6] = {0},
       buffer4[6] = {0}, buffer5[6] = {0}, buffer6[6] = {0}, buffer7[6] = {0},
       buffer8[6] = {0}, buffer9[6] = {0}, buffer10[6] = {0}, buffer11[6] = {0};

  String sensorData = "";

  oksigenMin.getText(buffer0, sizeof(buffer0));
  sensorData += String(buffer0) + ",";
  oksigenMax.getText(buffer1, sizeof(buffer1));
  sensorData += String(buffer1) + ",";

  nitrousMin.getText(buffer2, sizeof(buffer2));
  sensorData += String(buffer2) + ",";
  nitrousMax.getText(buffer3, sizeof(buffer3));
  sensorData += String(buffer3) + ",";

  karbondioksidaMin.getText(buffer4, sizeof(buffer4));
  sensorData += String(buffer4) + ",";
  karbondioksidaMax.getText(buffer5, sizeof(buffer5));
  sensorData += String(buffer5) + ",";

  kompressorMin.getText(buffer6, sizeof(buffer6));
  sensorData += String(buffer6) + ",";
  kompressorMax.getText(buffer7, sizeof(buffer7));
  sensorData += String(buffer7) + ",";

  vakumMin.getText(buffer8, sizeof(buffer8));
  sensorData += String(buffer8) + ",";
  vakumMax.getText(buffer9, sizeof(buffer9));
  sensorData += String(buffer9) + ",";

  nitrogenMin.getText(buffer10, sizeof(buffer10));
  sensorData += String(buffer10) + ",";
  nitrogenMax.getText(buffer11, sizeof(buffer11));
  sensorData += String(buffer11);

  if (sensorData.endsWith(","))
  {
    sensorData = sensorData.substring(0, sensorData.length() - 1);
  }
  writeToSDCard("sensor_settings.txt", sensorData);
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
      kompressorMax.setText(sensorValue.c_str());
      sensorIndex = sensorSettings.indexOf(',', sensorIndex) + 1;

      sensorValue = sensorSettings.substring(sensorIndex, sensorSettings.indexOf(',', sensorIndex));
      vakumMin.setText(sensorValue.c_str());
      sensorIndex = sensorSettings.indexOf(',', sensorIndex) + 1;

      sensorValue = sensorSettings.substring(sensorIndex, sensorSettings.indexOf(',', sensorIndex));
      vakumMax.setText(sensorValue.c_str());
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

void cekPassword()
{
  adminPassword = readFromSDCard("admin_password.txt");
  userPassword = readFromSDCard("user_password.txt");

  if (adminPassword.isEmpty() || userPassword.isEmpty())
  {
    if (adminPassword.isEmpty())
    {
      writeToSDCard("admin_password.txt", "12345");
    }
    if (userPassword.isEmpty())
    {
      writeToSDCard("user_password.txt", "54321");
    }
    dbSerial.println("Password default telah dibuat");
  }
  else
  {
    dbSerial.println("Password sudah ada");
  }
}

void writeSNslave()
{
  String data;
  char buffer[6][10] = {{0}};
  NexText *tslaves[6] = {&tslave1, &tslave2, &tslave3, &tslave4, &tslave5, &tslave6};

  for (int i = 0; i < 6; i++)
  {
    tslaves[i]->getText(buffer[i], sizeof(buffer[i]));
    data += String(buffer[i]);
    if (i < 5)
      data += ",";
  }

  data = filterNonPrintable(data);

  writeToSDCard("sn_slaves.txt", data);
}

void loadSNslave()
{
  int index = 0;
  int nextComma;
  String value;

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
  wifimanager.setConfigPortalTimeout(120);
  wifimanager.setCaptivePortalEnable(true);
  bool res;
  res = wifimanager.autoConnect("Master Alarm", "");
}

void checkButton()
{
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

  int admin = adminPassword.toInt();
  int user = userPassword.toInt();

  char passwordValue[6];

  password.getText(passwordValue, sizeof(passwordValue));

  int passwordInt = atoi(passwordValue);

  if (passwordInt == admin)
  {
    password.setText("");
    settingsn.show();
    loadSNslave();
    loadDisplaySettings();
    indikator(3);
  }
  else if (passwordInt == user)
  {
    password.setText("");
    settingPage.show();
    loadDisplaySettings();
    indikator(2);
  }
  else
  {
    password.setText("");
    indikator(1);
  }
}

void bsavePopCallback(void *ptr)
{
  uint32_t passwordadmin;
  uint32_t passworduser;

  setAdmin.getValue(&passwordadmin);
  String adminPasswordStr = String(passwordadmin);
  writeToSDCard("admin_password.txt", adminPasswordStr);

  setUser.getValue(&passworduser);
  String userPasswordStr = String(passworduser);
  writeToSDCard("user_password.txt", userPasswordStr);
  indikatorBuzzer(4);
  esp_restart();
}

void bPageloginPopCallback(void *ptr)
{
  indikatorBuzzer(4);
  t1.disable();
  t2.disable();
  loginpage.show();
}

void bSubmitPopCallback(void *ptr)
{
  indikatorBuzzer(4);
  writePasswordToSD();
  writeSettingsToSD();
}

void bpPasswordPopCallback(void *ptr)
{
  indikatorBuzzer(4);
  verifikasilogin();
}

void badminpasswordPopCallback(void *ptr)
{
  indikatorBuzzer(4);
  uint32_t passwordadmin;

  writeSNslave();
  setpasswordadmin.getValue(&passwordadmin);
  String adminPasswordStr = String(passwordadmin);
  writeToSDCard("admin_password.txt", adminPasswordStr);
}

void balarmsettingPopCallback(void *ptr)
{
  indikatorBuzzer(4);
  loadDisplaySettings();
  alarmpage.show();
}

void bbackPopCallback(void *ptr)
{
  indikatorBuzzer(4);
  writeSensorSettingsToSD();
  settingPage.show();
}

void bPagehomePopCallback(void *ptr)
{
  indikatorBuzzer(4);
  home.show();
  esp_restart();
  clearinputnilai();
}

void bBarPopCallback(void *ptr)
{
  indikatorBuzzer(4);
  lastSelectedButton = 1;
  saveButtonState(lastSelectedButton);
}

void bKPaPopCallback(void *ptr)
{
  indikatorBuzzer(4);
  lastSelectedButton = 2;
  saveButtonState(lastSelectedButton);
}

void bPSiPopCallback(void *ptr)
{
  indikatorBuzzer(4);
  lastSelectedButton = 3;
  saveButtonState(lastSelectedButton);
}

void processReceivedData(const String &receivedData, int currentSlaveIndex)
{
  float supply = 0.00;
  float leftBank = 0.00;
  float rightBank = 0.00;

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
  t3.disable();
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
      // indikatorBuzzer(2);
    }
    t3.enable();
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

bool errorSent[6] = {false, false, false, false, false, false}; // Status untuk melacak apakah error sudah dikirim

void taskNextion()
{
  byte numberValue[6];
  int index = 0;
  for (int i = 0; i < 6; i++)
  {
    int commaIndex = displaySettings.indexOf(',', index);
    if (commaIndex == -1 && i < 5)
    {
      dbSerial.println("Error: Format display_settings.txt tidak valid");
      return;
    }
    numberValue[i] = displaySettings.substring(index, commaIndex).toInt();
    index = commaIndex + 1;
  }

  const char *macPrefixes[] = {"OX", "NO", "CO", "KP", "VK", "NG"};
  int displayIndex = 0;
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
          float supplyValue = slaveData[j].supply;

          // Jika supply value adalah 0 (slave tidak terhubung)
          if (supplyValue == 0.00)
          {
            if (!errorSent[displayIndex])
            {
              // Hanya kirim "Err" sekali jika belum pernah dikirim sebelumnya
              supplyText[displayIndex].setText("Err");
              nilaiText[displayIndex][0].setText("Err");
              nilaiText[displayIndex][1].setText("Err");
              tKet[displayIndex].Set_background_image_pic(8); // Low
              tStatus[displayIndex].setText("Err");
              dbSerial.println("Error: Slave " + String(j) + " tidak terhubung");
              errorSent[displayIndex] = true; // Tandai error sudah dikirim
            }
          }
          else
          {
            // Reset status error jika suplai kembali normal
            if (errorSent[displayIndex])
            {
              errorSent[displayIndex] = false;
            }

            // Mengolah data untuk supply, left bank, dan right bank
            char supply[10], left[10], right[10];
            float conversionFactor = (selectedUnit == "3") ? 14.5038 : (selectedUnit == "2") ? 100
                                                                                             : 1;
            float precision = (selectedUnit == "1") ? 2 : 0;

            snprintf(supply, sizeof(supply), "%.*f", (int)precision, supplyValue * conversionFactor);
            supplyText[displayIndex].setText(supply);

            if (strcmp(macPrefix, "KP") != 0 && strcmp(macPrefix, "VK") != 0)
            {
              snprintf(left, sizeof(left), "%.*f", (int)precision, slaveData[j].leftBank * conversionFactor);
              snprintf(right, sizeof(right), "%.*f", (int)precision, slaveData[j].rightBank * conversionFactor);
              nilaiText[displayIndex][0].setText(left);
              nilaiText[displayIndex][1].setText(right);
            }

            // Ambil nilai min dan max dari sensorSettings
            float minVal = 0.0, maxVal = 0.0;
            int sensorIndex = gasType * 4;
            minVal = sensorSettings.substring(sensorIndex, sensorSettings.indexOf(',', sensorIndex)).toFloat() * conversionFactor;
            sensorIndex = sensorSettings.indexOf(',', sensorIndex) + 1;
            maxVal = sensorSettings.substring(sensorIndex, sensorSettings.indexOf(',', sensorIndex)).toFloat() * conversionFactor;

            // Set background image dan status
            if (supplyValue < minVal)
            {
              tKet[displayIndex].Set_background_image_pic(8); // Low
              tStatus[displayIndex].setText("Low");
            }
            else if (supplyValue > maxVal)
            {
              tKet[displayIndex].Set_background_image_pic(7); // High
              tStatus[displayIndex].setText("High");
            }
            else
            {
              tKet[displayIndex].Set_background_image_pic(9); // Normal
              tStatus[displayIndex].setText("Normal");
            }
          }

          found = true;
          break;
        }
      }

      displayIndex++;
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
  bback.attachPop(bPagehomePopCallback, &bback);
  balarmsetting.attachPop(balarmsettingPopCallback, &balarmsetting);
  bsimpan.attachPop(badminpasswordPopCallback, &bsimpan);
  bsave.attachPop(bsavePopCallback, &bsave);
  bsubmit1.attachPop(bbackPopCallback, &bsubmit1);
  bBar.attachPop(bBarPopCallback, &bBar);
  bKPa.attachPop(bKPaPopCallback, &bKPa);
  bPSi.attachPop(bPSiPopCallback, &bPSi);
}

// ... kode sebelumnya ...

void checkForFirmwareUpdate()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("Memeriksa pembaruan firmware...");

    updateInfo ui = OTADRIVE.updateFirmware(true);
    if (ui.available)
    {
      Serial.println("Pembaruan firmware tersedia. Memulai proses pembaruan...");
      // Proses pembaruan akan dimulai otomatis oleh OTADRIVE
    }
    else
    {
      Serial.println("Tidak ada pembaruan firmware yang tersedia.");
    }
  }
  else
  {
    Serial.println("Koneksi WiFi tidak tersedia, tidak bisa memeriksa pembaruan.");
  }
}

// ... kode selanjutnya ...

void setup()
{
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(TRIGGER_PIN, INPUT);
  digitalWrite(BUZZER_PIN, LOW);
  OTADRIVE.setInfo("ea7d2d48-f17a-4ac1-82ae-f7b2999ef231", "1.1.5");

  // indikatorBuzzer(3);

  nexInit();
  clearUnusedDisplays();
  setupNextiontombol();

  setupSDCard();
  readAllSettingsFromSD();
  cekPassword();

  runner.init();
  runner.addTask(t1);
  runner.addTask(t2);
  runner.addTask(t3);
  t1.enable();
  t2.enable();
  t3.enable();

  SPI.begin();

  activateLoRa();
  setupLoRa();

  setupwifimanager();

  client.setServer(mqttServer, mqttPort);

  for (int i = 0; i < numSlaves; i++)
  {
    slaveData[i].mac = slaveMacs[i];
  }

  byte numberValue[6];
  int currentIndex = 0;
  readAndProcessValues(numberValue, currentIndex);
  checkForFirmwareUpdate();
}

void loop()
{
  checkButton();
  nexLoop(nex_listen_list);
  runner.execute();

  wifimanager.process();
}