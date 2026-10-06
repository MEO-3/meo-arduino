#include <Arduino.h>
#include <Meo3.h>
#include <WiFi.h>

// ESP32-C3-DevKitC-02 built-in RGB is GPIO8; adjust if your board differs
#define LED_PIN 8

// How often to generate a temperature + humidity sample (ms)
#define READING_INTERVAL_MS 2000

MeoDevice meo("MEO Test Device 2");

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
    digitalWrite(LED_PIN, value ? LOW : HIGH);
    return true;
}

// "temp" read handler, °C x100. No sensor on this build — a random value in
// 20.00..35.00 °C stands in so messaging can be tested on a bare devkit.
static int16_t readTemperature() {
    return (int16_t)random(2000, 3501);
}

// "humid" read handler, %RH x100; random 40.00..80.00 %RH, same reason as temp
static int16_t readHumidity() {
    return (int16_t)random(4000, 8001);
}

void setup() {
    Serial.begin(115200);
    delay(500); // let USB CDC enumerate before first print

    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    Serial.println("\n=== MEO Provisioning + Messaging Test ===");

    // Caps declared before begin() — the edge reads them off the BLE
    // capability characteristic during provisioning. idx = declaration order.
    meo.addCap("led", MEO_CAP_SWITCH, handleLed);
    meo.addCap("temp", MEO_CAP_TEMPERATURE, nullptr, readTemperature);
    meo.addCap("humid", MEO_CAP_HUMIDITY, nullptr, readHumidity);

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

    // Sample once messaging is online and push only values that moved. A push
    // that fails leaves the marker alone, so it retries next tick.
    static uint32_t lastReadingAt = 0;
    static int16_t pushedT = INT16_MIN, pushedH = INT16_MIN;
    if (meo.isMqttConnected() && millis() - lastReadingAt >= READING_INTERVAL_MS) {
        lastReadingAt = millis();
        int16_t t = readTemperature();
        int16_t h = readHumidity();
        if (t != pushedT && meo.sendEvent("temp", t)) {
            pushedT = t;
        }
        if (h != pushedH && meo.sendEvent("humid", h)) {
            pushedH = h;
        }
    }
}
