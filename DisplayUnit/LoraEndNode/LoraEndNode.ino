#include "ModbusSlave.h"
#include "EasyNextionLibrary.h"


ModbusSlave modbusSlave;
EasyNex display(Serial1);


// Global Variables

uint32_t lcdUpdateMillis = 0;

void modbusTask(void *pvParameters) {
  while (true) {
    modbusSlave.handleModbus();           // Run Modbus task
    vTaskDelay(10 / portTICK_PERIOD_MS);  // Delay to prevent task hogging CPU
  }
}

void setup() {
  Serial.begin(9600);
  Serial.println("Modular Modbus Slave with FreeRTOS Task starting...");

  // Initialize Display
  display.begin(115200);

  // Initialize Modbus Slave
  modbusSlave.begin();

  // Create a FreeRTOS task for Modbus
  xTaskCreate(
    modbusTask,    // Task function
    "ModbusTask",  // Name of the task (for debugging)
    4096,          // Stack size (in words)
    NULL,          // Task parameters
    1,             // Priority (1 = low, higher = higher priority)
    NULL           // Task handle
  );


}

void loop() {

  // Update display
  if (millis() - lcdUpdateMillis > 1000) {

    //Update sensor Values
    //display.writeStr("t0.txt", String((uint32_t)(ModbusSlave::instance->mb.Hreg(6) / 100.0)));  // TDS
    display.writeStr("t3.txt", String(ModbusSlave::instance->mb.Hreg(7) / 100.0));  // PH
    display.writeStr("t6.txt", String(ModbusSlave::instance->mb.Hreg(4) / 100.0, 1));  // DO
    display.writeStr("t4.txt", String((uint32_t)(ModbusSlave::instance->mb.Hreg(5) / 100.0)));  // Water Temperature
    display.writeStr("t2.txt", String(ModbusSlave::instance->mb.Hreg(3) / 100.0, 1));  // Water Level
    display.writeStr("t0.txt", String((uint32_t)(ModbusSlave::instance->mb.Hreg(5) / 100.0)));  // TDS
    display.writeStr("t8.txt", String((uint32_t)(ModbusSlave::instance->mb.Hreg(9) / 100.0)));  // Humidity
    display.writeStr("t7.txt", String((uint32_t)(ModbusSlave::instance->mb.Hreg(8) / 100.0)));  // Temperature

  
    // display.writeNum("p3.pic", GPRS_OK);
    // display.writeNum("p2.pic", WIFI_OK);
    // display.writeNum("p1.pic", MQTT_OK);
    
    //Serial.println("MQTT Connection: " + String(networkInfo.mqttConnected));
    lcdUpdateMillis = millis();
  }

  
}


