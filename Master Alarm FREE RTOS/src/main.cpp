#include <Arduino.h>
#include <LoRa.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiManager.h>

uint8_t csPin = 5;
uint8_t resetPin = 14;
uint8_t irqPin = 33;

TickType_t xLastWakeTime;
const TickType_t xLoRaInterval = pdMS_TO_TICKS(1000); // interval 1 detik

void taks1(void *pvParameter)
{
  LoRa.begin(433E6);
  LoRa.setPins(csPin, resetPin, irqPin);
  xLastWakeTime = xTaskGetTickCount();
  while (1)
  {

    int slaveId = (xTaskGetTickCount() / xLoRaInterval) % 8;
    String macAddress = "0x00" + String(slaveId, HEX);

    // kode lainnya...

    vTaskDelayUntil(&xLastWakeTime, xLoRaInterval);
  }
}

void setup()
{
  // put your setup code here, to run once:
}

void loop()
{
  // put your main code here, to run repeatedly:
}
