#include "Meo3_Msg.h"
#include "Meo3_MsgErr.h"
#include <string.h>

// Frame: mirror of the edge's MeoEdgeMsgFrame.java.
static const unsigned int FRAME_SIZE = 4;
static const uint8_t TYPE_READ = 0;
static const uint8_t TYPE_WRITE = 1;
static const uint8_t TYPE_OK = 2;
static const uint8_t TYPE_ERR = 3;
static const uint8_t TYPE_EVENT = 4;
static const uint8_t SEQ_MASK = 0x1F;

// Same rule as the edge's provisioning check: [a-z0-9_]{1,32}.
static bool validKey(const char *key) {
    if (!key)
        return false;
    size_t n = strlen(key);
    if (n == 0 || n > 32)
        return false;
    for (size_t i = 0; i < n; ++i) {
        char c = key[i];
        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_'))
            return false;
    }
    return true;
}

int MeoMsg::addCap(const char *key, uint8_t type, MeoWriteHandler onWrite, MeoReadHandler onRead) {
    if (!validKey(key)) {
        loge("MSG", "Invalid cap key '%s' (use a-z, 0-9, _; max 32)", key ? key : "");
        return -1;
    }
    for (uint8_t i = 0; i < _capCount; ++i) {
        if (strcmp(_caps[i].key, key) == 0) {
            loge("MSG", "Duplicate cap '%s'", key);
            return -1;
        }
    }
    if (_capCount >= MEO_MAX_CAPS) {
        loge("MSG", "Cap table full (%u); ignoring '%s'", MEO_MAX_CAPS, key);
        return -1;
    }
    _caps[_capCount] = {key, type, onWrite, onRead, 0};
    return _capCount++;
}

bool MeoMsg::begin(MeoMqttClient *mqtt, const char *deviceId) {
    if (!mqtt || !deviceId || !deviceId[0])
        return false;
    _mqtt = mqtt;
    snprintf(_topicDown, sizeof(_topicDown), "meo/v1/device/%s/down", deviceId);
    snprintf(_topicUp, sizeof(_topicUp), "meo/v1/device/%s/up", deviceId);
    _mqtt->setMessageHandler(&MeoMsg::_onMessageStatic, this);
    return true;
}

void MeoMsg::loop() {
    if (!_mqtt)
        return;

    if (!_mqtt->isConnected()) {
        _subscribed = false;
        uint32_t now = millis();
        if (_lastConnectAttempt != 0 && now - _lastConnectAttempt < RECONNECT_INTERVAL_MS)
            return;
        _lastConnectAttempt = now;
        if (!_mqtt->connect())
            return;
    }

    if (!_subscribed) {
        // Clean session on every connect, so (re)subscribe each time
        _subscribed = _mqtt->subscribe(_topicDown, 1);
        if (_subscribed)
            logi("MSG", "Messaging online (%s)", _topicDown);
    }

    _mqtt->loop();
}

bool MeoMsg::isConnected() {
    return _mqtt && _mqtt->isConnected() && _subscribed;
}

bool MeoMsg::sendEvent(const char *key, int16_t value) {
    for (uint8_t i = 0; i < _capCount; ++i) {
        if (strcmp(_caps[i].key, key) == 0) {
            _caps[i].last = value;
            return isConnected() && _send(TYPE_EVENT, 0, i, value);
        }
    }
    logw("MSG", "sendEvent: unknown cap '%s'", key ? key : "");
    return false;
}

void MeoMsg::_onMessageStatic(const char *topic, const uint8_t *payload, unsigned int length,
                              void *ctx) {
    MeoMsg *self = reinterpret_cast<MeoMsg *>(ctx);
    if (!self || !topic || strcmp(topic, self->_topicDown) != 0)
        return;
    self->_handleFrame(payload, length);
}

void MeoMsg::_handleFrame(const uint8_t *payload, unsigned int length) {
    if (length < 1)
        return; // no seq to reply to

    uint8_t type = payload[0] >> 5;
    uint8_t seq = payload[0] & SEQ_MASK;
    uint8_t idx = length > 1 ? payload[1] : 0;
    if (length != FRAME_SIZE || (type != TYPE_READ && type != TYPE_WRITE)) {
        logw("MSG", "Bad frame: len=%u type=%u", length, type);
        _send(TYPE_ERR, seq, idx, MEO_ERR_BAD_REQUEST);
        return;
    }
    if (idx >= _capCount) {
        logw("MSG", "Unknown cap idx %u", idx);
        _send(TYPE_ERR, seq, idx, MEO_ERR_UNKNOWN_CAP);
        return;
    }

    Cap &cap = _caps[idx];
    if (type == TYPE_READ) {
        if (cap.onRead)
            cap.last = cap.onRead();
        _send(TYPE_OK, seq, idx, cap.last);
        return;
    }

    int16_t value = (int16_t)(payload[2] | (payload[3] << 8));
    if (!cap.onWrite) {
        _send(TYPE_ERR, seq, idx, MEO_ERR_OP_NOT_SUPPORTED);
        return;
    }
    if (!cap.onWrite(value)) {
        _send(TYPE_ERR, seq, idx, MEO_ERR_HANDLE_FAILED);
        return;
    }
    cap.last = value;
    _send(TYPE_OK, seq, idx, value);
}

bool MeoMsg::_send(uint8_t type, uint8_t seq, uint8_t idx, int16_t value) {
    uint16_t u = (uint16_t)value;
    uint8_t buf[FRAME_SIZE] = {(uint8_t)((type << 5) | (seq & SEQ_MASK)), idx, (uint8_t)(u & 0xFF),
                               (uint8_t)(u >> 8)};
    return _mqtt->publish(_topicUp, buf, sizeof(buf), false);
}
