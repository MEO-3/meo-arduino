#pragma once

// ThingCube pin map — ESP32-C3-DevKitC-02.

// Shared I2C bus: MPU6050 + SSD1306 0.96" OLED. Core defaults for the C3.
// GPIO8 doubles as the onboard RGB LED — don't drive it while I2C is up.
#define SDA_PIN 8
#define SCL_PIN 9

#define MPU6050_ADDR 0x68 // AD0 low
#define OLED_ADDR    0x3C // some modules use 0x3D (jumper on the back)

#define DHT11_PIN 3
