#include <EEPROM.h>

void setup()
{
  Serial.begin(115200);
  EEPROM.begin(8); // Initialize EEPROM with 8 bytes of storage
}

void loop()
{
  char data[] = "12345"; // Data to be stored
  int addr = 24;             // Starting address for storage

  // Write data to EEPROM
  for (int i = 0; i < strlen(data); i++)
  {
    EEPROM.write(addr + i, data[i]);
  }

  // Commit changes to EEPROM
  EEPROM.commit();

  Serial.println("Data stored in EEPROM:");
  Serial.println(data);

  delay(1000);
}

void readData()
{
  int addr = 0;  // Starting address for reading
  char data[30]; // Buffer to store read data

  // Read data from EEPROM
  for (int i = 0; i < 30; i++)
  {
    data[i] = EEPROM.read(addr + i);
  }

  Serial.println("Data read from EEPROM:");
  Serial.println(data);
}
