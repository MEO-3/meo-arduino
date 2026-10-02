# MEO 3 Arduino Library

MEO 3 Arduino is an ESP32 firmware library for MEO 3 devices. It handles BLE provisioning against the MEO Edge gateway so a device can join Wi-Fi and report what it can do, without the sketch touching MQTT or BLE directly.

## What it does

- Advertises BLE provisioning and walks the MEO provisioning GATT contract
- Connects to Wi-Fi once the gateway writes credentials (or via `beginWifi()` for local development)
- Uses the board MAC as the stable device identity — no device IDs or MQTT credentials to configure
- Reports device model, firmware version, and declared capabilities to the gateway during provisioning

## Install

- Open the library in PlatformIO (or Arduino IDE)
- `#include <Meo3.h>`
- Give your device a name and declare its capabilities

## Quick start

```cpp
#include <Meo3.h>

MeoDevice meo("Classroom Weather Station");

int16_t readTemp() { return 2345; }            // 23.45 °C, values are x100
bool setMotor(int16_t speed) { /* ... */ return true; }

void setup() {
  // Declare caps before begin() — the gateway reads them off the BLE
  // capability characteristic during provisioning.
  meo.addCap("temp", nullptr, readTemp);
  meo.addCap("motor", setMotor);

  meo.begin();
}

void loop() {
  meo.loop();
}
```

## API

- `MeoDevice()` / `MeoDevice(model)`
- `setDeviceInfo(model, manufacturer)`, `setFirmwareVersion(version)`
- `addCap(key, onWrite, onRead)` — declare a cap by key; call before `begin()`
- `sendEvent(key, value)` — push a cap's value (reading or event) to the gateway
- `beginWifi(ssid, pass)` — bypass BLE provisioning for local development
- `begin()` — init storage/BLE/provisioning, connect if already provisioned
- `loop()` — drive provisioning, detect Wi-Fi connect, stop BLE once online
- `isProvisioned()`, `isWifiConnected()`, `isMqttConnected()`

See `docs/key_concepts.md` for handler rules and the wire format.

## Provisioning

The device advertises the MEO provisioning service when Wi-Fi is missing. The full GATT contract (characteristics, payload formats, status states) is shared with the gateway and documented once, in `meo-edge/docs/firmware_development_guide.md` — that file is the source of truth, not this README.

## Capabilities

A device defines its own caps by key (`[a-z0-9_]{1,32}`, max 16) and reports them during provisioning; there is no shared catalog. Every cap can be read, written and sent as an event, with `int16` values (decimals x100). Changing the cap list requires re-provisioning.

## Examples

- `examples/01_blink_command`
- `examples/02_button_event`
- `examples/03_temperature_reading`

## Docs

- `docs/key_concepts.md` — beginner mental model and full API reference
- `meo-edge/docs/firmware_development_guide.md` — authoritative BLE provisioning contract
