#ifndef MQTT_BROKER_CLIENT_H
#define MQTT_BROKER_CLIENT_H

#include <utility>

#include "mqtt_protocol.h"
using namespace mqtt_protocol;

class Client{
private:
    string ip{};
    MqttStringEntity        client_id;
    queue<MqttTopic>        topics_to_send;
    unordered_map<string, uint8_t>   subscribed_topics;

    [[maybe_unused]] uint8_t state;
    uint8_t flags;
    uint16_t alive;
    time_t time_last_packet;
public:
    MqttPropertyChain conn_properties;

    explicit Client(string _ip);

    void SetConnFlags(uint8_t _flags);
    void SetConnAlive(uint16_t _alive);
    void SetID(const string& _id);
    void AddSubscription(const string &_topic_name, uint8_t options);

    bool isUserNameFlag() const;
    bool isPwdFlag() const;
    bool isWillRetFlag() const;
    uint8_t WillQoSFlag() const;
    bool isWillFlag() const;
    bool isCleanFlag() const;

    string GetID() const;
    string& GetIP();
    time_t GetPacketLastTime() const;
    uint16_t GetAlive() const;
    unordered_map<string, uint8_t>::const_iterator CFind(const string &_topic_name);
    unordered_map<string, uint8_t>::const_iterator CEnd();

    void SetPacketLastTime(time_t _cur_time);

    ~Client(){
        conn_properties.~MqttPropertyChain();
    }
};

#endif //MQTT_BROKER_CLIENT_H
