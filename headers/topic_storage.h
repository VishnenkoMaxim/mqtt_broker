#pragma once
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-attributes"

#include "mqtt_protocol.h"
#include <shared_mutex>

using namespace mqtt_protocol;

class CTopicStorage{
public:
    void StoreTopicValue(uint8_t qos, uint16_t id, const std::string& topic_name, const std::shared_ptr<MqttBinaryDataEntity>& data);
    void StoreTopicValue(const MqttTopic& topic);

    MqttBinaryDataEntity GetStoredValue(const std::string& topic_name, bool& found);
    std::shared_ptr<MqttBinaryDataEntity> GetStoredValuePtr(const std::string& topic_name);
    MqttTopic GetTopic(const std::string& topic_name, bool& found);
    void DeleteTopicValue(const MqttTopic& _topic);

    void AddQoSTopic(const std::string& client_id, const MqttTopic& topic);
    MqttTopic GetQoSTopic(const std::string& client_id, const uint16_t packet_id, bool &found);
    void DelQoSTopic(const std::string& client_id, const uint16_t packet_id);
    uint32_t GetQoSTopicCount();

private:
    std::shared_mutex mtx;
    std::set<MqttTopic> topics;

    std::shared_mutex qos_mtx;
    std::unordered_multimap<std::string, MqttTopic> qos_2_topics;
};
