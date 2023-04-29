#include "mqtt_protocol.h"

using namespace mqtt_protocol;
using namespace temp_funcs;

[[nodiscard]] uint8_t mqtt_protocol::ReadVariableInt(const int fd, int &value){
    uint8_t single_byte;
    uint32_t multiplayer = 1;
    value = 0;
    while(true){
        int ret = read(fd, &single_byte, sizeof(single_byte));
        if (ret != 1) {
            return mqtt_err::read_err;
        }
        value += (single_byte & 0x7F)*multiplayer;
        if (multiplayer > mult<128,3>::value){
            return mqtt_err::var_int_err;
        }
        multiplayer *= 128;
        if ((single_byte & 0x80) == 0) break;
    }
    return mqtt_err::ok;
}

[[nodiscard]] uint8_t mqtt_protocol::ReadVariableInt(const uint8_t *buf, int &value, uint8_t &size){
    uint8_t single_byte;
    uint32_t multiplayer = 1;
    value = 0;
    uint8_t offset = 0;
    while(true){
        single_byte = *(buf + offset);
        value += (single_byte & 0x7F)*multiplayer;
        if (multiplayer > mult<128,3>::value){
            return mqtt_err::var_int_err;
        }
        multiplayer *= 128;
        offset++;
        if ((single_byte & 0x80) == 0) break;
    }
    size = offset;
    return mqtt_err::ok;
}

MqttDataEntity::MqttDataEntity(const uint8_t _type, const uint8_t *_data) : type(_type) {
    switch (type) {
        case mqtt_data_type::two_byte : {
            data = shared_ptr<uint8_t>(new uint8_t[2]);
            memcpy(data.get(), _data, 2);
        }; break;

        case mqtt_data_type::four_byte : {
            data = shared_ptr<uint8_t>(new uint8_t[4]);
            memcpy(data.get(), _data, 4);
        }; break;

        case mqtt_data_type::mqtt_string : {
            uint16_t len;
            memcpy(&len, _data, 2);
            data = shared_ptr<uint8_t>(new uint8_t[len + 2]);
            memcpy(data.get(), _data, len + 2);
        }; break;

        case mqtt_data_type::binary_data : {
            uint16_t len;
            memcpy(&len, _data, 2);
            data = shared_ptr<uint8_t>(new uint8_t[len + 2]);
            memcpy(data.get(), _data, len + 2);
        }; break;

        case mqtt_data_type::mqtt_string_pair : {
            uint16_t len;
            uint16_t len_2;
            memcpy(&len, _data, 2);
            memcpy(&len_2, _data + len + 2, 2);
            data = shared_ptr<uint8_t>(new uint8_t[len + len_2 + sizeof(len) + sizeof(len_2)]);
            memcpy(data.get(), _data, len + len_2 + sizeof(len) + sizeof(len_2));
        }; break;

        default : {
            type = mqtt_data_type::undefined;
            data = nullptr;
        }
    };
}

uint8_t MqttDataEntity::GetType(){
    return type;
}

uint8_t* MqttDataEntity::GetData(){
    return data.get();
}

shared_ptr<pair<MqttStringEntity, MqttStringEntity>> MqttEntity::GetPair() {
    return nullptr;
}

uint8_t MqttEntity::GetType(){
    return type;
}

uint32_t MqttByteEntity::Size() {
    return sizeof(uint8_t);
}

uint8_t* MqttByteEntity::GetData(){
    return (uint8_t *) data.get();
}

uint32_t MqttTwoByteEntity::Size() {
    return sizeof(uint16_t);
}

uint8_t* MqttTwoByteEntity::GetData(){
    return (uint8_t *) data.get();
}

uint32_t MqttFourByteEntity::Size(){
    return sizeof(uint32_t);
}

uint8_t* MqttFourByteEntity::GetData(){
    return (uint8_t *) data.get();
}

uint32_t MqttStringEntity::Size(){
    return sizeof(uint16_t) + data->size();
}

uint8_t* MqttStringEntity::GetData(){
    return (uint8_t *) data->data();
}

uint32_t MqttBinaryDataEntity::Size(){
    return sizeof(uint16_t) + size;
}

uint8_t* MqttBinaryDataEntity::GetData(){
    return (uint8_t *) data.get();
}

uint32_t MqttStringPairEntity::Size(){
    return data->first.Size() + data->second.Size();
}

uint8_t* MqttStringPairEntity::GetData(){
    return data->first.GetData();
};

