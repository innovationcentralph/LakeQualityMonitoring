#include "ModbusSlave.h"
#include <SPI.h>  // Include libraries
#include <LoRa.h>
#include "Globals.h"

ModbusSlave modbusSlave;


// Global Variables

volatile bool sendLoRaMessageFlag = false;

volatile bool txDoneFlag = false;  // Flag for Tx Done
String receivedMessage = "";       // Buffer for received messages

// Function Prototypes
void initializeLoRa();
void LoRa_rxMode();
void LoRa_txMode();
void LoRa_sendMessage(const String &message);
void handleTxDone();
void handleReceive(int packetSize);
uint16_t calculateCRC16(const uint8_t *data, size_t length);


void modbusTask(void *pvParameters) {
  while (true) {
    modbusSlave.handleModbus();           // Run Modbus task
    vTaskDelay(10 / portTICK_PERIOD_MS);  // Delay to prevent task hogging CPU
  }
}

void setup() {
  Serial.begin(9600);
  Serial.println("Modular Modbus Slave with FreeRTOS Task starting...");

  pinMode(LORA_LED, OUTPUT);
  digitalWrite(LORA_LED, LOW);

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


  initializeLoRa();
  Serial.println("LoRa End Node initialized.");
}

void loop() {
  // Handle Tx Done Flag
  if (txDoneFlag) {
    txDoneFlag = false;  // Clear the flag
    LoRa_rxMode();       // Switch to receive mode
    Serial.println("Tx Done and switched to Rx mode.");
  }

  // Process received messages
  if (receivedMessage.length() > 0) {
    Serial.print("Received Message: ");
    Serial.println(receivedMessage);
    receivedMessage = "";  // Clear the buffer
  }

  if (sendLoRaMessageFlag == true) {

    digitalWrite(LORA_LED, HIGH);

    uint8_t buffer[256];     // Temporary buffer to hold the payload
    size_t bufferIndex = 0;  // Track the current position in the buffer

    // Retrieve Sensor ID and MSG ID from Modbus registers
    uint16_t sensorID = ModbusSlave::instance->mb.Hreg(0x01);  // Register 1: Sensor ID
    uint16_t msgID = ModbusSlave::instance->mb.Hreg(0x02);     // Register 2: MSG ID

    // Add Sensor ID to the buffer
    buffer[bufferIndex++] = (sensorID >> 8) & 0xFF;  // High byte
    buffer[bufferIndex++] = sensorID & 0xFF;         // Low byte

    // Add MSG ID to the buffer
    buffer[bufferIndex++] = (msgID >> 8) & 0xFF;     // High byte
    buffer[bufferIndex++] = msgID & 0xFF;            // Low byte

    // Add sensor data from registers 3 to 9 (your specified range)
    for (uint16_t reg = 3; reg <= 9; reg++) { 
        uint16_t sensorData = ModbusSlave::instance->mb.Hreg(reg);
        buffer[bufferIndex++] = (sensorData >> 8) & 0xFF;  // High byte
        buffer[bufferIndex++] = sensorData & 0xFF;         // Low byte
    }

    // Calculate CRC-16 for the payload
    uint16_t crc = calculateCRC16(buffer, bufferIndex);

    // Append CRC-16 to the payload
    buffer[bufferIndex++] = (crc >> 8) & 0xFF;  // High byte of CRC
    buffer[bufferIndex++] = crc & 0xFF;         // Low byte of CRC

    // Print the payload for debugging
    Serial.print("Payload: ");
    for (size_t i = 0; i < bufferIndex; i++) {
        Serial.printf("%02X ", buffer[i]);
    }
    Serial.println();

    // Send the payload over LoRa
    LoRa_txMode();       // Switch to transmit mode
    LoRa.beginPacket();  // Start LoRa packet

    for (size_t i = 0; i < bufferIndex; i++) {
        LoRa.write(buffer[i]);  // Write each byte from the buffer
    }

    LoRa.endPacket(true);  // Send the packet

    Serial.println("LoRa Message Sent with Sensor ID, MSG ID, Sensor Data, and CRC-16");
    sendLoRaMessageFlag = false;  // Reset the flag
    
    digitalWrite(LORA_LED, LOW);
}
}

// Initialize LoRa Configuration
void initializeLoRa() {
  LoRa.setPins(CS_PIN, RESET_PIN, IRQ_PIN);  // Set LoRa pins

  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa initialization failed. Check connections.");
    while (true)
      ;
  }

  LoRa.onReceive(handleReceive);  // Attach receive callback
  LoRa.onTxDone(handleTxDone);    // Attach Tx Done callback

  LoRa_rxMode();  // Start in receive mode
  Serial.println("LoRa initialized successfully.");
}

// Set LoRa to Receive Mode
void LoRa_rxMode() {
  LoRa.enableInvertIQ();  // Enable IQ inversion
  LoRa.receive();         // Set LoRa to receive mode
}

// Set LoRa to Transmit Mode
void LoRa_txMode() {
  LoRa.idle();             // Set LoRa to standby mode
  LoRa.disableInvertIQ();  // Disable IQ inversion
}

// Send a Message via LoRa
void LoRa_sendMessage(const String &message) {
  LoRa_txMode();         // Switch to transmit mode
  LoRa.beginPacket();    // Start LoRa packet
  LoRa.print(message);   // Add message payload
  LoRa.endPacket(true);  // Send packet and wait for completion
}

// Handle Tx Done Event
void handleTxDone() {
  txDoneFlag = true;  // Set flag to handle in loop
}

// Handle Receive Event
void handleReceive(int packetSize) {
  String tempMessage = "";

  while (LoRa.available()) {
    tempMessage += (char)LoRa.read();
  }

  receivedMessage = tempMessage;  // Pass message to loop for processing
}

uint16_t calculateCRC16(const uint8_t *data, size_t length) {
  uint16_t crc = 0xFFFF;  // Initial value

  for (size_t i = 0; i < length; i++) {
    crc ^= data[i];  // XOR byte into least significant byte of crc

    for (uint8_t j = 0; j < 8; j++) {
      if (crc & 1) {
        crc = (crc >> 1) ^ 0xA001;  // Apply Modbus polynomial
      } else {
        crc >>= 1;
      }
    }
  }

  return crc;
}
