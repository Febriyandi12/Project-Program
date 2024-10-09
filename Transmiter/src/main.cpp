#include <SPI.h>
#include <SD.h>
#include <Nextion.h>
#include <EEPROM.h>
#include <TaskScheduler.h>
#include <LoRa.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>

const int TRIGGER_PIN = 4;
const int csPin = 33;    // LoRa radio chip select  *input only eror
const int resetPin = 15; // LoRa radio reset
const int irqPin = 13;   // LoRa radio interrupt
const int buzzerPin = 5; // LED pin

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
const long timeout = 270;

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
Task t1(0, TASK_FOREVER, &taskLoRa);
Task t2(2000, TASK_FOREVER, &taskNextion);
Task t3(1000, TASK_FOREVER, &taskMQTT);
// Task t4(1000, TASK_FOREVER, &t4Callback);

int currentSlaveToSend = 0;

String receivedData = "";
byte eepromAddressUnit = 29;
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
NexButton bsetAlarm = NexButton(3, 12, "bsetAlarm");
// NexButton bsetting = NexButton(3, 8, "b0");
NexDSButton bBar = NexDSButton(1, 19, "bBar");
NexDSButton bKPa = NexDSButton(1, 20, "bKPa");
NexDSButton bPSi = NexDSButton(1, 21, "bPSi");

NexPage home = NexPage(0, 0, "mainmenu3");
NexPage settingPage = NexPage(1, 0, "menusetting");
NexPage loginpage = NexPage(2, 0, "loginpage");
NexPage alarmpage = NexPage(3, 0, "settingalarm");
NexPage settingsn = NexPage(4, 0, "settingsn");
NexPage configPage = NexPage(5, 0, "configpage");

// input untuk halaman login
NexText password = NexText(2, 2, "tpassword");
NexText alert = NexText(2, 1, "t1");

NexNumber passworduser = NexNumber(5, 2, "nUserpass");
NexNumber passwordadmin = NexNumber(5, 1, "nAdminpass");
NexButton bsave = NexButton(5, 5, "bsave");
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
    NexText(0, 18, "bg5")}; // Sudah diubah

NexText gasText[6] = {
    NexText(0, 6, "tGas0"),
    NexText(0, 7, "tGas1"),
    NexText(0, 8, "tGas2"),
    NexText(0, 11, "tGas3"),
    NexText(0, 12, "tGas4"),
    NexText(0, 19, "tGas5")}; // Sudah diubah

NexText satuanText[6][3] = {
    {NexText(0, 51, "tSatuan0"), NexText(0, 45, "tsLeft0"), NexText(0, 24, "tsRight0")},
    {NexText(0, 52, "tSatuan1"), NexText(0, 25, "tsLeft1"), NexText(0, 27, "tsRight1")},
    {NexText(0, 53, "tSatuan2"), NexText(0, 29, "tsLeft2"), NexText(0, 31, "tsRight2")},
    {NexText(0, 54, "tSatuan3"), NexText(0, 33, "tsLeft3"), NexText(0, 35, "tsRight3")},
    {NexText(0, 55, "tSatuan4"), NexText(0, 37, "tsLeft4"), NexText(0, 39, "tsRight4")},
    {NexText(0, 56, "tSatuan5"), NexText(0, 41, "tsLeft5"), NexText(0, 43, "tsRight5")}}; // Sudah diubah

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
                               &bsetAlarm, &blogin, &btutup, &bBar,
                               &bKPa, &bPSi, &bback, &tserialNumber, &bsave, NULL};

