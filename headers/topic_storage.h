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

private:
    shared_mutex mtx;
    set<MqttTopic> topics;
};

#endif //MQTT_BROKER_TOPIC_STORAGE_H
