// Initialize LoRa Configuration
void initializeLoRa() {
    LoRa.setPins(CS_PIN, RESET_PIN, IRQ_PIN);

    if (!LoRa.begin(LORA_FREQUENCY)) {
        Serial.println("LoRa initialization failed. Check connections.");
        while (true);
    }

    LoRa.onReceive(handleReceive);
    LoRa.onTxDone(handleTxDone);

    LoRa_rxMode();
    Serial.println("LoRa initialized successfully.");
}

// Set LoRa to Receive Mode
void LoRa_rxMode() {
    LoRa.disableInvertIQ();
    LoRa.receive();
}

// Set LoRa to Transmit Mode
void LoRa_txMode() {
    LoRa.idle();
    LoRa.enableInvertIQ();
}

// Send a Message via LoRa
void LoRa_sendMessage(const String &message) {
    LoRa_txMode();
    LoRa.beginPacket();
    LoRa.print(message);
    LoRa.endPacket(true);
}

// Handle Tx Done Event
void handleTxDone() {
    txDoneFlag = true;
}

// Handle Receive Event
void handleReceive(int packetSize) {
    if (packetSize == 0) return;

    receivedMessage = "";
    for (int i = 0; i < packetSize; i++) {
        receivedMessage += (char)LoRa.read();
    }
}



// Run a Task at Fixed Intervals
boolean runEvery(unsigned long interval) {
    static unsigned long previousMillis = 0;
    unsigned long currentMillis = millis();

    if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;
        return true;
    }

    return false;
}

// Function to calculate CRC-16
uint16_t calculateCRC16(const uint8_t *data, size_t length) {
    uint16_t crc = 0xFFFF;

    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];

        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }

    return crc;
}
