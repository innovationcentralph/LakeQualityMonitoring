#ifndef SYSTEM_CONFIG_H
#define SYSTEM_CONFIG_H

// Debug flags
#define DEBUG_WIFI
#define DEBUG_MQTT

// Pin Definitions
#define MASTER_RO 26
#define MASTER_DI 25
#define MASTER_EN 27
#define CS_PIN 5
#define RESET_PIN 14
#define IRQ_PIN 2

#define LED_INDICATOR 12
#define ACTIVE_INDICATOR 13
#define LED_ON        HIGH
#define LED_OFF       LOW

#define EEPROM_SIZE 512
#define EEPROM_SEND_INTERVAL_ADDR 0

#define MQTT_DISCONNECT_TIMEOUT_MS 60000

// MQTT configuration
extern const char* mqttServer;
extern const int mqttPort;
extern const char* mqttUser;
extern const char* mqttPassword;
extern const char* deviceESN;
extern const char* willTopic;
extern const char* willMessage;

extern const char* topicInit;
extern const char* topicConfig;
extern const char* topicSensorData;


enum GatewayMode {
    WIFI_ONLY,        // WiFi mode only
    WIFI_AND_GSM,     // WiFi + GSM mode
    GSM_ONLY          // GSM mode only
};

// LoRa Configuration
const long LORA_FREQUENCY = 433E6;

const GatewayMode gatewayMode = WIFI_ONLY;

#endif // SYSTEM_CONFIG_H


