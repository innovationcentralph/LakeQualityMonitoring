// Header Includes
#include <SPI.h>
#include <LoRa.h>
#include <ModbusRTU.h>
#include "NetworkManager.h"
#include "SystemConfig.h"
#include "MQTTMonitor.h"
#include "EEPROM.h"

#include <esp_task_wdt.h>

uint32_t sendInterval = 10000;  // initial sending interval default
uint32_t lastMQTTConnectionTime = 0;
MQTTHandler mqttHandler;

uint32_t heartbeatMillis = 0;


// Global Variables
volatile bool txDoneFlag = false;
String receivedMessage = "";

// Modbus Object
ModbusRTU modbus;

// Function Prototypes
void initializeLoRa();
void LoRa_rxMode();
void LoRa_txMode();
void LoRa_sendMessage(const String &message);
void handleTxDone();
void handleReceive(int packetSize);
boolean runEvery(unsigned long interval);
uint16_t calculateCRC16(const uint8_t *data, size_t length);
void initializeModbus();
void modbusTask(void *pvParameters);
void processReceivedMessage();
void mockTest();

void setup() {
  Serial.begin(9600);  // Initialize Serial

  pinMode(ACTIVE_INDICATOR, OUTPUT);
  digitalWrite(ACTIVE_INDICATOR, LOW);

  // Define the Task Watchdog Timer configuration
  esp_task_wdt_config_t wdt_config = {
    .timeout_ms = 30000,   // 30 seconds timeout
    .idle_core_mask = 0,   // Monitor no idle cores by default
    .trigger_panic = true  // Trigger a panic on timeout
  };

  // Initialize the Task Watchdog Timer
  esp_err_t err = esp_task_wdt_init(&wdt_config);
  if (err == ESP_OK) {
    Serial.println("Watchdog timer initialized successfully.");
  } else {
    Serial.println("Failed to initialize watchdog timer.");
  }

  esp_task_wdt_add(NULL);  // Add the current task to the watchdog


  pinMode(LED_INDICATOR, OUTPUT);
  digitalWrite(LED_INDICATOR, LED_OFF);

  initializeLoRa();
  Serial.println("LoRa Gateway initialized.");

  EEPROM.begin(EEPROM_SIZE);

  // Read `sendInterval` from EEPROM
  sendInterval = readSendIntervalFromEEPROM();

  if (sendInterval < 10000 || sendInterval >= 3600000) {  // Minimum Sending time is 10s
    sendInterval = 10000;
    EEPROM.put(EEPROM_SEND_INTERVAL_ADDR, sendInterval);
    EEPROM.commit();
  }

  Serial.println("Sending Interval: " + String(sendInterval));

  initializeModbus();

  //mockTest();

  // Create a task to handle Modbus communication
  xTaskCreatePinnedToCore(
    modbusTask,    // Function to run
    "ModbusTask",  // Task name
    4096,          // Stack size (in bytes)
    NULL,          // Task parameter
    1,             // Priority
    NULL,          // Task handle
    1              // Core ID (1 for ESP32 dual-core)
  );

  // Start the Network Monitor Task
  startNetworkMonitorTask();

  // Initialize MQTT
  mqttHandler.init(mqttServer, mqttPort, mqttUser, mqttPassword, deviceESN, willTopic, willMessage);

  // Set the initial message to publish on connect
  String initMessage = "Jolan Gateway " + String(deviceESN) + " is now connected";
  mqttHandler.setInitialMessage("Jolan/init", initMessage.c_str());

  // // Add subscription topics
  mqttHandler.addSubscriptionTopic(topicConfig);

  // Start the MQTT Monitor Task
  startMQTTMonitorTask();
}

void loop() {


  // Check Server Connectivity -> Restart when disconnected for too long
  if (networkInfo.mqttConnected) {

    lastMQTTConnectionTime = millis();
    digitalWrite(LED_INDICATOR, LED_ON);
  } else {
    digitalWrite(LED_INDICATOR, LED_OFF);
    // Check if the ESP has been disconnected for too long
    if (millis() - lastMQTTConnectionTime > MQTT_DISCONNECT_TIMEOUT_MS) {
      Serial.println("MQTT disconnected for too long. Restarting ESP...");
      vTaskDelay(100);
      ESP.restart();  // Restart the ESP
    }
  }


  // Handle Tx Done Flag
  if (txDoneFlag) {
    txDoneFlag = false;
    LoRa_rxMode();
    Serial.println("Tx Done and switched to Rx mode.");
  }

  // Process received messages
  if (!receivedMessage.isEmpty()) {
    processReceivedMessage();
  }

  if (millis() - heartbeatMillis > sendInterval) {

    String topic = String("WQM/Heartbeat/") + deviceESN;
    String payload = "Heartbeat";

    Serial.println("Trying to Send Heartbeat");
    if (networkInfo.mqttConnected) {
      mqttHandler.publish(topic.c_str(), payload.c_str());
#ifdef DEBUG_MQTT
      Serial.println("Sent heartbeat message.");
#endif
    }

    heartbeatMillis = millis();
  }

  //Send a message every 5000 ms
  // if (runEvery(5000)) {
  //   String message = "HeLoRa World! I'm a Gateway! " + String(millis());
  //   LoRa_sendMessage(message);
  //   Serial.println("Message sent!");
  // }


  if (runEvery(3000)) {
    digitalWrite(ACTIVE_INDICATOR, HIGH);
    vTaskDelay(200);
    digitalWrite(ACTIVE_INDICATOR, LOW);
  }


  esp_task_wdt_reset();

  // //delay(1000);
  // //processReceivedMessage();
}

