#include "topic_storage.h"

using namespace std;

void CTopicStorage::StoreTopicValue(const uint8_t qos, const uint16_t id, const string& topic_name, const shared_ptr<MqttBinaryDataEntity>& data){
    unique_lock lock(mtx);
    MqttTopic topic(qos, id, topic_name, data);
    topics.erase(topic);
    topics.emplace(topic);
}

void CTopicStorage::StoreTopicValue(const MqttTopic& topic){
    unique_lock lock(mtx);
    topics.erase(topic);
    topics.emplace(topic);
}

MqttBinaryDataEntity CTopicStorage::GetStoredValue(const string& topic_name, bool& found){
    shared_lock lock(mtx);
    found = false;
    MqttTopic tmp_topic(0, 0, topic_name, nullptr);
    auto it = topics.find(tmp_topic);
    if(it != topics.end()) {
        found = true;
        return it->GetValue();
    }
    return MqttBinaryDataEntity{};
}

shared_ptr<MqttBinaryDataEntity> CTopicStorage::GetStoredValuePtr(const string& topic_name){
    shared_lock lock(mtx);
    MqttTopic tmp_topic(0, 0, topic_name, nullptr);
    auto it = topics.find(tmp_topic);
    if(it != topics.end()) {
        return it->GetPtr();
    }
    return nullptr;
}

MqttTopic CTopicStorage::GetTopic(const string& topic_name, bool& found) {
    shared_lock lock(mtx);
    found = false;
    MqttTopic tmp_topic(0, 0, topic_name, nullptr);
    auto it = topics.find(tmp_topic);
    if(it != topics.end()) {
        found = true;
        return *it;
    }
    return tmp_topic;
}

void CTopicStorage::DeleteTopicValue(const MqttTopic& _topic){
    unique_lock lock(mtx);
    topics.erase(_topic);
}


//qos topics
void CTopicStorage::AddQoSTopic(const string& client_id, const MqttTopic& topic){
    unique_lock lock(qos_mtx);
    qos_2_topics.insert(make_pair(client_id, topic));
}

MqttTopic CTopicStorage::GetQoSTopic(const string& client_id, const uint16_t packet_id, bool& found){
    shared_lock lock(qos_mtx);
    found = false;
    auto it = qos_2_topics.find(client_id);

    while (it != qos_2_topics.end() && it->first == client_id){
        if (it->second.GetID() == packet_id){
            found = true;
            return it->second;
        }
        it++;
    }
    return MqttTopic{0, 0, string{""}, nullptr};

}

void CTopicStorage::DelQoSTopic(const string& client_id, const uint16_t packet_id){
    unique_lock lock(qos_mtx);
    auto it = qos_2_topics.find(client_id);

    while (it != qos_2_topics.end() && it->first == client_id){
        if (it->second.GetID() == packet_id){
            qos_2_topics.erase(it);
            return;
        }
        it++;
    }
}

uint32_t CTopicStorage::GetQoSTopicCount(){
    return qos_2_topics.size();
}

