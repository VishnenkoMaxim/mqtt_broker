#ifndef MQTT_BROKER_CLIENT_H
#define MQTT_BROKER_CLIENT_H

#include "conn_properties.h"

using namespace mqtt_protocol;

class Client{
private:
    string ip{};
    string client_id{};
    uint8_t state;
    ConnProperties conn_properties;

public:
    explicit Client(const string &_ip) : ip(_ip) {
        state = 0;
    }

};

#endif //MQTT_BROKER_CLIENT_H
