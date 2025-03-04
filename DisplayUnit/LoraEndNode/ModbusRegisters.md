# Modbus Register Documentation

This file documents the Modbus registers used in the project. Each register has an address, type, default value, and a brief description.

## Register Table

| Address | Type  | Default Value | Description                      | Range    | Unit         | Data Type |
|---------|-------|---------------|----------------------------------|----------|--------------|-----------|
| 1       | HREG  | 10,001        | Sensor ID                        | 0-65535  | -            | uint16_t     |
| 2       | HREG  | 0             | MSG ID                           | 0-65535  | -            | uint16_t     |
| 3       | HREG  | 0             | Water Level                      | 0-100    | mH20         | uint16_t     |
| 4       | HREG  | 0             | Water Temperature                | 0-100    | C            | uint16_t     |
| 5       | HREG  | 0             | Dissolved Oxygen                 | 0-1000   | mg/L         | uint16_t     |
| 6       | HREG  | 0             | Total Dissolved Solids           | 0-100    | ppm          | uint16_t     |
| 7       | HREG  | 0             | PH Level                         | 0-100    |              | uint16_t     |
| 8       | HREG  | 0             | Ambient Temperature              | 0-100    | C            | uint16_t     |
| 9       | HREG  | 0             | Ambient Humidity                 | 0-100    | %RH          | uint16_t     |
| 10      | HREG  | 0             | General Purpose                  | 0-100    | -            | uint16_t     |

| 100     | HREG  | 16            | Slave ID                         | 1-247    | -            | Float        |





