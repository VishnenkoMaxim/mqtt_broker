#ifndef MQTT_BROKER_CONN_PROPERTIES_H
#define MQTT_BROKER_CONN_PROPERTIES_H

#include "mqtt_protocol.h"
#include "functions.h"

using namespace mqtt_protocol;

class ConnProperties{
private:
    uint8_t flags;
    MqttPropertyChain properties;
public:
    ConnProperties() : flags(0){}
    explicit ConnProperties(const uint8_t _flags) : flags(_flags){}

    void SetFlags(uint8_t _flags);

    bool isUserNameFlag() const;
    bool isPwdFlag() const;
    bool isWillRetFlag() const;
    uint8_t WillQoSFlag() const;
    bool isWillFlag() const;
    bool isCleanFlag() const;

    void AddProperty(shared_ptr<MqttProperty> property);
};

#endif //MQTT_BROKER_CONN_PROPERTIES_H
