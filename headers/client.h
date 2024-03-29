#pragma once
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-attributes"

#include <utility>
#include "mqtt_protocol.h"

class Client{
private:
    std::string ip{};
    mqtt_protocol::MqttStringEntity        client_id;
    std::queue<mqtt_protocol::MqttTopic>        topics_to_send;
    std::unordered_map<std::string, uint8_t>   subscribed_topics;

    uint8_t flags;
    uint16_t alive;
    time_t time_last_packet;
    uint16_t packet_id_gen;
    bool id_was_random_generated = false;
    uint8_t mqtt_version;

public:
    explicit Client(std::string _ip);

    mqtt_protocol::MqttPropertyChain conn_properties;
    mqtt_protocol::MqttPropertyChain will_properties;
    mqtt_protocol::MqttTopic will_topic;

    void SetConnFlags(uint8_t _flags);
    void SetConnAlive(uint16_t _alive);
    void SetID(const std::string& _id);
    void AddSubscription(const std::string &_topic_name, uint8_t options);
    bool MyTopic(const std::string &_topic, uint8_t& options);
    uint16_t GenPacketID();
    uint8_t DelSubscription(const std::string &_topic_name);

    bool isUserNameFlag() const;
    bool isPwdFlag() const;
    bool isWillRetFlag() const;
    uint8_t WillQoSFlag() const;
    bool isWillFlag() const;
    bool isCleanFlag() const;
    bool isRandomID() const;

    std::string GetID() const;
    std::string& GetIP();
    time_t GetPacketLastTime() const;
    uint16_t GetAlive() const;
    uint8_t GetClientMQTTVersion() const noexcept;
    void SetClientMQTTVersion(const uint8_t version) noexcept;
    std::unordered_map<std::string, uint8_t>::const_iterator CFind(const std::string &_topic_name);
    std::unordered_map<std::string, uint8_t>::const_iterator CEnd();

    void SetPacketLastTime(time_t _cur_time);
    void SetRandomID();

    ~Client(){
        //conn_properties.~MqttPropertyChain();
    }
};
