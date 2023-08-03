#include "mqtt_protocol.h"

using namespace mqtt_protocol;

MqttTopic::MqttTopic(uint8_t _qos, uint16_t _id, const string &_name, const shared_ptr<MqttBinaryDataEntity> &_data) : qos(_qos), id(_id), name(_name), data(_data){
    if (qos > 2) qos = 0;
}

bool MqttTopic::operator==(const string &str){
    return name == str;
}

bool MqttTopic::operator <(const MqttTopic& _topic) const{
    //return name < _topic.name;
    return id < _topic.id;
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

uint8_t MqttTopic::GetQoS() const{
    return qos;
}

string MqttTopic::GetName() const{
    return name;
}

void MqttTopic::SetPacketID(uint16_t new_id){
    id = new_id;
}

void MqttTopic::SetQos(uint8_t _qos){
    qos = _qos;
}

void MqttTopic::SetName(const string& _name){
    name = _name;
}
