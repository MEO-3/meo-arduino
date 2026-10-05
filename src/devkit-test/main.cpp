#include <Arduino.h>
#include <DHT.h>
#include <Meo3.h>
#include <WiFi.h>

// ESP32-C3-DevKitC-02 built-in RGB is GPIO8; adjust if your board differs
#define LED_PIN 8
#define DHT11_PIN 3

// How often to sample temperature + humidity (ms); the DHT11 refreshes every ~2s
#define READING_INTERVAL_MS 2000

MeoDevice meo("MEO Test Device");
static DHT dht(DHT11_PIN, DHT11);

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

// "temp" read handler, °C x100. The DHT11 drops reads routinely and the reply
// frame has no "no value" encoding, so hold the last good sample.
static int16_t readTemperature() {
    static int16_t last = 0;
    float v = dht.readTemperature();
    if (!isnan(v)) {
        last = (int16_t)lroundf(v * 100);
    }
    return last;
}

// "humid" read handler, %RH x100; same last-good hold as temp
static int16_t readHumidity() {
    static int16_t last = 0;
    float v = dht.readHumidity();
    if (!isnan(v)) {
        last = (int16_t)lroundf(v * 100);
    }
    return last;
}

void setup() {
    Serial.begin(115200);
    delay(500); // let USB CDC enumerate before first print

    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    dht.begin();

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