// Fungsi untuk konversi data ke format JSON
String convertDataToJson(const String &mac, float supply, float leftBank, float rightBank)
{
  JsonDocument jsonDoc;
  jsonDoc["mac"] = mac;
  jsonDoc["supply"] = String(supply, 2);
  jsonDoc["leftBank"] = String(leftBank, 2);
  jsonDoc["rightBank"] = String(rightBank, 2);

  String jsonData;
  serializeJson(jsonDoc, jsonData);
  return jsonData;
}
void setupLoRa()
{

  LoRa.setPins(csPin, resetPin, irqPin);
  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);
  LoRa.setTxPower(20);
  if (!LoRa.begin(433E6))
  {
    dbSerial.println("Gagal menginisialisasi LoRa");
  }
  else
  {
    dbSerial.println("Berhasil menginisialisasi LoRa");
  }
}
void readDeviceData()
{
  const int startAddress = 50;
  const int addressIncrement = 8;

  for (int i = 0; i < 6; i++)
  {
    char buffer[8] = {0};
    EEPROM.get(startAddress + i * addressIncrement, buffer);

    NexText tDevice = NexText(4, i + 1, ("tslave" + String(i + 1)).c_str());

    // Hanya set text jika buffer tidak kosong
    if (strlen(buffer) > 0)
    {
      tDevice.setText(buffer);
    }
  }
}
void checkEEPROMForPasswords()
{
  uint32_t passworduser, passwordadmin;
  EEPROM.get(44, passworduser);
  EEPROM.get(38, passwordadmin);

  // Jika password admin atau user kosong, panggil configPage
  if (passworduser == 4294967295 || passwordadmin == 4294967295)
  {
    configPage.show();
  }
  else
  {
    dbSerial.println("Password tidak kosong");
  }
}
void bsavePopCallback(void *ptr)
{
  uint32_t user, admin;
  passworduser.getValue(&user);
  EEPROM.put(44, user);
  passwordadmin.getValue(&admin);
  EEPROM.put(38, admin);

  EEPROM.commit();
  home.show();
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

void saveButtonState(uint8_t buttonNumber)
{
  EEPROM.write(eepromAddressUnit, buttonNumber);
  EEPROM.commit();
}

void writeEEprom()
{
  // Array untuk menyimpan semua nilai byte
  byte values[11];
  char nilaibatas[5]; // Buffer untuk menyimpan nilai dari Nextion
  uint32_t tempValue; // Variabel sementara untuk menyimpan nilai dari Nextion
  uint32_t Xfloat;
  uint32_t passworduser;
  // Ambil nilai dari komponen Nextion
  setpassworduser.getValue(&passworduser);
  EEPROM.put(44, passworduser);

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

  // Tulis semua nilai ke EEPROM
  for (int i = 0; i < 11; i++)
  {
    EEPROM.write(i, values[i]);
  }

  EEPROM.commit(); // Simpan semua perubahan ke EEPROM
}

void writeSNslave()
{
  char buffer0[10] = {0}; // Inisialisasi buffer ke nilai 0
  char buffer1[10] = {0};
  char buffer2[10] = {0};
  char buffer3[10] = {0};
  char buffer4[10] = {0};
  char buffer5[10] = {0};

  tslave1.getText(buffer0, sizeof(buffer0));
  tslave2.getText(buffer1, sizeof(buffer1));
  tslave3.getText(buffer2, sizeof(buffer2));
  tslave4.getText(buffer3, sizeof(buffer3));
  tslave5.getText(buffer4, sizeof(buffer4));
  tslave6.getText(buffer5, sizeof(buffer5));

  // Simpan hanya jika serialNumber tidak kosong
  EEPROM.put(50, buffer0);
  EEPROM.put(58, buffer1);
  EEPROM.put(66, buffer2);
  EEPROM.put(74, buffer3);
  EEPROM.put(82, buffer4);
  EEPROM.put(90, buffer5);

  EEPROM.commit();
}

void readEEprom()
{
  // Array untuk menyimpan semua nilai byte
  byte values[11];
  char serialNumber[7];
  uint32_t passworduser, passwordadmin;
  char serialNumber1[7], serialNumber2[7], serialNumber3[7], serialNumber4[7], serialNumber5[7], serialNumber6[7];
  char buffer[7];

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

  EEPROM.get(50, serialNumber1);
  tslave1.setText(serialNumber1);

  EEPROM.get(58, serialNumber2);
  tslave2.setText(serialNumber2);

  EEPROM.get(66, serialNumber3);
  tslave3.setText(serialNumber3);

  EEPROM.get(74, serialNumber4);
  tslave4.setText(serialNumber4);

  EEPROM.get(82, serialNumber5);
  tslave5.setText(serialNumber5);

  EEPROM.get(90, serialNumber6);
  tslave6.setText(serialNumber6);
}

void checkButton()
{
  // check for button press
  if (digitalRead(TRIGGER_PIN) == HIGH)
  {
    // poor mans debounce/press-hold, code not ideal for production
    delay(50);
    if (digitalRead(TRIGGER_PIN) == HIGH)
    {
      Serial.println("Button Pressed");
      // still holding button for 3000 ms, reset settings, code not ideaa for production
      delay(3000); // reset delay hold
      if (digitalRead(TRIGGER_PIN) == HIGH)
      {
        Serial.println("Button Held");
        Serial.println("Erasing Config, restarting");
        wifimanager.resetSettings();
        ESP.restart();
      }

      // start portal w delay
      Serial.println("Starting config portal");
      wifimanager.setConfigPortalTimeout(120);

      if (!wifimanager.startConfigPortal("OnDemandAP", "password"))
      {
        Serial.println("failed to connect or hit timeout");
        delay(3000);
        // ESP.restart();
      }
      else
      {
        // if you get here you have connected to the WiFi
        Serial.println("connected...yeey :)");
      }
    }
  }
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
  tOxmin.setValue(0);
  tVakum.setValue(0);
  tKompressor.setValue(0);
  tOxmax.setValue(0);
  tNomax.setValue(0);
  tNomin.setValue(0);
  tNomax.setValue(0);
  tComin.setValue(0);
  tComax.setValue(0);
  tNgmin.setValue(0);
  tNgmax.setValue(0);
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
  password.getText(passwordValue, sizeof(passwordValue)); // Pastikan password adalah objek Nextion Text input

  // Konversi teks menjadi integer untuk perbandingan
  int passwordInt = atoi(passwordValue);

  // Verifikasi password untuk admin
  if (passwordInt == sandi1)
  {
    password.setText("");
    settingsn.show();
    // Arahkan ke halaman settings
    readEEprom(); // Baca nilai dari EEPROM
  }
  // Verifikasi password untuk user
  else if (passwordInt == sandi2)
  {
    password.setText("");
    settingPage.show(); // Arahkan ke halaman settingPage
    readEEprom();       // Baca nilai dari EEPROM
  }
  else
  {
    password.setText(""); // Kosongkan input password
  }
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

  writeSNslave();
  // Ambil nilai password admin dari komponen Nextion
  setpasswordadmin.getValue(&passwordadmin);
  EEPROM.put(38, passwordadmin);

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

  const char *selectedSatuan;
  switch (EEPROM.read(eepromAddressUnit))
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
  case 6:
    satuanText[currentIndex][0].setText(selectedSatuan); // Nitrogen
    satuanText[currentIndex][1].setText(selectedSatuan); // Satuan Left Bank
    satuanText[currentIndex][2].setText(selectedSatuan);
    break;

  case 4:
    satuanText[currentIndex][0].setText(selectedSatuan);
    break;

  case 5:                                         // Vakum
    satuanText[currentIndex][0].setText(satuan1); // untuk membuat statis satuan Mmhg
    break;
  }

  // Setel warna latar belakang dan teks
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
  for (int i = 0; i < 6; i++)
  {

    gasText[i].setText("-");
    supplyText[i].setText("");
    satuanText[i][0].setText("");
    satuanText[i][1].setText("");
    satuanText[i][2].setText("");

    gasText[i].Set_font_color_pco(0);
    gasText[i].Set_background_color_bco(0);
    satuanText[i][0].Set_background_color_bco(0);
    satuanText[i][1].Set_background_color_bco(0);
    satuanText[i][2].Set_background_color_bco(0);
    supplyText[i].Set_background_color_bco(0);
  }
}

