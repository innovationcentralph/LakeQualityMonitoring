#ifndef MODBUS_SLAVE_H
#define MODBUS_SLAVE_H

#include <ModbusRTU.h>
#include <Preferences.h>
#include "PinConfig.h" // Include pin configuration

class ModbusSlave {
public:
  ModbusSlave();
  void begin();
  void handleModbus();

  ModbusRTU mb;            // Modbus object
  static ModbusSlave* instance; // Static instance pointer for callback access

private:
  // Constants for registers
  static const uint16_t MSG_ID_REG = 2;             // Register for MSG ID
  static const uint16_t SLAVE_ID_REG = 100;         // Register for Slave ID
  static const uint16_t DEFAULT_SLAVE_ID = 32;      // Default Modbus Slave ID

  Preferences preferences; // Preferences object for KVS

  static bool serialNumberUpdatePending; // Static flag for serial number updates

  // Struct for register configuration
  struct RegisterConfig {
    uint16_t address;
    uint16_t defaultValue;
    const char* description;
  };

  // Private methods
  static uint16_t onSetHreg(TRegister* reg, uint16_t value); // Callback function
  void initializeRegisters(); // Initialize registers and load from KVS
  void processSerialNumber(); // Process and reconstruct serial number
};

#endif // MODBUS_SLAVE_H
