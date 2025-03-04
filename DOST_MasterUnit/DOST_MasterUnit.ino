#include "SensorManager.h"
#include "EEPROM.h"
#include "SystemConfig.h"
#include "EasyNextionLibrary.h"


bool configTimeFlag = false;
uint32_t lastMQTTConnectionTime = 0;
uint32_t lcdUpdateMillis = 0;
uint32_t sendingIntervalMillis = 0;


// Nextion HMI Display instance
EasyNex display(Serial);

void setup() {

  Serial.begin(115200);
  delay(1000);

  // Initialize Display
  // display.begin(115200);

  pinMode(TIMER_DONE, OUTPUT);
  digitalWrite(TIMER_DONE, LOW);

  // // Start the Sensor Manager Task
  startSensorManagerTask();

  delay(1000);
}

void loop() {

  if (sensorDataReady) {
    if (xSemaphoreTake(xSemaphore_Sensor, portMAX_DELAY) == pdTRUE) {

      uint16_t modbus_resp[16];
      bool res = modbus.readHreg(LORA_MODBUS_ADDR, 0x0002, modbus_resp, 1, nullptr);
      vTaskDelay(2000);  // Example delay

      if (res == true) {
        uint16_t msg_id = modbus_resp[0];
        Serial.println("MSG ID: " + String(msg_id));
        msg_id = msg_id + 1;
        Serial.println("Writing MSG ID: " + String(msg_id));
        modbus.writeHreg(LORA_MODBUS_ADDR, 0x02, msg_id);
        vTaskDelay(5000);
      }

      Serial.println("Shutting down now");
      vTaskDelay(100);  // some time to print
      digitalWrite(TIMER_DONE, HIGH);

      xSemaphoreGive(xSemaphore_Sensor);
    }
  }

 

  vTaskDelay(100);
}