void processReceivedData(const String &receivedData, int currentSlaveIndex)
{
  String cleanedData = receivedData;
  cleanedData.trim(); // Bersihkan spasi dan karakter tidak diperlukan
  if (cleanedData.startsWith(","))
  {
    cleanedData = cleanedData.substring(1); // Buang koma di awal string
  }

  // Parsing the received data based on comma positions
  int firstComma = cleanedData.indexOf(',');
  int secondComma = cleanedData.indexOf(',', firstComma + 1);
  int thirdComma = cleanedData.indexOf(',', secondComma + 1);

  String mac = cleanedData.substring(0, firstComma);
  mac.trim();

  // Default supply, leftBank, rightBank
  float supply;
  float leftBank;
  float rightBank;

  // Extract the supply value
  if (firstComma != -1 && secondComma != -1)
  {
    supply = cleanedData.substring(firstComma + 1, secondComma).toFloat();
  }

  // Check if MAC is not Kompressor (KP) or Vakum (VK)
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
    // For KP and VK, only extract the supply value
    if (firstComma != -1)
    {
      supply = cleanedData.substring(firstComma + 1).toFloat();
    }
  }

  // Store the parsed data in the slaveData array
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
    char slaveMac[8] = {0};
    EEPROM.get(50 + currentSlaveIndex * 8, slaveMac);
    slaveMac[7] = '\0'; // Add null terminator to avoid garbage characters

    // Hanya kirim permintaan jika slaveMac tidak kosong
    if (strlen(slaveMac) > 0)
    {
      // Send request to slave
      LoRa.beginPacket();
      LoRa.write(masterAddress);
      LoRa.print(", ");
      LoRa.print(slaveMac);
      LoRa.endPacket();
      LoRa.receive();
      awaitingResponse = true;
      requestTime = millis();
    }
    else
    {
      // Jika slaveMac kosong, langsung ke slave berikutnya
      currentSlaveIndex = (currentSlaveIndex + 1) % numSlaves;
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

        dbSerial.println(receivedAddress);
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
    // Timeout, move to next slave
    awaitingResponse = false;
    currentSlaveIndex = (currentSlaveIndex + 1) % numSlaves;
  }
}

