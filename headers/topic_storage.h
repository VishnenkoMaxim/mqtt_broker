#ifndef MQTT_BROKER_TOPIC_STORAGE_H
#define MQTT_BROKER_TOPIC_STORAGE_H

#include "mqtt_protocol.h"
#include <shared_mutex>

using namespace mqtt_protocol;

class CTopicStorage{
public:
    void StoreTopicValue(const uint16_t id, const string& topic_name, const MqttBinaryDataEntity& data);
    MqttBinaryDataEntity GetStoredValue(const string& topic_name, bool& found);
    void DeleteTopicValue(const string& topic_name);

private:
    shared_mutex mtx;
    set<MqttTopic> topics;
};


#endif //MQTT_BROKER_TOPIC_STORAGE_H
