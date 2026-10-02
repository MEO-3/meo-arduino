#include <Arduino.h>
#include <WiFi.h>
#include <Meo3.h>

// ESP32-C3-DevKitC-02 built-in RGB is GPIO8; adjust if your board differs
#define LED_PIN 8

// How often to publish the periodic temperature reading (ms)
#define READING_INTERVAL_MS 10000

MeoDevice meo("MEO Test Device");

// Blink LED n times at the given on/off period (ms)
static void blinkLed(int times, int periodMs) {
    for (int i = 0; i < times; i++) {
        digitalWrite(LED_PIN, HIGH);
        delay(periodMs / 2);
        digitalWrite(LED_PIN, LOW);
        delay(periodMs / 2);
    }
}

// "led" write handler: 0 = off, anything else = on
static bool handleLed(int16_t value) {
    digitalWrite(LED_PIN, value ? HIGH : LOW);
    return true;
}

// "temp" read handler, °C x100: no real sensor on the devkit — fake a slow drift
static int16_t readTemperature() {
    return 2000 + (millis() % 10000) / 10;
}

void setup() {
    Serial.begin(115200);
    delay(500);  // let USB CDC enumerate before first print

    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    Serial.println("\n=== MEO Provisioning + Messaging Test ===");


    // Caps declared before begin() — the edge reads them off the BLE
    // capability characteristic during provisioning. idx = declaration order.
    meo.addCap("led", handleLed);
    meo.addCap("temp", nullptr, readTemperature);

    bool ok = meo.begin();
    if (!ok) {
        Serial.println("[ERROR] begin() failed — halting");
        while (true) {
            blinkLed(3, 200);
            delay(1000);
        }
    }

    if (meo.isProvisioned()) {
        Serial.printf("[INFO] Already provisioned. IP: %s\n", WiFi.localIP().toString().c_str());
        blinkLed(2, 300);
    } else {
        Serial.println("[INFO] Not provisioned — BLE advertising. Waiting for edge...");
    }
}

void loop() {
    meo.loop();

    // Slow blink while waiting for provisioning
    if (!meo.isProvisioned()) {
        blinkLed(1, 1000);
        return;
    }

    // First time we reach provisioned state; from here the LED belongs to
    // "led" writes
    static bool announced = false;
    if (!announced) {
        announced = true;
        Serial.printf("[INFO] Provisioned! IP: %s\n", WiFi.localIP().toString().c_str());
        blinkLed(5, 150);
    }

    // Periodic reading once messaging is online
    static uint32_t lastReadingAt = 0;
    if (meo.isMqttConnected() && millis() - lastReadingAt >= READING_INTERVAL_MS) {
        lastReadingAt = millis();
        meo.sendEvent("temp", readTemperature());
    }
}