void taskNextion()
{
  byte numberValue[6];
  readEEPROMValues(numberValue);

  const char *macPrefixes[] = {"OX", "NO", "CO", "KP", "VK", "NG"};
  int displayIndex = 0;

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
          char supply[10];
          char left[10];
          char right[10];

          snprintf(supply, sizeof(supply), "%.2f", slaveData[j].supply);
          supplyText[displayIndex].setText(supply);

          if (!(macPrefix == "KP" || macPrefix == "VK"))
          {
            snprintf(left, sizeof(left), "%.2f", slaveData[j].leftBank);
            snprintf(right, sizeof(right), "%.2f", slaveData[j].rightBank);
            nilaiText[displayIndex][0].setText(left);
            nilaiText[displayIndex][1].setText(right);
          }

          // Tentukan status berdasarkan nilai sensor
          if (slaveData[j].supply < 3.40)
          {
            tKet[displayIndex].Set_background_image_pic(17); // Low
          }
          else if (slaveData[j].supply > 8.00)
          {
            tKet[displayIndex].Set_background_image_pic(15); // High
          }
          else
          {
            tKet[displayIndex].Set_background_image_pic(16); // Normal
          }

          found = true;
          displayIndex++;
          break;
        }
      }

      // Jika tidak ada data dari slave, kosongkan tampilan
      if (!found)
      {
        supplyText[displayIndex].setText("Err");
        if (!(strcmp(macPrefix, "KP") == 0 || strcmp(macPrefix, "VK") == 0))
        {
          nilaiText[displayIndex][0].setText("Err");
          nilaiText[displayIndex][1].setText("Err");
        }
        displayIndex++;
      }
    }
  }
}

void taskMQTT()
{
  handleMQTTConnection();
  // Periksa apakah ada slave yang valid di indeks saat ini
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
      // Kirim data JSON ke MQTT
      sendToMqtt(mac, jsonData);

      // Beralih ke slave berikutnya pada siklus berikutnya
      currentSlaveToSend = (currentSlaveToSend + 1) % numSlaves;
    }
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
  bsave.attachPop(bsavePopCallback, &bsave);

  bback.attachPop(bbackPopCallback, &bback);
  bBar.attachPop(bBarPopCallback, &bBar);
  bKPa.attachPop(bKPaPopCallback, &bKPa);
  bPSi.attachPop(bPSiPopCallback, &bPSi);
}

void setup()
{
  nexInit();
  clearUnusedDisplays();
  pinMode(buzzerPin, OUTPUT);
  pinMode(TRIGGER_PIN, INPUT);
  pinMode(buzzerPin, LOW);
  runner.init();
  runner.addTask(t1);
  runner.addTask(t2);
  runner.addTask(t3);

  setupwifimanager();
  setuptombol();
  setupLoRa();
  EEPROM.begin(512);

  t1.enable();
  t2.enable();
  t3.enable();

  byte numberValue[6];
  int currentIndex = 0;
  readAndProcessEEPROMValues(numberValue, currentIndex);

  readDeviceData();

  // Connect to MQTT
  client.setServer(mqttServer, mqttPort);
  // Perbarui warna tombol berdasarkan status terakhir
  updateButtonColors(lastSelectedButton);

  for (int i = 0; i < numSlaves; i++)
  {
    slaveData[i].mac = slaveMacs[i];
  }
  checkEEPROMForPasswords();
}

void loop()
{

  nexLoop(nex_listen_list);
  runner.execute();
  checkButton();
  wifimanager.process(); // avoid delays() in loop when non-blocking and other long running code
}