#include <SPI.h>
#include <LoRa.h>

int counter = 0;

const int csPin = 5;
const int resetPin = 14;
const int irqPin = 2;

void setup()
{
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.println("LoRa Sender");
  LoRa.setPins(csPin, resetPin, irqPin);
  if (!LoRa.begin(433E6))
  {
    Serial.println("Starting LoRa failed!");
    while (1)
      ;
  }
}

void loop()
{
  Serial.print("Sending packet: ");
  Serial.println(counter);

  // send packet
  LoRa.beginPacket();
  LoRa.write("12345");
  LoRa.print(counter);
  LoRa.endPacket();

  counter++;

  delay(1000);
}