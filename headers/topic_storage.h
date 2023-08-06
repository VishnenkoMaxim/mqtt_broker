#ifndef MQTT_BROKER_TOPIC_STORAGE_H
#define MQTT_BROKER_TOPIC_STORAGE_H

#include "mqtt_protocol.h"
#include <shared_mutex>

using namespace mqtt_protocol;

class CTopicStorage{
public:
    void StoreTopicValue(uint8_t qos, uint16_t id, const string& topic_name, const shared_ptr<MqttBinaryDataEntity>& data);
    void StoreTopicValue(const MqttTopic& topic);

    MqttBinaryDataEntity GetStoredValue(const string& topic_name, bool& found);
    shared_ptr<MqttBinaryDataEntity> GetStoredValuePtr(const string& topic_name);
    MqttTopic GetTopic(const string& topic_name, bool& found);
    void DeleteTopicValue(const MqttTopic& _topic);

    void AddQoSTopic(const string& client_id, const MqttTopic& topic);
    MqttTopic GetQoSTopic(const string& client_id, const uint16_t packet_id, bool &found);
    void DelQoSTopic(const string& client_id, const uint16_t packet_id);
    uint32_t GetQoSTopicCount();

private:
    shared_mutex mtx;
    set<MqttTopic> topics;

    shared_mutex qos_mtx;
    unordered_multimap<string, MqttTopic> qos_2_topics;
};

#endif //MQTT_BROKER_TOPIC_STORAGE_H
