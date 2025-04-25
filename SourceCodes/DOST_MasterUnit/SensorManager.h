#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Arduino.h>
#include <ModbusRTU.h>

// Struct to hold sensor data
typedef struct {
    float waterLevel; 
    float waterTemperature;    
    float dissolvedOxygen;
    float tds;
    float phLevel;
    float temperature;
    float humidity;
} SensorData_t;

// RTOS Task Handle
extern TaskHandle_t xTaskHandle_SensorManager;
extern SemaphoreHandle_t xSemaphore_Sensor;
extern bool sensorDataReady;


// Global Sensor Data
extern SensorData_t sensorData;
extern ModbusRTU modbus;  

// Function Prototypes
void startSensorManagerTask();  // Function to start the task
void SensorManagerTask(void* pvParameters); // Task function

#endif // SENSOR_MANAGER_H
