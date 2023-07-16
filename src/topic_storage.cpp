#include "topic_storage.h"

void CTopicStorage::StoreTopicValue(const string& topic_name, const MqttBinaryDataEntity& data){
    unique_lock lock(mtx);
    MqttTopic topic(0, topic_name, data);
    topics.erase(topic);
    topics.emplace(topic);
}

MqttBinaryDataEntity CTopicStorage::GetStoredValue(const string& topic_name){
    shared_lock lock(mtx);
    MqttTopic tmp_topic(0, topic_name, MqttBinaryDataEntity{});
    auto it = topics.find(tmp_topic);
    if(it != topics.end())
        return it->GetValue();
    return MqttBinaryDataEntity{};
}

void CTopicStorage::DeleteTopicValue(const string& topic_name){
    unique_lock lock(mtx);
    topics.erase(MqttTopic(0, topic_name, MqttBinaryDataEntity{}));
}