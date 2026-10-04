# Contributing

This is the ESP32 firmware library that runs on MEO 3 devices. It's one of five independent repos in the MEO 3 workspace (`meo-edge` edge service, `meo-cloud`, this library, `meo-dist` packaging, `node-red-meo` Node-RED fork) — see the workspace-level `AGENTS.md` for how they fit together.

## Repo layout

- `lib/meo/` — the library itself, one subfolder per concern:
  - `Meo3_Device.{h,cpp}` — `MeoDevice`, the main entry point (lifecycle, capability declaration)
  - `ble/` — thin NimBLE wrapper (`MeoBle`)
  - `provision/` — BLE provisioning GATT service and state machine (`MeoBleProvision`)
  - `mqtt/` — MQTT transport wrapper (`MeoMqttClient`)
  - `msg/` — cap table, 4-byte device frame and error codes (`MeoMsg`, `Meo3_MsgErr.h`)
  - `storage/` — persisted Wi-Fi credentials (`MeoStorage`)
- `src/<project>/main.cpp` — dev sketches (`devkit-test`, `thingcube`), one PlatformIO env each
- `examples/` — sketches shipped with the library for end users
- `docs/key_concepts.md` — user-facing mental model and API reference

## Build, flash, monitor

PlatformIO, targeting `esp32-c3-devkitc-02` (see `platformio.ini`):

```bash
pio run -e devkit-test            # build
pio run -e devkit-test -t upload  # flash
pio device monitor -b 115200      # serial monitor
```

There's no test suite checked in yet (`test/` is the PlatformIO Unit Testing placeholder). If you add one, wire it into `pio test`.

## Wire contract

`msg/` mirrors the edge's `MeoEdgeMsgFrame.java`, the device topics in `MeoTopic.java`, and the device codes (1–99) in `MeoErr.java` (in `meo-edge`). `provision/` UUIDs mirror `ProvisionBleUuid.java`. There's no shared build-time check, so change both sides together (separate commits — independent repos).

## Conventions

- `lib/meo/` uses 4-space indent, matches the rest of the codebase.
- `examples/` use 2-space indent and stick to the simple `MeoDevice` API surface — they're read by students, so avoid introducing advanced/internal APIs there.
- The BLE provisioning GATT contract (UUIDs, payload formats, status states) is documented once, in `meo-edge/docs/firmware_development_guide.md`. Don't duplicate it in this repo's README or docs — link to it instead, so the two don't drift apart.
- Commits: small and scoped; Conventional Commits style (`feat:`, `fix:`, `refactor:`, `docs:`) matches existing history.
- Don't commit Wi-Fi credentials, device keys, or other runtime config.
