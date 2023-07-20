#include "mqtt_protocol.h"

using namespace mqtt_protocol;

MqttTopic::MqttTopic(uint16_t _id, const string &_name, const shared_ptr<MqttBinaryDataEntity> &_data) : id(_id), name(_name), data(_data){}

bool MqttTopic::operator==(const string &str){
    return name == str;
}

bool MqttTopic::operator <(const MqttTopic& _topic) const{
    return name < _topic.name;
}

uint32_t MqttTopic::GetSize(){
    return data->Size() - 2;
}

const uint8_t* MqttTopic::GetData(){
    return data->GetData();
}

string MqttTopic::GetString() const {
    return data->GetString();
}

MqttBinaryDataEntity MqttTopic::GetValue() const{
    return *data;
}

shared_ptr<MqttBinaryDataEntity> MqttTopic::GetPtr() const{
    return data;
}

uint16_t MqttTopic::GetID() const{
    return id;
}

