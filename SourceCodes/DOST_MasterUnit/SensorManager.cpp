#include "SensorManager.h"
#include "DFRobot_SHT20.h"

#include "SystemConfig.h"

ModbusRTU modbus;

// Define the task handle
TaskHandle_t xTaskHandle_SensorManager = NULL;
SemaphoreHandle_t xSemaphore_Sensor = NULL;
bool sensorDataReady = false;

// Define the global sensor data
SensorData_t sensorData = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };

DFRobot_SHT20 sht20(&Wire, SHT20_I2C_ADDR);

// Timer interval for reading sensors (in milliseconds)
const uint32_t SENSOR_READ_INTERVAL_MS = 2000;

void modbusTask(void* pvParameters);
void SensorManagerTask(void* pvParameters);

void startSensorManagerTask() {

  xSemaphore_Sensor = xSemaphoreCreateMutex();
  if (xSemaphore_Sensor == NULL) {
    Serial.println("Failed to create mutex for sensor");
    return;
  }

  xTaskCreatePinnedToCore(
    SensorManagerTask,           // Task function
    "Sensor Manager",            // Task name
    4096,                        // Stack size
    NULL,                        // Task parameters
    1,                           // Task priority
    &xTaskHandle_SensorManager,  // Task handle
    1                            // Core ID
  );

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
}

// Modbus task function
void modbusTask(void* pvParameters) {
  for (;;) {

    // Handle Modbus tasks
    modbus.task();

    // Small delay to prevent CPU hogging
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void SensorManagerTask(void* pvParameters) {
#ifdef DEBUG_SENSORS
  Serial.print("Sensor Manager running on core ");
#endif
  Serial.println(xPortGetCoreID());

// Initialize sensor peripherals here
#ifdef DEBUG_SENSORS
  Serial.println("Initializing sensors...");
#endif

  // Initialize Modbus Port as Master
  Serial2.begin(9600, SERIAL_8N1, MASTER_RO, MASTER_DI);
  modbus.begin(&Serial2, MASTER_EN);
  modbus.setBaudrate(9600);
  modbus.master();


  // Init SHT20 Sensor
  sht20.initSHT20();


#ifdef DEBUG_SENSORS
  Serial.println("Modbus Master Initialized");
#endif

  uint32_t lastReadTime = -1 * SENSOR_READ_INTERVAL_MS;  // to read ASAP

  // Main task loop
  for (;;) {
    if (millis() - lastReadTime >= SENSOR_READ_INTERVAL_MS) {
      // Time to read sensors
      Serial.println("Reading sensors...");

      if (xSemaphoreTake(xSemaphore_Sensor, portMAX_DELAY) == pdTRUE) {

        uint16_t modbus_resp[16];
        bool res;

        // Read Water Level Sensor
        res = modbus.readHreg(50, 0x0002, modbus_resp, 3, nullptr);
        vTaskDelay(2000);  // Example delay

        if (res == true) {
          sensorData.waterLevel = modbus_resp[2] / 1000.0;
#ifdef DEBUG_SENSORS
          Serial.println("Water Level: " + String(sensorData.waterLevel) + " mH20");
#endif
        } else {
          Serial.println("Error reading Water Level sensor");
        }



        // Read DO Sensor with Water Temperature
        res = modbus.readHreg(25, 0x0000, modbus_resp, 4, nullptr);

        vTaskDelay(2000);  // Example delay

        res = modbus.readHreg(25, 0x0000, modbus_resp, 4, nullptr);

        vTaskDelay(2000);  // Example delay

        if (res == true) {
          //Serial.printf("RESPONSE: %02f %02f %02f %02f", modbus_resp[0], modbus_resp[1], modbus_resp[2], modbus_resp[3]);
          sensorData.dissolvedOxygen = modbus_resp[0] / 100.0;
          sensorData.waterTemperature = modbus_resp[2] / 100.0;

#ifdef DEBUG_SENSORS
          Serial.println("                  DO: " + String(sensorData.dissolvedOxygen) + " mg/L");
          Serial.println("   Water Temperature: " + String(sensorData.waterTemperature) + " C");
#endif
        } else {
          Serial.println("Error reading DO sensor");
        }

        // read EC/TDS sensor


        // Read PH Level
        res = modbus.readHreg(1, 0x0000, modbus_resp, 1, nullptr);

        vTaskDelay(1000);  // Example delay

        if (res == true) {
          sensorData.phLevel = modbus_resp[0] / 100.0;


#ifdef DEBUG_SENSORS
          Serial.println("   PH Level: " + String(sensorData.phLevel));
#endif
        } else {
          Serial.println("Error reading PH sensor");
        }

        // Read Turbidity Sensor
        res = modbus.readHreg(0x11, 0x0000, modbus_resp, 1, nullptr);
        vTaskDelay(2000);
        if (res == true) {
          sensorData.tds = modbus_resp[0] / 1000.0;
          Serial.println("Turbidity: " + String(sensorData.tds) + " NTU");
        }

        // read temperature and humidity
        sensorData.temperature = sht20.readTemperature();
        sensorData.humidity = sht20.readHumidity();

#ifdef DEBUG_SENSORS
        Serial.print("Temperature: ");
        Serial.println(sensorData.temperature);
        Serial.print("Humidity: ");
        Serial.println(sensorData.humidity);

#endif

        // Add LCD Update


        // Update Modbus Slave

        Serial.println("Updating LCD");
        uint16_t writeData[7];
        writeData[0] = (uint16_t)(sensorData.waterLevel * 100);
        writeData[1] = (uint16_t)(sensorData.dissolvedOxygen * 100);
        writeData[2] = (uint16_t)(sensorData.waterTemperature * 100);
        writeData[3] = (uint16_t)(sensorData.tds * 100);
        writeData[4] = (uint16_t)(sensorData.phLevel * 100);
        writeData[5] = (uint16_t)(sensorData.temperature * 100);
        writeData[6] = (uint16_t)(sensorData.humidity * 100);
        modbus.writeHreg(LCD_MODBUS_ADDR, 0x03, writeData, 7);
        vTaskDelay(2000);


        writeData[0] = (uint16_t)(sensorData.waterLevel * 100);
        writeData[1] = (uint16_t)(sensorData.dissolvedOxygen * 100);
        writeData[2] = (uint16_t)(sensorData.waterTemperature * 100);
        writeData[3] = (uint16_t)(sensorData.tds * 100);
        writeData[4] = (uint16_t)(sensorData.phLevel * 100);
        writeData[5] = (uint16_t)(sensorData.temperature * 100);
        writeData[6] = (uint16_t)(sensorData.humidity * 100);
        modbus.writeHreg(LORA_MODBUS_ADDR, 0x03, writeData, 7);
        vTaskDelay(2000);

        sensorDataReady = true;

        xSemaphoreGive(xSemaphore_Sensor);
      }

      // Reset timer
      lastReadTime = millis();
    }

    // Task delay to yield execution
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}
