#include <SPI.h>
#include <LoRa.h>
#include <TaskScheduler.h>

const int csPin = 5;     // LoRa radio chip select
const int resetPin = 14; // LoRa radio reset
const int irqPin = 2;    // LoRa radio interrupt

const long frequency = 433E6; // LoRa frequency
const int masterAddress = 0xFF; // Master address

Scheduler ts;

void checkForPacket();
void sendMacAddress();

// Task definitions
Task tCheckForPacket(100, TASK_FOREVER, &checkForPacket, &ts);
Task tSendMacAddress(100, TASK_ONCE, &sendMacAddress, &ts);

void setup() {
  Serial.begin(115200);
  while (!Serial);

  LoRa.setPins(csPin, resetPin, irqPin);

  if (!LoRa.begin(frequency)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }

  Serial.println("LoRa Slave Initialized");

  // Start the tasks
  tCheckForPacket.enable();
}

void loop() {
  // Task scheduler loop
  ts.execute();
}

void checkForPacket() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    Serial.print("Received address: ");

    // Read the packet content
    while (LoRa.available()) {
      char received = LoRa.read();
      Serial.print(received, HEX);

      if (received == masterAddress) {
        tSendMacAddress.enable();
      }
    }
    Serial.println();
  }
}

void sendMacAddress() {
  Serial.print("Sending MAC address to master: 0x");
  Serial.println(masterAddress, HEX);

  LoRa.beginPacket();
  LoRa.write(masterAddress);
  LoRa.endPacket();
}