uint32_t MqttVIntEntity::Size(){
    return 4;
}

uint8_t* MqttVIntEntity::GetData(){
    return (uint8_t *) data.get();
};

shared_ptr<pair<MqttStringEntity, MqttStringEntity>> MqttStringPairEntity::GetPair(){
    return data;
}

uint8_t*    MqttProperty::GetData(){
    return property->GetData();
}

uint32_t    MqttProperty::Size(){
    return property->Size();
}

uint8_t MqttProperty::GetId() const{
    return id;
}

uint8_t MqttProperty::GetType() {
    return property->GetType();
}

shared_ptr<pair<MqttStringEntity, MqttStringEntity>>     MqttProperty::GetPair(){
    return property->GetPair();
}

void MqttPropertyChain::AddProperty(shared_ptr<MqttProperty> entity){
    properties.push_back(entity);
}
uint32_t MqttPropertyChain::Count(){
    return properties.size();
}
shared_ptr<MqttProperty> MqttPropertyChain::GetProperty(unsigned int i){
    return properties[i];
}

void ConnectVH::CopyFromNet(const uint8_t *buf){
    memcpy(this, buf, sizeof(ConnectVH));
    uint16_t tmp = ntohs(prot_name_len);
    prot_name_len = tmp;
    tmp = ntohs(alive);
    alive = tmp;
}

shared_ptr<MqttProperty> CreateProperty(const uint8_t *buf, uint8_t &size){
    if (buf == nullptr){
        size = 0;
        return nullptr;
    }

    uint8_t id = buf[0];
    size = 1;
    switch (id){
        case payload_format_indicator: case request_problem_information: case request_response_information: case maximum_qos:
        case retain_available: case wildcard_subscription_available: case subscription_identifier_available: case shared_subscription_available: {
            size += 1;
            return make_shared<MqttProperty>(id, shared_ptr<MqttEntity>(new MqttByteEntity(&buf[1])));
        }

        case server_keep_alive: case receive_maximum: case topic_alias_maximum: case topic_alias: {
            size += 2;
            uint16_t val = ConvertToHost2Bytes(&buf[1]);
            return make_shared<MqttProperty>(id, shared_ptr<MqttEntity>(new MqttTwoByteEntity((uint8_t *)&val)));
        }

        case message_expiry_interval: case session_expiry_interval: case will_delay_interval: case maximum_packet_size:{
            size += 4;
            uint32_t val = ConvertToHost4Bytes(&buf[1]);
            return make_shared<MqttProperty>(id, shared_ptr<MqttEntity>(new MqttFourByteEntity((uint8_t *)&val)));
        }

        case content_type: case response_topic: case assigned_client_identifier: case authentication_method:
        case response_information: case server_reference: case reason_string: {
            uint16_t len;
            len = ConvertToHost2Bytes(&buf[1]);
            size += sizeof(len);
            uint16_t offset = size;
            size += len;
            return make_shared<MqttProperty>(id, shared_ptr<MqttEntity>(new MqttStringEntity(len, &buf[offset])));
        }

        case correlation_data: case authentication_data: {
            uint16_t len;
            len = ConvertToHost2Bytes(&buf[1]);
            size += sizeof(len);
            uint16_t offset = size;
            size += len;
            return make_shared<MqttProperty>(id, shared_ptr<MqttEntity>(new MqttBinaryDataEntity(len, &buf[offset])));
        }

        case user_property:{
            uint16_t len_1;
            uint16_t len_2;
            len_1 = ConvertToHost2Bytes(&buf[1]);
            size += sizeof(len_1);
            len_2 = ConvertToHost2Bytes(&buf[size + len_1]);
            size += len_1;
            size += sizeof(len_2);
            size += len_2;
            return make_shared<MqttProperty>(id, shared_ptr<MqttEntity>(new MqttStringPairEntity(MqttStringEntity(len_1, &buf[1 + sizeof(len_1)]),
                                                                                         MqttStringEntity(len_2, &buf[1 + sizeof(len_1) + len_1 + sizeof(len_2)]))));
        }

        case subscription_identifier:{
            int val;
            uint8_t res = ReadVariableInt(buf, val, size);
            if (res == mqtt_err::ok){
                return make_shared<MqttProperty>(id, shared_ptr<MqttEntity>(new MqttVIntEntity((uint8_t *)&val)));
            } else {
                size = 0;
                return nullptr;
            }
        }

        default: {
            size = 0;
            return nullptr;
        }
    }
}