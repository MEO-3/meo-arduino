#pragma once

#include <Arduino.h>
#include "../Meo3_Logger.h"
#include "../mqtt/Meo3_Mqtt.h"


class MeoMsg {
public:
    typedef bool (*MeoWriteHandler)(int16_t value);
    typedef int16_t (*MeoReadHandler)();

    static const uint8_t MEO_MAX_CAPS = 16;

    int addCap(const char* key, MeoWriteHandler onWrite, MeoReadHandler onRead);
    uint8_t capCount() const { return _capCount; }
    const char* capKey(uint8_t idx) const { return idx < _capCount ? _caps[idx].key : nullptr; }

    bool begin(MeoMqttClient* mqtt, const char* deviceId);
    void loop();

    bool isConnected();
    bool sendEvent(const char* key, int16_t value);

private:
    static const uint32_t RECONNECT_INTERVAL_MS = 5000;

    struct Cap {
        const char*     key;
        MeoWriteHandler onWrite;
        MeoReadHandler  onRead;
        int16_t         last;
    };

    MeoMqttClient* _mqtt = nullptr;

    char _topicDown[40] = {0};
    char _topicUp[40] = {0};

    Cap     _caps[MEO_MAX_CAPS];
    uint8_t _capCount = 0;

    uint32_t _lastConnectAttempt = 0;
    bool     _subscribed = false;

    static void _onMessageStatic(const char* topic, const uint8_t* payload, unsigned int length, void* ctx);
    void _handleFrame(const uint8_t* payload, unsigned int length);
    bool _send(uint8_t type, uint8_t seq, uint8_t idx, int16_t value);
};
