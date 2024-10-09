#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

// Informasi Wi-Fi
const char *ssid = "Rakyat_Jelata"; // Ganti dengan nama WiFi Anda
const char *password = "passwordsalah"; // Ganti dengan password WiFi Anda

// Bot Telegram
String botToken = "7484279421:AAH3IJ43BQw4CzLkdLclXLLbIX12WHIlUQc"; // Ganti dengan token bot Anda
WiFiClientSecure client;                                            // Koneksi aman ke Telegram
UniversalTelegramBot bot(botToken, client);

// Status dan simulasi
float supply = random(320, 900) / 100.0; // Supply nilai random dari 3.20 sampai 9.00 bar
String supplyStatus = (supply < 4.00) ? "low" : (supply >= 9.00) ? "high" : "normal"; // Seleksi status supply
String leftStatus = "Standby";
String rightStatus = "Used";

unsigned long lastTimeBotRan;
const int botRequestDelay = 100; // Delay 100 ms antara setiap polling

// Variabel untuk interval pengiriman
unsigned long lastIntervalSend = 0;
int sendInterval = 0;
bool shouldSendInterval = false;
String selectedStatus = "";
// Fungsi untuk mengirim status supply, left status, dan right status
void sendStatus(String chat_id)
{
  String message = "";
  if (selectedStatus == "supply")
  {
    message += "Supply: " + String(supply, 2) + " bar\n";
    message += "Supply Status: " + supplyStatus + "\n";
    message += "Left Status: " + leftStatus + "\n";
    message += "Right Status: " + rightStatus + "\n";
  }
  if (selectedStatus == "left")
  {
    message += "Left Status: " + leftStatus + "\n";
  }
  if (selectedStatus == "right")
  {
    message += "Right Status: " + rightStatus + "\n";
  }
  bot.sendMessage(chat_id, message, "");
}

// Fungsi untuk menangani perintah dari Telegram
void handleCommand(String text, String chat_id)
{
  if (text == "/setting@NotifikasiFres_bot" || text == "/setting")
  {
    bot.sendMessage(chat_id, "Selamat datang di bot notifikasi Fres! \n\n"
                             "Berikut adalah perintah yang dapat Anda gunakan:\n\n"
                             "/setting - Menu Perintah\n"
                             "/ceksupply - Cek status supply\n"
                             "/cekleftstatus - Cek status kiri\n"
                             "/cekrightstatus - Cek status kanan\n\n"
                             "Silakan pilih perintah yang Anda inginkan.",
                    "");
  }
  if (text == "/ceksupply@NotifikasiFres_bot" || text == "/ceksupply")
  {
    selectedStatus = "supply";
    sendStatus(chat_id);
  }
  if (text == "/cekleftstatus@NotifikasiFres_bot" || text == "/cekleftstatus")
  {
    selectedStatus = "left";
    sendStatus(chat_id);
  }
  if (text == "/cekrightstatus@NotifikasiFres_bot" || text == "/cekrightstatus")
  {
    selectedStatus = "right";
    sendStatus(chat_id);
  }
}

void setup()
{
  Serial.begin(115200);

  // Koneksi ke Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Konfigurasi koneksi aman untuk Telegram
  client.setInsecure(); // Hapus validasi sertifikat untuk koneksi aman
}

void loop()
{
  // Cek apakah ada perintah dari bot setiap 100 ms
  if (millis() - lastTimeBotRan > botRequestDelay)
  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while (numNewMessages)
    {
      Serial.println("Pesan baru diterima");
      for (int i = 0; i < numNewMessages; i++)
      {
        String chat_id = String(bot.messages[i].chat_id);
        String text = bot.messages[i].text;

        Serial.println("Pesan dari chat ID: " + chat_id);
        Serial.println("Pesan: " + text);

        handleCommand(text, chat_id); // Tangani perintah dari pesan
      }
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeBotRan = millis();
  }
if (Serial.available())
  {
    String serialInput = Serial.readStringUntil('\n');
    serialInput.trim(); // Hapus karakter kosong

    // Kirim pesan ke bot
    bot.sendMessage("-4595686796", serialInput, "");
  }
}