void mockTest() {
  // Construct a mock payload
  uint8_t mockPayload[] = {
    0x27, 0x11, 0x00, 0x02, 0x44, 0x44, 0x55, 0x55, 0x66, 0x66, 0x3A, 0x76
  };

  // Convert the payload to a String for the `receivedMessage`
  receivedMessage = "";
  for (size_t i = 0; i < sizeof(mockPayload); i++) {
    receivedMessage += (char)mockPayload[i];
  }

  // Print the mock payload in hex format for debugging
  Serial.print("Mock Payload: ");
  for (size_t i = 0; i < sizeof(mockPayload); i++) {
    Serial.printf("0x%02X ", mockPayload[i]);
  }
  Serial.println();

  // Call the process function
  processReceivedMessage();
}


void processReceivedMessage() {
  Serial.print("Received Message: ");
  Serial.println(receivedMessage);

  // Get the RSSI value of the received packet
  int packetRssi = LoRa.packetRssi();
  float packetSnr = LoRa.packetSnr();

  size_t receivedLength = receivedMessage.length();
  if (receivedLength < 6) {  // 2 bytes (Sensor ID) + 2 bytes (MSG ID) + 2 bytes (CRC) = minimum 6 bytes
    Serial.println("Error: Message too short for processing.");
    return;
  }

  uint8_t receivedBytes[receivedLength];
  for (size_t i = 0; i < receivedLength; i++) {
    receivedBytes[i] = (uint8_t)receivedMessage[i];
  }

  size_t payloadLength = receivedLength - 2;  // Exclude the last 2 bytes (CRC)
  uint16_t receivedCRC = (receivedBytes[payloadLength] << 8) | receivedBytes[payloadLength + 1];
  uint16_t calculatedCRC = calculateCRC16(receivedBytes, payloadLength);

  Serial.print("Message in Hex: ");
  for (size_t i = 0; i < receivedLength; i++) {
    Serial.printf("0x%02X ", receivedBytes[i]);
  }
  Serial.println();

  if (calculatedCRC == receivedCRC) {
    Serial.println("CRC Check Passed!");

    // Extract Sensor ID (first 2 bytes)
    uint16_t sensorID = (receivedBytes[0] << 8) | receivedBytes[1];
    Serial.print("Sensor ID: ");
    Serial.println(sensorID);

    // Extract Message ID (next 2 bytes)
    uint16_t messageID = (receivedBytes[2] << 8) | receivedBytes[3];
    Serial.print("Message ID: ");
    Serial.println(messageID);

    // Extract Data (bytes in between Sensor ID, Message ID, and CRC)
    size_t dataStart = 4;                           // Start of data (after Sensor ID and MSG ID)
    size_t dataLength = payloadLength - dataStart;  // Length of the data

    if (dataLength % 2 != 0) {
      Serial.println("Error: Data length is not a multiple of 2 (invalid uint16 format).");
      return;
    }

    Serial.print("Data Length (in bytes): ");
    Serial.println(dataLength);

    // Parse sensor data
    String dataString = "";
    if (dataLength > 0) {
      size_t dataCount = dataLength / 2;
      uint16_t dataArray[dataCount];
      for (size_t i = 0; i < dataCount; i++) {
        dataArray[i] = (receivedBytes[dataStart + i * 2] << 8) | receivedBytes[dataStart + i * 2 + 1];
      }

      // Prepare JSON payload
      dataString += "{\"MID\":\"" + String(messageID) + "\"";
      if (dataCount > 0) dataString += ", \"WL\":\"" + String(dataArray[0] / 100.0, 3) + "\"";
      if (dataCount > 1) dataString += ", \"WT\":\"" + String(dataArray[1] / 100.0, 3) + "\"";
      if (dataCount > 2) dataString += ", \"DO\":\"" + String(dataArray[2] / 100.0, 3) + "\"";
      if (dataCount > 3) dataString += ", \"TDS\":\"" + String(dataArray[3] / 100.0, 3) + "\"";
      if (dataCount > 4) dataString += ", \"PH\":\"" + String(dataArray[4] / 100.0, 3) + "\"";
      if (dataCount > 5) dataString += ", \"T\":\"" + String(dataArray[5] / 100.0, 3) + "\"";
      if (dataCount > 6) dataString += ", \"H\":\"" + String(dataArray[6] / 100.0, 3) + "\"";
      dataString += ", \"RSSI\":\"" + String(packetRssi) + "\"";
      dataString += ", \"SNR\":\"" + String(packetSnr) + "\"";
      dataString += "}";

      // Print parsed data
      Serial.println("Parsed Payload: ");
      Serial.println(dataString);
    } else {
      Serial.println("No data present.");
    }

    // Generate MQTT Topic
    String topic = topicSensorData;

    // Publish to MQTT
    if (mqttHandler.checkConnectivity()) {
      mqttHandler.publish(topic.c_str(), dataString.c_str());
      Serial.println("MQTT Message Sent:");
      Serial.print("Topic: ");
      Serial.println(topic);
      Serial.print("Payload: ");
      Serial.println(dataString);
    } else {
      Serial.println("MQTT not connected. Failed to send message.");
    }

  } else {
    Serial.printf("CRC Check Failed! Calculated CRC: 0x%04X, Received CRC: 0x%04X\n", calculatedCRC, receivedCRC);
  }

  receivedMessage = "";
}



// Read `sendInterval` from EEPROM
uint32_t readSendIntervalFromEEPROM() {
  uint32_t interval;
  EEPROM.get(EEPROM_SEND_INTERVAL_ADDR, interval);
  if (interval == 0 || interval > 3600000) {  // Set a default if the value is invalid
    interval = 60000;                         // Default to 60 seconds
    EEPROM.put(EEPROM_SEND_INTERVAL_ADDR, interval);
    EEPROM.commit();
  }
  return interval;
}
