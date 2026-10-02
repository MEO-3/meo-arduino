#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <string>
#include "Meo3_Logger.h"

#include "storage/Meo3_Storage.h"
#include "ble/Meo3_Ble.h"
#include "provision/Meo3_BleProvision.h"
#include "mqtt/Meo3_Mqtt.h"
#include "msg/Meo3_Msg.h"

class MeoDevice {
public:
    MeoDevice();
    MeoDevice(const char* model);
    // CSV of tags to enable DEBUG logs for (e.g. "DEVICE,PROV")

    // Device model and manufacturer. The model is reported to the gateway in
    // the capability report; the human-facing device name lives on the gateway,
    // not on firmware.
    void setDeviceInfo(const char* model, const char* manufacturer);

    // Firmware version reported in the capability report (default "0.0.0")
    void setFirmwareVersion(const char* version);

    // Declare a cap by key ([a-z0-9_]{1,32}, unique, max 16) in setup(), before
    // begin(); its idx is the call order. Either handler may be null: a read
    // without a handler answers the last value written or sent (0 after boot),
    // a write without one replies MEO_ERR_OP_NOT_SUPPORTED. Returns idx or -1.
    // Changing the cap list after provisioning requires re-provisioning.
    int addCap(const char* key,
               MeoMsg::MeoWriteHandler onWrite = nullptr,
               MeoMsg::MeoReadHandler onRead = nullptr);

    // Serialize the {"model","fw","caps":[...]} report served over the BLE
    // capability characteristic into out. Returns length, or 0 if out is too small.
    size_t buildCapabilityPayload(char* out, size_t cap) const;

    // Publish a cap's value unsolicited (reading or event). Values are int16;
    // send decimals scaled x100 (23.45 -> 2345). Returns false while offline.
    bool sendEvent(const char* key, int16_t value);

    // Override Wi-Fi upfront (development / bypass provisioning)
    void beginWifi(const char* ssid, const char* pass);

    // Override the MQTT broker upfront (development / bypass provisioning).
    // Normally the broker comes from storage, written during BLE provisioning.
    void setBroker(const char* host, uint16_t port = 1883);

    // Lifecycle
    bool begin();   // Init storage, BLE, provisioning service; connect if already provisioned
    void loop();    // Drive provisioning; detect Wi-Fi connect; stop BLE when online; run messaging

    // Status
    bool isProvisioned() const;      // Wi-Fi connected and MAC identity set
    bool isWifiConnected() const { return _wifiReady; }
    bool isMqttConnected() { return _msg.isConnected(); }

private:
    const char* _model;
    const char* _manufacturer;
    const char* _fwVersion;

    const char* _wifiSsid;
    const char* _wifiPass;

    std::string _deviceId;   // Wi-Fi MAC — stable device identity

    MeoStorage      _storage;
    MeoBle          _ble;
    MeoBleProvision _prov;
    MeoMqttClient   _mqtt;
    MeoMsg    _msg;

    // Development broker override (setBroker); normally loaded from storage
    const char* _brokerHostOverride = nullptr;
    uint16_t    _brokerPortOverride = 1883;
    std::string _storedBrokerHost;

    bool _wifiReady  = false;
    bool _bleActive  = false;
    bool _msgStarted = false;  // start attempted (one-shot)
    bool _msgActive  = false;  // started successfully; _msg.loop() runs


    bool _tryConnectStoredWifi();
    void _ensureMacIdentity();
    void _startMsg();
};
