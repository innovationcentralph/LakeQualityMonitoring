#include "MQTTMonitor.h"

// Include external dependencies
#include "MQTTHandler.h"
#include "NetworkManager.h"
#include "WiFi.h"
#include "EEPROM.h"
#include "SystemConfig.h"

#include <esp_task_wdt.h>


// Define task handle
TaskHandle_t xTaskHandle_MQTTMonitor = NULL;
SemaphoreHandle_t xSemaphore_LocalTime = NULL;



// Create an instance of the MQTT handler
extern MQTTHandler mqttHandler;  // Assuming the instance is declared in the main sketch

String localTime = "";
unsigned long epochTime = 0;

// Function prototypes
String getTime12HourFormat(unsigned long epoch, int gmtOffset);
void writeSendIntervalToEEPROM(uint32_t interval);


// Function to start the MQTT monitor task
void startMQTTMonitorTask() {

  xSemaphore_LocalTime = xSemaphoreCreateMutex();
  if (xSemaphore_LocalTime == NULL) {
    Serial.println("Failed to create mutex for localTime");
    return;
  }

  xTaskCreatePinnedToCore(
    MQTTMonitor_Routine,       // Task function
    "MQTT Monitor",            // Task name
    4096,                      // Stack size
    NULL,                      // Task parameters
    1,                         // Task priority
    &xTaskHandle_MQTTMonitor,  // Task handle
    0                          // Core ID
  );

  esp_task_wdt_add(xTaskHandle_MQTTMonitor);
}

// MQTT monitoring routine
void MQTTMonitor_Routine(void* pvParameters) {
  Serial.print("MQTT monitoring running on core ");
  Serial.println(xPortGetCoreID());

  esp_task_wdt_add(NULL); 


  for (;;) {
    if (networkInfo.wifiConnected == true) {

      // Check MQTT connectivity
      if (mqttHandler.checkConnectivity()) {
        networkInfo.mqttConnected = true;
        //Serial.println("MQTT Connected");
      } else {
        networkInfo.mqttConnected = false;
        Serial.println("MQTT Disconnected");
      }

      // Check for incoming messages
      if (mqttHandler.messageAvailable()) {
        String incomingTopic = mqttHandler.getMessageTopic();
        String incomingPayload = mqttHandler.getMessagePayload();

        // Process the message

        Serial.print("Received message on topic: ");
        Serial.println(incomingTopic);
        Serial.print("Message payload: ");
        Serial.println(incomingPayload);

        // Process change in sending interval
        if (strcmp(incomingTopic.c_str(), topicConfig) == 0) {
          // Parse the payload for a new interval
          uint32_t newInterval = incomingPayload.toInt();
          if (newInterval > 10000 && newInterval <= 3600000) {  // Ensure valid interval > 10s
            sendInterval = newInterval;
            writeSendIntervalToEEPROM(newInterval);  // Save to EEPROM
            Serial.println("Updated send interval: " + String(sendInterval) + " ms");
          }
        }




        // Clear the message flag
        mqttHandler.clearMessageFlag();
      }
    }
    esp_task_wdt_reset();
    // Yield to other tasks
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}



// Write `sendInterval` to EEPROM
void writeSendIntervalToEEPROM(uint32_t interval) {
  EEPROM.put(EEPROM_SEND_INTERVAL_ADDR, interval);
  EEPROM.commit();
}
