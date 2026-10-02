// Control the built-in LED from the gateway.
//
// Declares the "led" cap and drives LED_BUILTIN with it. The gateway sends the
// write over MQTT and waits for this device's reply:
//
//   curl -X POST http://<gateway>:7070/api/v1/devices/<deviceId>/command \
//        -H 'Content-Type: application/json' -d '{"cap":"led","op":"write","value":1}'
//
// value 0 turns the LED off, non-zero on. "op":"read" returns the last value.
// deviceId is the device's Wi-Fi MAC, lowercase hex without separators.
//
// On first boot the device is unprovisioned and advertises over BLE; provision
// it from the gateway before sending commands. To skip provisioning while
// developing, uncomment the beginWifi()/setBroker() calls below.

#include <Arduino.h>
#include <Meo3.h>

#define LED_BUILTIN 8

MeoDevice meo("MEO LED Demo");

// "led" write handler. Returning false replies MEO_ERR_HANDLE_FAILED to the
// gateway; here the write always succeeds.
static bool handleLed(int16_t value) {
    digitalWrite(LED_BUILTIN, value ? LOW : HIGH);
    Serial.printf("[LED] %s\n", value ? "on" : "off");
    return true;
}

void setup()
{
    Serial.begin(115200);
    delay(500); // let USB CDC enumerate before the first print

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);


    // Declare the cap before begin(): the gateway reads the cap list off the
    // BLE capability characteristic while provisioning.
    meo.addCap("led", handleLed);

    // Development shortcut — bypasses BLE provisioning:
    // meo.beginWifi("your-ssid", "your-password");
    // meo.setBroker("192.168.1.10", 1883);

    if (!meo.begin())
    {
        Serial.println("[ERROR] begin() failed - halting");
        while (true)
        {
            delay(1000);
        }
    }

    Serial.println(meo.isProvisioned()
                       ? "[INFO] Provisioned - waiting for commands"
                       : "[INFO] Not provisioned - BLE advertising");
}

void loop()
{
    meo.loop();
}
