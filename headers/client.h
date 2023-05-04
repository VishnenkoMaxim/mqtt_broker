#ifndef MQTT_BROKER_CLIENT_H
#define MQTT_BROKER_CLIENT_H

#include <utility>

#include "mqtt_protocol.h"
using namespace mqtt_protocol;

class Client{
private:
    string ip{};
    string client_id{};
    uint8_t state;
    uint8_t flags;
    uint16_t alive;
    MqttPropertyChain conn_properties;

public:
    explicit Client(string _ip) : ip(std::move(_ip)), state(0), flags(0), alive(0) {}

    void SetConnFlags(uint8_t _flags);
    void SetConnAlive(uint16_t _alive);

    bool isUserNameFlag() const;
    bool isPwdFlag() const;
    bool isWillRetFlag() const;
    uint8_t WillQoSFlag() const;
    bool isWillFlag() const;
    bool isCleanFlag() const;
};

#endif //MQTT_BROKER_CLIENT_H
