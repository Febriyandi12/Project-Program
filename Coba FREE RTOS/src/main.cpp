#include <Arduino.h>

int counter1 = 0;
int counter2 = 0;
int counter3 = 0;
uint8_t led = 2;

TaskHandle_t TaskHandle_1;
void taks1(void *pvParameter)
{
  for (;;)
  {
    Serial.print("Hitungan 1 :");
    Serial.println(counter1++);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}
void taks2(void *pvParameter)
{
  for (;;)
  {
    if (counter1 == 10){
      vTaskSuspend(TaskHandle_1);
    }
    Serial.print("Hitungan 2 :");
    Serial.println(counter2++);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}
void taks3(void *pvParameter)
{
  for (;;)
  {
    Serial.print("Hitungan 3 :");
    Serial.println(counter3++);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}
void taks4(void *pvParameter)
{
  pinMode (led, OUTPUT);
  for (;;)
  {
    digitalWrite(led, HIGH);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    digitalWrite(led, LOW);
    vTaskDelay(3000 / portTICK_PERIOD_MS);
  }
}

void setup()
{
  Serial.begin(9600);
  xTaskCreate(taks1, "Task1", 2048, NULL, 1, NULL);
  xTaskCreate(taks2, "Task2", 2048, NULL, 1, NULL);
  // xTaskCreate(taks3, "Task3", 2048, NULL, 1, NULL);
  xTaskCreate(taks4, "Task4", 2048, NULL, 1, NULL);
}

void loop()
{
  // put your main code here, to run repeatedly:
}
