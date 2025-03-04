#include "ModbusSlave.h"

ModbusSlave* ModbusSlave::instance = nullptr;
bool ModbusSlave::serialNumberUpdatePending = false;  // Initialize the static flag

ModbusSlave::ModbusSlave()
  : mb() {}

void ModbusSlave::begin() {
  instance = this;

  // Initialize Preferences
  preferences.begin("modbus", false);

  // Initialize Serial2 for Modbus communication
  Serial2.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);
  mb.begin(&Serial2, TXRX_CONTROL_PIN);

  pinMode(MODBUS_LED, OUTPUT);
  digitalWrite(MODBUS_LED, LOW);

  // Initialize or load registers
  initializeRegisters();
}

void ModbusSlave::handleModbus() {
  mb.task();              // Run Modbus communication
}
uint16_t ModbusSlave::onSetHreg(TRegister* reg, uint16_t value) {
    uint16_t address = reg->address.address;

    digitalWrite(MODBUS_LED, HIGH);  // Indicate Modbus activity

    if (!instance) return value;  // Safety check for the instance pointer

    Serial.print("Register type: HREG, Address: ");
    Serial.print(address);
    Serial.print(", Updated value: ");
    Serial.println(value);

    // Handle Slave ID update
    if (address == SLAVE_ID_REG) {
        uint16_t savedSlaveID = instance->preferences.getUInt("slave_id", DEFAULT_SLAVE_ID);
        if (savedSlaveID != value) {
            Serial.println("Slave ID updated!");
            instance->preferences.putUInt("slave_id", value);
            instance->mb.slave(value);
        } else {
            Serial.println("Slave ID unchanged. No save needed.");
        }
    }

    // Handle MSG ID update
    else if (address == MSG_ID_REG) {
      
    }

    // Handle other registers
    else {
       
    }

    digitalWrite(MODBUS_LED, LOW);  // Indicate end of Modbus activity

    return value;
}

void ModbusSlave::initializeRegisters() {
    bool isFresh = !preferences.isKey("initialized");

    if (isFresh) {
        Serial.println("Fresh KVS detected. Initializing registers to defaults...");

        // Initialize registers as per the updated table
        RegisterConfig registerConfigs[] = {
            {1, 10001, "Sensor ID"},
            {2, 0, "MSG ID"},  // Default MSG ID
            {3, 0, "Water Level"},
            {4, 0, "Water Temperature"},
            {5, 0, "Dissolved Oxygen"},
            {6, 0, "Total Dissolved Solids"},
            {7, 0, "PH Level "},
            {8, 0, "Ambient Temperature"},
            {9, 0, "Ambient Humidity"},
            {10, 0, "General Purpose"},
        };


        // Initialize the registers
        for (const auto& reg : registerConfigs) {
            mb.addHreg(reg.address, reg.defaultValue);
            mb.onSetHreg(reg.address, onSetHreg);
            preferences.putUInt(String(reg.address).c_str(), reg.defaultValue);
            Serial.printf("Register %d (%s) initialized to default value: %d\n", reg.address, reg.description, reg.defaultValue);
        }

        // Initialize and save MSG ID to NVS
        preferences.putUInt("msg_id", 0);

        // Initialize Slave ID
        preferences.putUInt("slave_id", DEFAULT_SLAVE_ID);
        mb.addHreg(SLAVE_ID_REG, DEFAULT_SLAVE_ID);
        mb.onSetHreg(SLAVE_ID_REG, onSetHreg);
        mb.slave(DEFAULT_SLAVE_ID);
        Serial.printf("Slave ID initialized to default value: %d\n", DEFAULT_SLAVE_ID);

        preferences.putBool("initialized", true);
    } else {
        Serial.println("KVS data found. Loading register values...");

        // Load MSG ID from NVS
        uint16_t savedMsgID = preferences.getUInt("msg_id", 0);
        mb.addHreg(MSG_ID_REG, savedMsgID);
        mb.onSetHreg(MSG_ID_REG, onSetHreg);
        Serial.printf("MSG ID loaded with value: %d\n", savedMsgID);

        // Load other registers as per the updated table
        struct RegisterConfig {
            uint16_t address;
            const char* description;
        };

        RegisterConfig registerConfigs[] = {
            {1, "Sensor ID"},
            {3, "Water Level"},
            {4, "Water Temperature"},
            {5, "Dissolved Oxygen"},
            {6, "Total Dissolved Solids"},
            {7, "PH Level"},
            {8, "Ambient Temperature"},
            {9, "Ambient Humidity"},
            {10, "General Purpose"},
        };

        for (const auto& reg : registerConfigs) {
            uint16_t savedValue = preferences.getUInt(String(reg.address).c_str(), 0);
            mb.addHreg(reg.address, savedValue);
            mb.onSetHreg(reg.address, onSetHreg);
            Serial.printf("Register %d (%s) loaded with value: %d\n", reg.address, reg.description, savedValue);
        }

        // Load Slave ID
        uint16_t savedSlaveID = preferences.getUInt("slave_id", DEFAULT_SLAVE_ID);
        mb.addHreg(SLAVE_ID_REG, savedSlaveID);
        mb.onSetHreg(SLAVE_ID_REG, onSetHreg);
        mb.slave(savedSlaveID);
        Serial.printf("Slave ID loaded with value: %d\n", savedSlaveID);
    }
}
