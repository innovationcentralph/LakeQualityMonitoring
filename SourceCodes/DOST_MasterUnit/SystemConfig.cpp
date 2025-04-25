#include "SystemConfig.h"

// Wi-Fi credentials
const char* wifiSSID = "PLDTinnov";
const char* wifiPass = "Password12345!";

// MQTT configuration variables
const char* mqttServer     = "3.27.210.100";
const int mqttPort         = 1883;
const char* mqttUser       = "mqtt";
const char* mqttPassword   = "ICPHmqtt!";
const char* deviceESN      = "WQM-001";

// MQTT broker
const char* topicInit       = "WQM/WQM-001/init";
const char* topicConfig     = "WQM/WQM-001/config";
const char* topicSensorData = "WQM/WQM-001";
