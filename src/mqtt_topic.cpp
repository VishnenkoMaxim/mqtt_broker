#include "mqtt_protocol.h"

using namespace mqtt_protocol;

MqttTopic::MqttTopic(uint16_t _id, const string &_name, uint16_t _len, const uint8_t * _data) : id(_id), name(_name), data(_len, _data){}
MqttTopic::MqttTopic(uint16_t _id, const string &_name, const MqttBinaryDataEntity &_data) : id(_id), name(_name), data(_data){}

bool MqttTopic::operator==(const string &str){
    return name == str;
}

bool MqttTopic::operator <(const MqttTopic& _topic) const{
    return name < _topic.name;
}

uint32_t MqttTopic::GetSize(){
    return data.Size() - 2;
}

const uint8_t* MqttTopic::GetData(){
    return data.GetData();
}

string MqttTopic::GetString() const {
    return data.GetString();
}

MqttBinaryDataEntity MqttTopic::GetValue() const{
    return data;
}

MqttBinaryDataEntity& MqttTopic::GetValueRef(){
    return data;
}

