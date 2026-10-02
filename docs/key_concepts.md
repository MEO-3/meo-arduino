# Key Concepts

This library follows the MEO Edge contract: it gets an ESP32 device provisioned over BLE, then online over MQTT — receiving reads/writes, replying, and publishing readings/events.

## Device lifecycle

- `MeoDevice(model)` creates one device instance
- Declare caps with `addCap(key, onWrite, onRead)` in `setup()`, before `begin()`
- `begin()` starts storage, BLE, and provisioning; connects to Wi-Fi if credentials are already stored
- `loop()` drives provisioning, detects when Wi-Fi comes up, then runs MQTT messaging (broker host/port come from storage, written during BLE provisioning)
- `isProvisioned()` / `isWifiConnected()` / `isMqttConnected()` report status

## API reference

### Construction & identity

- `MeoDevice()` / `MeoDevice(model)`
- `setDeviceInfo(model, manufacturer)`
- `setFirmwareVersion(version)` — reported in the cap report, default `"0.0.0"`

### Capabilities & handlers

- `addCap(key, onWrite = nullptr, onRead = nullptr)` → idx, or -1 if rejected. The key is yours to pick: `[a-z0-9_]{1,32}`, unique, at most 16 caps, and the whole report must fit 512 bytes. A cap's idx (its wire id) is its declaration order.
  - `onWrite`: `bool fn(int16_t value)`; return `false` to reply `MEO_ERR_HANDLE_FAILED`. Without one, writes reply `MEO_ERR_OP_NOT_SUPPORTED`.
  - `onRead`: `int16_t fn()`. Without one, a read answers the cap's last value written or sent (0 after boot — call `sendEvent` once online if the cap boots non-zero).
- Every cap can be read, written and sent as an event; handlers decide which actually do something.
- All registration happens in `setup()`, before `begin()`. Changing the cap list on a provisioned device requires re-provisioning, since the edge learns the list only then.
- Values are `int16`. Send decimals scaled x100 (23.45 °C → `2345`).
- `buildCapabilityPayload(out, cap)` — serializes the declared set into the capability characteristic payload. Called internally by `begin()`; exposed mainly for testing.

### Readings & events

- `sendEvent(key, value)` — publish the cap's value unsolicited (readings and events alike). Also updates the value reads return. Returns `false` while offline or for an unknown key.

### Wi-Fi & lifecycle

- `beginWifi(ssid, pass)` — connect directly, bypassing BLE provisioning (local development only)
- `setBroker(host, port)` — override the MQTT broker, bypassing stored provisioning data (local development only; pairs with `beginWifi`)
- `begin()` → `bool` — false only if storage or BLE init fails
- `loop()` — call on every `loop()` iteration
- `isProvisioned()`, `isWifiConnected()`, `isMqttConnected()`

## Provisioning

When Wi-Fi is missing, the device advertises the MEO provisioning BLE service and walks the edge through MAC → capabilities → Wi-Fi config → status notify. The exact GATT contract (UUIDs, payload formats, status states) is documented once, shared with the edge, in `meo-edge/docs/firmware_development_guide.md` — treat that file as the source of truth rather than this one.

## Capabilities

There is no shared catalog: the device defines its own caps by key and reports them to the edge during provisioning as `{"model":..,"fw":..,"caps":["led","temp"]}`. Array position is the cap's idx on the wire.

## Messaging

Once Wi-Fi is up, `MeoDevice` connects to the edge's broker (host/port stored during BLE provisioning, keys `mq_host`/`mq_port`), subscribes to `meo/v1/device/{id}/down`, dispatches each READ/WRITE to the cap's handler, and replies OK/ERR on `meo/v1/device/{id}/up` (events go there too). Every frame is 4 bytes: `type(3b)|seq(5b)`, `idx`, `int16` value little-endian. Reconnects retry every 5 s; `down` is QoS 1, replies and events are QoS 0 (PubSubClient publish limitation).

## Identity

- The board MAC is the default stable identity.
- Students should not need to type device IDs or MQTT credentials.

## What's not here yet

Broker authentication (the broker is an open local listener for now) and device online/offline presence are deliberately out of the messaging contract.
