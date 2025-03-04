
// Initialize Modbus
void initializeModbus() {
    Serial2.begin(9600, SERIAL_8N1, MASTER_RO, MASTER_DI);
    modbus.begin(&Serial2, MASTER_EN);
    modbus.setBaudrate(9600);
    modbus.master();
}

// Modbus Task Function
void modbusTask(void* pvParameters) {
    for (;;) {
        modbus.task();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}